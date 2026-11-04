/*
 * Part of Very Secure FTPd
 * Licence: GPL v2
 * Author: Chris Evans
 * standalone.c
 *
 * Code to listen on the network and launch children servants.
 */

#include <net/if.h>

#include "standalone.h"

#include "parseconf.h"
#include "tunables.h"
#include "sysutil.h"
#include "sysdeputil.h"
#include "utility.h"
#include "defs.h"
#include "hash.h"
#include "str.h"
#include "ipaddrparse.h"

#pragma CHECKED_SCOPE on

static unsigned int s_children;
static _Ptr<struct hash> s_p_ip_count_hash = ((void *)0);
static _Ptr<struct hash> s_p_pid_ip_hash = ((void *)0);
static unsigned int s_ipaddr_size;

static void handle_sigchld(_Ptr<void>  duff);
static void handle_sighup(_Ptr<void>  duff);
static void prepare_child(int sockfd);
static unsigned int handle_ip_count(_Array_ptr<void> p_raw_addr : byte_count(s_p_ip_count_hash->key_size));
static void drop_ip_count(_Array_ptr<void> p_raw_addr : byte_count(s_p_ip_count_hash->key_size));

static unsigned int hash_ip(unsigned int buckets, _Array_ptr<void> p_key : byte_count(0));
static unsigned int hash_pid(unsigned int buckets, _Array_ptr<void> p_key : byte_count(0));

struct vsf_client_launch
vsf_standalone_main(void)
{
  _Ptr<struct vsf_sysutil_sockaddr> p_accept_addr = 0;
  int listen_sock = -1;
  int retval;
  s_ipaddr_size = vsf_sysutil_get_ipaddr_size();
  if (tunable_listen && tunable_listen_ipv6)
  {
    die("run two copies of vsftpd for IPv4 and IPv6");
  }
  if (tunable_background)
  {
    int forkret = vsf_sysutil_fork();
    if (forkret > 0)
    {
      /* Parent, just exit */
      vsf_sysutil_exit(0);
    }
    /* Son, close standard FDs to avoid SSH hang-on-exit */
    vsf_sysutil_reopen_standard_fds();
    vsf_sysutil_make_session_leader();
  }
  if (tunable_listen)
  {
    listen_sock = vsf_sysutil_get_ipv4_sock();
  }
  else
  {
    listen_sock = vsf_sysutil_get_ipv6_sock();
  }
  vsf_sysutil_activate_reuseaddr(listen_sock);

  s_p_ip_count_hash = hash_alloc(256, s_ipaddr_size,
                                 sizeof(unsigned int), hash_ip);
  s_p_pid_ip_hash = hash_alloc(256, sizeof(int),
                               s_ipaddr_size, hash_pid);
  if (tunable_setproctitle_enable)
  {
    vsf_sysutil_setproctitle("LISTENER");
  }
  vsf_sysutil_install_sighandler(kVSFSysUtilSigCHLD, handle_sigchld, 0, 1);
  vsf_sysutil_install_sighandler(kVSFSysUtilSigHUP, handle_sighup, 0, 1);
  if (tunable_listen)
  {
    _Ptr<struct vsf_sysutil_sockaddr> p_sockaddr = 0;
    vsf_sysutil_sockaddr_alloc_ipv4(&p_sockaddr);
    vsf_sysutil_sockaddr_set_port(p_sockaddr,
                                  (unsigned short) tunable_listen_port);
    if (!tunable_listen_address)
    {
      vsf_sysutil_sockaddr_set_any(p_sockaddr);
    }
    else
    {
      if (!vsf_sysutil_inet_aton(tunable_listen_address, p_sockaddr))
      {
        die2("bad listen_address: ", tunable_listen_address);
      }
    }
    retval = vsf_sysutil_bind(listen_sock, p_sockaddr);
    vsf_sysutil_free<struct vsf_sysutil_sockaddr>(p_sockaddr);
    if (vsf_sysutil_retval_is_error(retval))
    {
      die("could not bind listening IPv4 socket");
    }
  }
  else
  {
    _Ptr<struct vsf_sysutil_sockaddr> p_sockaddr = 0;
    vsf_sysutil_sockaddr_alloc_ipv6(&p_sockaddr);
    vsf_sysutil_sockaddr_set_port(p_sockaddr,
                                  (unsigned short) tunable_listen_port);
    if (!tunable_listen_address6)
    {
      vsf_sysutil_sockaddr_set_any(p_sockaddr);
    }
    else
    {
      struct mystr addr_str = INIT_MYSTR;
      struct mystr scope_id = INIT_MYSTR;
      _Array_ptr<const unsigned char> p_raw_addr : count(sizeof(struct in6_addr)) = ((void *)0);
      unsigned int if_index = 0;

      /* See if we got a scope id */
      str_alloc_text(&addr_str, tunable_listen_address6);
      str_split_char(&addr_str, &scope_id, '%');
      if (str_getlen(&scope_id) > 0) {
        _Unchecked {
          if_index = if_nametoindex(((const char *)str_getbuf(&scope_id)));
        }
        str_free(&scope_id);
      }
      p_raw_addr = vsf_sysutil_parse_ipv6(&addr_str);
      str_free(&addr_str);
      if (!p_raw_addr)
      {
        die2("bad listen_address6: ", tunable_listen_address6);
      }
      vsf_sysutil_sockaddr_set_ipv6addr(p_sockaddr, p_raw_addr);
      vsf_sysutil_sockaddr_set_ipv6scope(p_sockaddr, if_index);
    }
    retval = vsf_sysutil_bind(listen_sock, p_sockaddr);
    vsf_sysutil_free<struct vsf_sysutil_sockaddr>(p_sockaddr);
    if (vsf_sysutil_retval_is_error(retval))
    {
      die("could not bind listening IPv6 socket");
    }
  }
  retval = vsf_sysutil_listen(listen_sock, VSFTP_LISTEN_BACKLOG);
  if (vsf_sysutil_retval_is_error(retval))
  {
    die("could not listen");
  }
  vsf_sysutil_sockaddr_alloc(&p_accept_addr);
  while (1)
  {
    struct vsf_client_launch child_info;
    _Array_ptr<void> p_raw_addr : byte_count(s_p_pid_ip_hash->value_size) = 0;
    int new_child;
    int new_client_sock;
    new_client_sock = vsf_sysutil_accept_timeout(
        listen_sock, p_accept_addr, 0);
    if (vsf_sysutil_retval_is_error(new_client_sock))
    {
      continue;
    }
    ++s_children;
    child_info.num_children = s_children;
    child_info.num_this_ip = 0;
    p_raw_addr = _Dynamic_bounds_cast<_Array_ptr<void>>(vsf_sysutil_sockaddr_get_raw_addr(p_accept_addr), byte_count(s_p_pid_ip_hash->value_size));
    _Array_ptr<void> p_raw_addr_key  : byte_count(s_p_ip_count_hash->key_size) = _Dynamic_bounds_cast<_Array_ptr<void>>(p_raw_addr, byte_count(s_p_ip_count_hash->key_size));
    child_info.num_this_ip = handle_ip_count(p_raw_addr_key);
    if (tunable_isolate)
    {
      if (tunable_http_enable && tunable_isolate_network)
      {
        new_child = vsf_sysutil_fork_isolate_all_failok();
      }
      else
      {
        new_child = vsf_sysutil_fork_isolate_failok();
      }
    }
    else
    {
      new_child = vsf_sysutil_fork_failok();
    }
    if (new_child != 0)
    {
      /* Parent context */
      vsf_sysutil_close(new_client_sock);
      if (new_child > 0)
      {
        _Array_ptr<void> key : byte_count(s_p_pid_ip_hash->key_size) = _Dynamic_bounds_cast<_Array_ptr<void>>(&new_child, byte_count(s_p_pid_ip_hash->key_size));
        hash_add_entry(s_p_pid_ip_hash, key, p_raw_addr);
      }
      else
      {
        /* fork() failed, clear up! */
        --s_children;
        drop_ip_count(p_raw_addr_key);
      }
      /* Fall through to while() loop and accept() again */
    }
    else
    {
      /* Child context */
      vsf_set_die_if_parent_dies();
      vsf_sysutil_close(listen_sock);
      prepare_child(new_client_sock);
      /* By returning here we "launch" the child process with the same
       * contract as xinetd would provide.
       */
      return child_info;
    }
  }
}

static void
prepare_child(int new_client_sock)
{
  /* We must satisfy the contract: command socket on fd 0, 1, 2 */
  vsf_sysutil_dupfd2(new_client_sock, 0);
  vsf_sysutil_dupfd2(new_client_sock, 1);
  vsf_sysutil_dupfd2(new_client_sock, 2);
  if (new_client_sock > 2)
  {
    vsf_sysutil_close(new_client_sock);
  }
}

static void
drop_ip_count(_Array_ptr<void> p_raw_addr : byte_count(s_p_ip_count_hash->key_size))
{
  unsigned int count;
  _Ptr<unsigned int> p_count = 0;
  _Array_ptr<void> result : byte_count(s_p_ip_count_hash->value_size) =
    hash_lookup_entry(s_p_ip_count_hash, p_raw_addr);
  p_count = _Dynamic_bounds_cast<_Ptr<unsigned int>>(result);
  if (!p_count)
  {
    bug("IP address missing from hash");
  }
  count = *p_count;
  if (!count)
  {
    bug("zero count for IP address");
  }
  count--;
  *p_count = count;
  if (!count)
  {
    hash_free_entry(s_p_ip_count_hash, p_raw_addr);
  }
}

static void
handle_sigchld(_Ptr<void> duff)
{
  unsigned int reap_one = 1;
  (void) duff;
  while (reap_one)
  {
    reap_one = (unsigned int)vsf_sysutil_wait_reap_one();
    if (reap_one)
    {
      /* Account total number of instances */
      --s_children;
      /* Account per-IP limit */
      _Array_ptr<void> key : byte_count(s_p_pid_ip_hash->key_size) = _Dynamic_bounds_cast<_Array_ptr<void>>(&reap_one, byte_count(s_p_pid_ip_hash->key_size));
      _Array_ptr<void> result : byte_count(s_p_pid_ip_hash->value_size) = hash_lookup_entry(s_p_pid_ip_hash, key);
      _Array_ptr<void> result_tmp : byte_count(s_p_ip_count_hash->key_size) = _Dynamic_bounds_cast<_Array_ptr<void>>(result, byte_count(s_p_ip_count_hash->key_size));
      drop_ip_count(result_tmp);
      hash_free_entry(s_p_pid_ip_hash, key);
    }
  }
}

static void
handle_sighup(_Ptr<void> duff)
{
  (void) duff;
  /* We don't crash the out the listener if an invalid config was added */
  tunables_load_defaults();
  vsf_parseconf_load_file(0, 0);
}

static unsigned int
hash_ip(unsigned int buckets, _Array_ptr<void> p_key : byte_count(0))
{
  _Array_ptr<const unsigned char> p_raw_ip : byte_count(s_ipaddr_size) = 0;
  _Unchecked {
    p_raw_ip = _Assume_bounds_cast<_Array_ptr<const unsigned char>>(p_key, byte_count(s_ipaddr_size));
  }
  unsigned int val = 0;
  int shift = 24;
  unsigned int i;
  for (i = 0; i < s_ipaddr_size; ++i)
  {
    val = val ^ (unsigned int) (p_raw_ip[i] << shift);
    shift -= 8;
    if (shift < 0)
    {
      shift = 24;
    }
  }
  return val % buckets;
}

static unsigned int
hash_pid(unsigned int buckets, _Array_ptr<void> p_key : byte_count(0))
{
  _Ptr<unsigned int> p_pid = 0;
  _Unchecked {
    p_pid = _Assume_bounds_cast<_Ptr<unsigned int>>(p_key);
  }
  return (*p_pid) % buckets;
}

static unsigned int
handle_ip_count(_Array_ptr<void> p_ipaddr : byte_count(s_p_ip_count_hash->key_size))
{
  _Array_ptr<void> result : byte_count(s_p_ip_count_hash->value_size) = hash_lookup_entry(s_p_ip_count_hash, p_ipaddr);
  _Ptr<unsigned int> p_count = _Dynamic_bounds_cast<_Ptr<unsigned int>>(result);
  unsigned int count;
  if (!p_count)
  {
    count = 1;
    _Array_ptr<void> value : byte_count(s_p_ip_count_hash->value_size) = _Dynamic_bounds_cast<_Array_ptr<void>>(&count, byte_count(s_p_ip_count_hash->value_size));
    hash_add_entry(s_p_ip_count_hash, p_ipaddr, value);
  }
  else
  {
    count = *p_count;
    count++;
    *p_count = count;
  }
  return count;
}

