/*
 * Part of Very Secure FTPd
 * Licence: GPL v2
 * Author: Chris Evans
 * 
 * sysutil.c
 *
 * Routines to make the libc/syscall API more pleasant to use. Long term,
 * more libc/syscalls will go in here to reduce the number of .c files with
 * dependencies on libc or syscalls.
 */

#define PRIVATE_HANDS_OFF_syscall_retval syscall_retval
#define PRIVATE_HANDS_OFF_exit_status exit_status
#include "sysutil.h"
#include "utility.h"
#include "tunables.h"
#include "sysdeputil.h"

/* Activate 64-bit file support on Linux/32bit plus others */
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1
#define _LARGE_FILES 1

/* For Linux, this adds nothing :-) */
#include "port/porting_junk.h"

#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <dirent.h>
#include <time.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/time.h>
/* Must be before netinet/ip.h. Found on FreeBSD, Solaris */
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <limits.h>
#include <syslog.h>
#include <utime.h>
#include <netdb.h>
#include <sys/resource.h>

#include "hacks.h"

#pragma CHECKED_SCOPE on

/* Private variables to this file */
/* Current umask() */
static unsigned int s_current_umask;
/* Cached time */
static struct timeval s_current_time;
/* Current pid */
static int s_current_pid = -1;
/* Exit function */
static exitfunc_t s_exit_func = ((void *)0);
/* Difference in timezone from GMT in seconds */
static long s_timezone;

/* Our internal signal handling implementation details */
static struct vsf_sysutil_sig_details
{
  vsf_sighandle_t sync_sig_handler;
  _Ptr<void> p_private;
  volatile sig_atomic_t pending;
  int running;
  int use_alarm;
};
static struct vsf_sysutil_sig_details s_sig_details _Checked[65];


static vsf_context_io_t s_io_handler = ((void *)0);
static _Ptr<void> s_p_io_handler_private;
static int s_io_handler_running;

struct vsf_sysutil_sockaddr
{
  union
  {
    struct sockaddr u_sockaddr;
    struct sockaddr_in u_sockaddr_in;
    struct sockaddr_in6 u_sockaddr_in6;
  } u;
};

/* File locals */
static void vsf_sysutil_common_sighandler(int signum);
static void vsf_sysutil_alrm_sighandler(int signum);
static int vsf_sysutil_translate_sig(const enum EVSFSysUtilSignal sig);
static void vsf_sysutil_set_sighandler(int sig, _Ptr<void (int)> p_handlefunc);
static int vsf_sysutil_translate_memprot(
  const enum EVSFSysUtilMapPermission perm);
static int vsf_sysutil_translate_openmode(
  const enum EVSFSysUtilOpenMode mode);
static void vsf_sysutil_alloc_statbuf(_Ptr<_Ptr<struct vsf_sysutil_statbuf>> p_ptr);
void vsf_sysutil_sockaddr_alloc(_Ptr<_Ptr<struct vsf_sysutil_sockaddr>> p_sockptr);
static int lock_internal(int fd, int lock_type);

static void
vsf_sysutil_alrm_sighandler(int signum)
{
  (void) signum;
  alarm(1);
}

static void
vsf_sysutil_common_sighandler(int signum)
{
  if (signum < 0 || signum >= NSIG)
  {
    /* "cannot happen" */
    return;
  }
  if (s_sig_details[signum].sync_sig_handler)
  {
    s_sig_details[signum].pending = 1;
    /* Since this synchronous signal framework has a small race (signal coming
     * in just before we start a blocking call), there's the option to fire an
     * alarm repeatedly until the signal is handled.
     */
    if (s_sig_details[signum].use_alarm)
    {
      alarm(1);
    }
  }
}

/* Notes. This signal check is evaluated after potentially blocking system
 * calls, i.e. at highly defined points in the code. Since we only interrupt
 * at these definite locations, the signal handlers can be non-trivial
 * without us having to worry about re-entrancy.
 *
 * We guarantee that a handler for a given signal is not re-entrant. This
 * is taken care of by the "running" flag.
 *
 * This call itself can only be re-entered once we dereference the actual
 * hander function pointer, so we are safe with respect to races modifying
 * the "running" flag.
 */
void
vsf_sysutil_check_pending_actions(
  const enum EVSFSysUtilInterruptContext context, int retval, int fd)
{
  unsigned int i;
  /* Check the i/o handler before the signal handlers */
  if (s_io_handler && !s_io_handler_running && context == kVSFSysUtilIO)
  {
    s_io_handler_running = 1;
    (*s_io_handler)(retval, fd, s_p_io_handler_private);
    s_io_handler_running = 0;
  }
  for (i=0; i < NSIG; ++i)
  {
    if (s_sig_details[i].pending && !s_sig_details[i].running)
    {
      s_sig_details[i].running = 1;
      if (s_sig_details[i].use_alarm)
      {
        alarm(0);
      }
      if (s_sig_details[i].sync_sig_handler)
      {
        s_sig_details[i].pending = 0;
        (*(s_sig_details[i].sync_sig_handler))(s_sig_details[i].p_private);
      }
      s_sig_details[i].running = 0;
    }
  }
}

static int
vsf_sysutil_translate_sig(const enum EVSFSysUtilSignal sig)
{
  int realsig = 0;
  switch (sig)
  {
    case kVSFSysUtilSigALRM:
      realsig = SIGALRM;
      break;
    case kVSFSysUtilSigTERM:
      realsig = SIGTERM;
      break;
    case kVSFSysUtilSigCHLD:
      realsig = SIGCHLD;
      break;
    case kVSFSysUtilSigPIPE:
      realsig = SIGPIPE;
      break;
    case kVSFSysUtilSigURG:
      realsig = SIGURG;
      break;
    case kVSFSysUtilSigHUP:
      realsig = SIGHUP;
      break;
    default:
      bug("unknown signal in vsf_sysutil_translate_sig");
      break;
  }
  if (realsig < 0 || realsig >= NSIG)
  {
    bug("signal out of range in vsf_sysutil_translate_sig");
  }
  return realsig;
}

void
vsf_sysutil_install_sighandler(const enum EVSFSysUtilSignal sig, vsf_sighandle_t handler, _Ptr<void> p_private, int use_alarm)
{
  int realsig = vsf_sysutil_translate_sig(sig);
  s_sig_details[realsig].p_private = p_private;
  s_sig_details[realsig].sync_sig_handler = handler;
  s_sig_details[realsig].use_alarm = use_alarm;
  vsf_sysutil_set_sighandler(realsig, vsf_sysutil_common_sighandler);
  if (use_alarm && realsig != SIGALRM)
  {
    vsf_sysutil_set_sighandler(SIGALRM, vsf_sysutil_alrm_sighandler);
  }
}

void
vsf_sysutil_default_sig(const enum EVSFSysUtilSignal sig)
{
  int realsig = vsf_sysutil_translate_sig(sig);
  vsf_sysutil_set_sighandler(realsig, 0);
  s_sig_details[realsig].p_private = NULL;
  s_sig_details[realsig].sync_sig_handler = NULL;
}

void
vsf_sysutil_install_null_sighandler(const enum EVSFSysUtilSignal sig)
{
  int realsig = vsf_sysutil_translate_sig(sig);
  s_sig_details[realsig].p_private = NULL;
  s_sig_details[realsig].sync_sig_handler = NULL;
  vsf_sysutil_set_sighandler(realsig, vsf_sysutil_common_sighandler);
}

void
vsf_sysutil_install_async_sighandler(const enum EVSFSysUtilSignal sig,
                                     vsf_async_sighandle_t handler)
{
  int realsig = vsf_sysutil_translate_sig(sig);
  s_sig_details[realsig].p_private = NULL;
  s_sig_details[realsig].sync_sig_handler = NULL;
  vsf_sysutil_block_sig(sig);
  vsf_sysutil_set_sighandler(realsig, handler);
}

static void
vsf_sysutil_set_sighandler(int sig, _Ptr<void (int)> p_handlefunc)
{
  int retval;
  struct sigaction sigact;
  vsf_sysutil_memclr<struct sigaction>(&sigact, sizeof(sigact));
  _Unchecked {
    sigact.sa_handler = (void (*)(int)) p_handlefunc;
    retval = sigfillset(&sigact.sa_mask);
  }
  if (retval != 0)
  {
    die("sigfillset");
  }
  _Unchecked {
    retval = sigaction(sig, &sigact, NULL);
  }
  if (retval != 0)
  {
    die("sigaction");
  }
}

void
vsf_sysutil_block_sig(const enum EVSFSysUtilSignal sig)
{
  sigset_t sset;
  int retval;
  int realsig = vsf_sysutil_translate_sig(sig);
  _Unchecked {
    retval = sigemptyset(&sset);
  }
  if (retval != 0)
  {
    die("sigemptyset");
  }
  _Unchecked {
    retval = sigaddset(&sset, realsig);
  }
  if (retval != 0)
  {
    die("sigaddset");
  }
  _Unchecked {
    retval = sigprocmask(SIG_BLOCK, &sset, NULL);
  }
  if (retval != 0)
  {
    die("sigprocmask");
  }
}

void
vsf_sysutil_unblock_sig(const enum EVSFSysUtilSignal sig)
{
  sigset_t sset;
  int retval;
  int realsig = vsf_sysutil_translate_sig(sig);
  _Unchecked {
    retval = sigemptyset(&sset);
  }
  if (retval != 0)
  {
    die("sigemptyset");
  }
  _Unchecked {
    retval = sigaddset(&sset, realsig);
  }
  if (retval != 0)
  {
    die("sigaddset");
  }
  _Unchecked {
    retval = sigprocmask(SIG_UNBLOCK, &sset, NULL);
  }
  if (retval != 0)
  {
    die("sigprocmask");
  }
}
void
vsf_sysutil_install_io_handler(vsf_context_io_t handler, _Ptr<void> p_private)
{
  if (s_io_handler != NULL)
  {
    bug("double register of i/o handler");
  }
  s_io_handler = handler;
  s_p_io_handler_private = p_private;
}

void
vsf_sysutil_uninstall_io_handler(void)
{
  if (s_io_handler == NULL)
  {
    bug("no i/o handler to unregister!");
  }
  s_io_handler = NULL;
  s_p_io_handler_private = NULL;
}

void
vsf_sysutil_set_alarm(const unsigned int trigger_seconds)
{
  (void) alarm(trigger_seconds);
}

void
vsf_sysutil_clear_alarm(void)
{
  vsf_sysutil_set_alarm(0);
}

int
vsf_sysutil_read(const int fd, void* p_buf, const unsigned int size)
{
  while (1)
  {
    int retval = 0;
    _Unchecked {
      retval = read(fd, p_buf, size);
    }
    int saved_errno = errno;
    vsf_sysutil_check_pending_actions(kVSFSysUtilIO, retval, fd);
    if (retval < 0 && saved_errno == EINTR)
    {
      continue;
    }
    return retval;
  }
}

int
vsf_sysutil_write(const int fd, const void* p_buf, const unsigned int size)
{
  while (1)
  {
    int retval = 0;
    _Unchecked {
      retval = write(fd, p_buf, size);
    }
    int saved_errno = errno;
    vsf_sysutil_check_pending_actions(kVSFSysUtilIO, retval, fd);
    if (retval < 0 && saved_errno == EINTR)
    {
      continue;
    }
    return retval;
  }
}

int
vsf_sysutil_read_loop(const int fd, void* p_buf, unsigned int size)
{
  _Array_ptr<char> p_char_buf : byte_count(size) = 0;
  _Unchecked {
    p_char_buf = _Assume_bounds_cast<_Array_ptr<char>>(p_buf, byte_count(size));
  }
  int retval;
  int num_read = 0;
  if (size > INT_MAX)
  {
    die("size too big in vsf_sysutil_read_loop");
  }
  unsigned int remaining = size;
  while (1)
  {
    retval = vsf_sysutil_read<char>(fd, p_char_buf + num_read, remaining);
    if (retval < 0)
    {
      return retval;
    }
    else if (retval == 0)
    {
      /* Read all we're going to read.. */
      return num_read; 
    }
    if ((unsigned int) retval > remaining)
    {
      die("retval too big in vsf_sysutil_read_loop");
    }
    num_read += retval;
    remaining -= (unsigned int) retval;
    if (remaining == 0)
    {
      /* Hit the read target, cool. */
      return num_read;
    }
  }
}

int
vsf_sysutil_write_loop(const int fd, const void* p_buf, unsigned int size)
{
  _Array_ptr<char> p_char_buf : byte_count(size) = 0;
  _Unchecked {
    p_char_buf = _Assume_bounds_cast<_Array_ptr<char>>(p_buf, byte_count(size));
  }
  int retval;
  int num_written = 0;
  if (size > INT_MAX)
  {
    die("size too big in vsf_sysutil_write_loop");
  }
  unsigned int remaining = size;
  while (1)
  {
    retval = vsf_sysutil_write<const char>(fd, p_char_buf + num_written, remaining);
    if (retval < 0)
    {
      /* Error */
      return retval;
    }
    else if (retval == 0)
    {
      /* Written all we're going to write.. */
      return num_written;
    }
    if ((unsigned int) retval > remaining)
    {
      die("retval too big in vsf_sysutil_write_loop");
    }
    num_written += retval;
    remaining -= (unsigned int) retval;
    if (size == 0)
    {
      /* Hit the write target, cool. */
      return num_written;
    }
  }
}

filesize_t
vsf_sysutil_get_file_offset(const int file_fd)
{
  filesize_t retval = lseek(file_fd, 0, SEEK_CUR);
  if (retval < 0)
  {
    die("lseek");
  }
  return retval;
}

void
vsf_sysutil_lseek_to(const int fd, filesize_t seek_pos)
{
  filesize_t retval;
  if (seek_pos < 0)
  {
    die("negative seek_pos in vsf_sysutil_lseek_to");
  }
  retval = lseek(fd, seek_pos, SEEK_SET);
  if (retval < 0)
  {
    die("lseek");
  }
}

void
vsf_sysutil_lseek_end(const int fd)
{
  filesize_t retval;
  retval = lseek(fd, 0, SEEK_END);
  if (retval < 0)
  {
    die("lseek");
  }
}

_Itype_for_any(T) void*
vsf_sysutil_malloc(unsigned int size) : itype(_Array_ptr<T>) byte_count(size)
{
  _Ptr<T> p_ret = 0;
  /* Paranoia - what if we got an integer overflow/underflow? */
  if (size == 0 || size > INT_MAX)
  {
    bug("zero or big size in vsf_sysutil_malloc");
  }  
  p_ret = malloc<T>(size);
  if (p_ret == NULL)
  {
    die("malloc");
  }
  return p_ret;
}

_Itype_for_any(T) void*
vsf_sysutil_realloc(void* p_ptr : itype(_Array_ptr<T>) byte_count(1), unsigned int size) : itype(_Array_ptr<T>) byte_count(size)
{
  _Ptr<T> p_ret = 0;
  if (size == 0 || size > INT_MAX)
  {
    bug("zero or big size in vsf_sysutil_realloc");
  }
  p_ret = realloc<T>(p_ptr, size);
  if (p_ret == NULL)
  {
    die("realloc");
  }
  return p_ret;
}

void
vsf_sysutil_free(void* p_ptr)
{
  if (p_ptr == NULL)
  {
    bug("vsf_sysutil_free got a null pointer");
  }
  free<void>(p_ptr);
}

unsigned int
vsf_sysutil_getpid(void)
{
  if (s_current_pid == -1)
  {
    s_current_pid = vsf_sysutil_getpid_nocache();
  }
  return (unsigned int) s_current_pid;
}

int
vsf_sysutil_fork(void)
{
  int retval = vsf_sysutil_fork_failok();
  if (retval < 0)
  {
    die("fork");
  }
  return retval;
}

int
vsf_sysutil_fork_failok(void)
{
  int retval;
  retval = fork();
  if (retval == 0)
  {
    vsf_sysutil_post_fork();
  }
  return retval;
}

void
vsf_sysutil_set_exit_func(exitfunc_t exitfunc)
{
  s_exit_func = exitfunc;
}

void
vsf_sysutil_exit(int exit_code)
{
  if (s_exit_func)
  {
    exitfunc_t curr_func = s_exit_func;
    /* Prevent recursion */
    s_exit_func = 0;
    (*curr_func)();
  }
  _exit(exit_code);
}

struct vsf_sysutil_wait_retval
vsf_sysutil_wait(void)
{
  struct vsf_sysutil_wait_retval retval;
  vsf_sysutil_memclr<struct vsf_sysutil_wait_retval>(&retval, sizeof(retval));
  while (1)
  {
    int sys_ret = 0;
    _Unchecked {
      wait(&retval.exit_status);
    }
    if (sys_ret < 0 && errno == EINTR)
    {
      vsf_sysutil_check_pending_actions(kVSFSysUtilUnknown, 0, 0);
      continue;
    }
    retval.syscall_retval = sys_ret;
    return retval;
  }
}

int
vsf_sysutil_wait_reap_one(void)
{
  int retval = waitpid(-1, NULL, WNOHANG);
  if (retval == 0 || (retval < 0 && errno == ECHILD))
  {
    /* No more children */
    return 0;
  }
  if (retval < 0)
  {
    die("waitpid");
  }
  /* Got one */
  return retval;
}

int
vsf_sysutil_wait_get_retval(_Ptr<const struct vsf_sysutil_wait_retval> p_waitret)
{
  return p_waitret->syscall_retval;
}

int
vsf_sysutil_wait_exited_normally(_Ptr<const struct vsf_sysutil_wait_retval> p_waitret)
{
  int status =  p_waitret->exit_status;
  return WIFEXITED(status);
}

int
vsf_sysutil_wait_get_exitcode(_Ptr<const struct vsf_sysutil_wait_retval> p_waitret)
{
  int status;
  if (!vsf_sysutil_wait_exited_normally(p_waitret))
  {
    bug("not a normal exit in vsf_sysutil_wait_get_exitcode");
  }
  status =  p_waitret->exit_status;
  return WEXITSTATUS(status);
}

void
vsf_sysutil_activate_keepalive(int fd)
{
  int keepalive = 1;
  int retval = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive,
                          sizeof(keepalive));
  if (retval != 0)
  {
    die("setsockopt: keepalive");
  }
}

void
vsf_sysutil_activate_reuseaddr(int fd)
{
  int reuseaddr = 1;
  int retval = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr,
                          sizeof(reuseaddr));
  if (retval != 0)
  {
    die("setsockopt: reuseaddr");
  }
}

void
vsf_sysutil_set_nodelay(int fd)
{
  int nodelay = 1;
  int retval = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay,
                          sizeof(nodelay));
  if (retval != 0)
  {
    die("setsockopt: nodelay");
  }
}

void
vsf_sysutil_activate_sigurg(int fd)
{
  int retval = 0;
  _Unchecked {
    retval = fcntl(fd, F_SETOWN, vsf_sysutil_getpid());
  }
  if (retval != 0)
  {
    die("fcntl");
  }
}

void
vsf_sysutil_activate_oobinline(int fd)
{
  int oob_inline = 1;
  int retval = setsockopt(fd, SOL_SOCKET, SO_OOBINLINE, &oob_inline,
                          sizeof(oob_inline));
  if (retval != 0)
  {
    die("setsockopt: oobinline");
  }
}

void
vsf_sysutil_set_iptos_throughput(int fd)
{
  int tos = IPTOS_THROUGHPUT;
  /* Ignore failure to set (maybe this IP stack demands privilege for this) */
  (void) setsockopt(fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));
}

void
vsf_sysutil_activate_linger(int fd)
{
  int retval;
  struct linger the_linger;
  vsf_sysutil_memclr<struct linger>(&the_linger, sizeof(the_linger));
  the_linger.l_onoff = 1;
  the_linger.l_linger = 60 * 10;
  retval = setsockopt(fd, SOL_SOCKET, SO_LINGER, &the_linger,
                      sizeof(the_linger));
  if (retval != 0)
  {
    die("setsockopt: linger");
  }
}

void
vsf_sysutil_deactivate_linger_failok(int fd)
{
  struct linger the_linger;
  the_linger.l_onoff = 0;
  the_linger.l_linger = 0;
  (void) setsockopt(fd, SOL_SOCKET, SO_LINGER, &the_linger, sizeof(the_linger));
}

void
vsf_sysutil_activate_noblock(int fd)
{
  int retval;
  int curr_flags = 0;
  _Unchecked {
    curr_flags = fcntl(fd, F_GETFL);
  }
  if (vsf_sysutil_retval_is_error(curr_flags))
  {
    die("fcntl");
  }
  curr_flags |= O_NONBLOCK;
  _Unchecked {
    retval = fcntl(fd, F_SETFL, curr_flags);
  }
  if (retval != 0)
  {
    die("fcntl");
  }
}

void
vsf_sysutil_deactivate_noblock(int fd)
{
  int retval;
  int curr_flags = 0;
  _Unchecked {
    curr_flags = fcntl(fd, F_GETFL);
  }
  if (vsf_sysutil_retval_is_error(curr_flags))
  {
    die("fcntl");
  }
  curr_flags &= ~O_NONBLOCK;
  _Unchecked {
    retval = fcntl(fd, F_SETFL, curr_flags);
  }
  if (retval != 0)
  {
    die("fcntl");
  }
}

int
vsf_sysutil_recv_peek(const int fd, void* p_buf, unsigned int len)
{
  while (1)
  {
    int retval = recv(fd, p_buf, len, MSG_PEEK);
    int saved_errno = errno;
    vsf_sysutil_check_pending_actions(kVSFSysUtilIO, retval, fd);
    if (retval < 0 && saved_errno == EINTR)
    {
      continue;
    }
    return retval;
  }
}

int
vsf_sysutil_atoi(_Nt_array_ptr<const char> p_str : count(0))
{
  return atoi(p_str);
}

filesize_t
vsf_sysutil_a_to_filesize_t(_Nt_array_ptr<const char> p_str : count(0))
{
  /* atoll() is C99 standard - but even modern FreeBSD, OpenBSD don't have
   * it, so we'll supply our own
   */
  filesize_t result = 0;
  filesize_t mult = 1;
  unsigned int len = vsf_sysutil_strlen(p_str);
  unsigned int i;
  /* Bail if the number is excessively big (petabytes!) */
  if (len > 15)
  {
    return 0;
  }
  for (i=0; i<len; ++i)
  {
    char the_char = p_str[len-(i+1)];
    filesize_t val;
    if (the_char < '0' || the_char > '9')
    {
      return 0;
    }
    val = the_char - '0';
    val *= mult;
    result += val;
    mult *= 10;
  }
  return result;
}

_Nt_array_ptr<const char> vsf_sysutil_ulong_to_str(unsigned long the_ulong) : count(0)
{
  static char ulong_buf _Nt_checked[32];
  _Unchecked { (void) snprintf(ulong_buf, sizeof(ulong_buf), "%lu", the_ulong) ; }
  return ulong_buf;
}

_Nt_array_ptr<const char> vsf_sysutil_filesize_t_to_str(filesize_t the_filesize) : count(0)
{
  static char filesize_buf _Nt_checked[32];
  if (sizeof(long) == 8)
  {
    /* Avoid using non-standard %ll if we can */
    _Unchecked {(void) snprintf(filesize_buf, sizeof(filesize_buf), "%ld",
                    (long) the_filesize); }
  }
  else
  {
    _Unchecked {(void) snprintf(filesize_buf, sizeof(filesize_buf), "%lld", the_filesize); }
  }
  return filesize_buf;
}

_Nt_array_ptr<const char> vsf_sysutil_double_to_str(double the_double) : count(0)
{
  static char double_buf _Nt_checked[32];
  _Unchecked { (void) snprintf(double_buf, sizeof(double_buf), "%.2f", the_double); }
  return double_buf;
}

_Nt_array_ptr<const char> vsf_sysutil_uint_to_octal(unsigned int the_uint) : count(0)
{
  static char octal_buf _Nt_checked[32];
  if (the_uint == 0)
  {
    octal_buf[0] = '0';
    octal_buf[1] = '\0';
  }
  else
  {
    _Unchecked {(void) snprintf(octal_buf, sizeof(octal_buf), "0%o", the_uint);}
  }
  return octal_buf;
}

unsigned int
vsf_sysutil_octal_to_uint(_Nt_array_ptr<const char> p_str)
{
  /* NOTE - avoiding using sscanf() parser */
  unsigned int result = 0;
  int seen_non_zero_digit = 0;
  while (*p_str != '\0')
  {
    int digit = *p_str;
    int is_digit = 0;
    _Unchecked {
      is_digit = isdigit(digit);
    }
    if (!is_digit || digit > '7')
    {
      break;
    }
    if (digit != '0')
    {
      seen_non_zero_digit = 1;
    }
    if (seen_non_zero_digit)
    {
      result <<= 3;
      result += (digit - '0');
    }
    p_str++;
  }
  return result;
}

int
vsf_sysutil_toupper(int the_char)
{
  return toupper((unsigned char) the_char);
}

int
vsf_sysutil_isspace(int the_char) _Unchecked 
{
  return isspace((unsigned char) the_char);
}

int
vsf_sysutil_isprint(int the_char)
{
  /* From Solar - we know better than some libc's! Don't let any potential
   * control chars through
   */
  unsigned char uc = (unsigned char) the_char;
  if (uc <= 31)
  {
    return 0;
  }
  if (uc == 177)
  {
    return 0;
  }
  if (uc >= 128 && uc <= 159)
  {
    return 0;
  }
  _Unchecked {
    return isprint(the_char);
  }
}

int
vsf_sysutil_isalnum(int the_char) _Unchecked 
{
  return isalnum((unsigned char) the_char);
}

int
vsf_sysutil_isdigit(int the_char) _Unchecked 
{
  return isdigit((unsigned char) the_char);
}

_Nt_array_ptr<char> vsf_sysutil_getcwd(_Array_ptr<char> p_dest : count(buf_size), const unsigned int buf_size)
{
  if (buf_size == 0) {
    return 0;
  }
  _Unchecked {
    getcwd((char*) p_dest, buf_size);
  }
  p_dest[buf_size - 1] = '\0';
  _Unchecked {
    return (_Nt_array_ptr<char>) p_dest;
  }
}

int
vsf_sysutil_mkdir(_Nt_array_ptr<const char> p_dirname, const unsigned int mode)
{
  return mkdir(p_dirname, mode);
}

int
vsf_sysutil_rmdir(_Nt_array_ptr<const char> p_dirname) _Unchecked
{
  return rmdir((char*) p_dirname);
}

int
vsf_sysutil_chdir(_Nt_array_ptr<const char> p_dirname) _Unchecked
{
  return chdir((char*) p_dirname);
}

int
vsf_sysutil_rename(_Nt_array_ptr<const char> p_from, _Nt_array_ptr<const char> p_to)
{
  return rename(p_from, p_to);
}

char *vsf_sysutil_realpath(const char *path : itype(_Nt_array_ptr<const char>), int may_be_fresh) : itype(_Nt_array_ptr<char>)
{
  { /* existing paths must resolve right away */
    _Nt_array_ptr<const char> resolved = 0;
    _Unchecked {
      resolved = _Assume_bounds_cast<_Nt_array_ptr<char>>(realpath(path, NULL), count(0));
    }
    if ((resolved != NULL) || (errno != ENOENT) || !may_be_fresh)
    {
      return  resolved;
    }
  }

  { /* try to resolve directory part */
    _Nt_array_ptr<char const> filename = strrchr(path, '/');
    _Nt_array_ptr<char const> resolved_dir = 0;
    if(filename == NULL)
    {
      filename     = path;
      _Unchecked {
        resolved_dir = _Assume_bounds_cast<_Nt_array_ptr<char>>(realpath(".", NULL), count(0));
      }
    }
    else
    {
      _Nt_array_ptr<char const> original_dir = 0;
      _Unchecked {
        strndup(path, filename-path);
      }
      filename++;
      _Unchecked {
        resolved_dir = _Assume_bounds_cast<_Nt_array_ptr<char>>(realpath((const char*) original_dir, NULL), count(0));
      }
      free<const char>(original_dir);
    }
    if(resolved_dir == NULL)  return  NULL;

    /* compose path from resolved directory and filename */
    size_t  dir_len = strlen(resolved_dir);
    /* empty root as slash is added anyways */
    if (dir_len == 1)  dir_len = 0;
    unchecked_widen_bounds(_Nt_array_ptr<const char>, resolved_dir_len, count(dir_len), resolved_dir);

    size_t file_len = strlen(filename);
    unchecked_widen_bounds(_Nt_array_ptr<const char>, filename_len, count(file_len), filename);

    size_t buf_len = dir_len+file_len+2;
    _Nt_array_ptr<char> resolved : count(buf_len - 1) = 0;

    _Array_ptr<char> tmp : count(buf_len) = calloc<char>(buf_len, sizeof(char));
    _Unchecked {
      resolved = (_Nt_array_ptr<char>) tmp;
    }

    _Nt_array_ptr<char> tmp2 : count(dir_len) = _Dynamic_bounds_cast<_Nt_array_ptr<char>>(resolved, count(dir_len));
    strncpy(tmp2, resolved_dir_len, dir_len);
    free<char>(resolved_dir);

    resolved[dir_len] = '/';
    _Nt_array_ptr<char> tmp3 : count(file_len) = _Dynamic_bounds_cast<_Nt_array_ptr<char>>(resolved+dir_len+1, count(file_len));
    strncpy(tmp3, filename_len, file_len);
    return  resolved;
  }
}

struct vsf_sysutil_dir *vsf_sysutil_opendir(const char *p_dirname : itype(_Nt_array_ptr<const char>)) : itype(_Ptr<struct vsf_sysutil_dir>) _Unchecked
{
  return (struct vsf_sysutil_dir*) opendir(p_dirname);
}

void
vsf_sysutil_closedir(struct vsf_sysutil_dir *p_dir : itype(_Ptr<struct vsf_sysutil_dir>))
{
  int retval = 0;
  _Unchecked {
    retval = closedir((DIR*) p_dir);
  }
  if (retval != 0)
  {
    die("closedir");
  }
}

const char *vsf_sysutil_next_dirent(struct vsf_sysutil_dir *p_dir : itype(_Ptr<struct vsf_sysutil_dir>)) : itype(_Nt_array_ptr<const char>) count(0)
{
  _Ptr<struct dirent> p_dirent = 0;
  _Unchecked {
    p_dirent = _Assume_bounds_cast<_Ptr<struct dirent>>(readdir((DIR*) p_dir));
  }
  if (p_dirent == NULL)
  {
    return NULL;
  }
  _Unchecked {
    return p_dirent->d_name;
  }
}

unsigned int
vsf_sysutil_strlen(_Nt_array_ptr<const char> p_text : count(0))
{
  size_t ret = strlen(p_text);
  /* A defense in depth measure. */
  if (ret > INT_MAX / 8)
  {
    die("string suspiciously long");
  }
  return (unsigned int) ret;
}

_Nt_array_ptr<char> vsf_sysutil_strdup(_Nt_array_ptr<const char> p_str : count(0))
{
  return ((_Nt_array_ptr<char> )strdup(p_str));
}

char *vsf_sysutil_strndup(_Nt_array_ptr<const char> p_str : byte_count(p_len), unsigned int p_len) : itype(_Nt_array_ptr<char>) byte_count(p_len)
{
  _Array_ptr<char> new : byte_count(p_len + 1) = calloc<char>(p_len+1, sizeof(char));

  if (new == NULL)
    return NULL;

  new[p_len]='\0';
  
  memcpy<char>(new, p_str, p_len);

  _Nt_array_ptr<char> tmp_ret : count(p_len) = 0;
  _Unchecked {
   tmp_ret = (_Nt_array_ptr<char>) new;
  }
  return tmp_ret;
}

void
vsf_sysutil_memclr(void* p_dest, unsigned int size)
{
  /* Safety */
  if (size == 0)
  {
    return;
  }
  memset(p_dest, '\0', size);
}

void
vsf_sysutil_memcpy(void* p_dest, const void* p_src, const unsigned int size)
{
  /* Safety */
  if (size == 0)
  {
    return;
  }
  /* Defense in depth */
  if (size > INT_MAX)
  {
    die("possible negative value to memcpy?");
  }
  memcpy<void>(p_dest, p_src, size);
}

void
vsf_sysutil_strcpy(_Array_ptr<char> p_dest : count(maxsize), _Array_ptr<const char> p_src : count(maxsize), unsigned int maxsize)
{
  if (maxsize == 0)
  {
    return;
  }
  strncpy(p_dest, p_src, maxsize);
  p_dest[maxsize - 1] = '\0';
}

int
vsf_sysutil_memcmp(const void* p_src1, const void* p_src2, unsigned int size)
{
  /* Safety */
  if (size == 0)
  {
    return 0;
  }
  return memcmp(p_src1, p_src2, size);
}

int
vsf_sysutil_strcmp(_Nt_array_ptr<const char> p_src1, _Nt_array_ptr<const char> p_src2)
{
  return strcmp(p_src1, p_src2);
}

unsigned int
vsf_sysutil_getpagesize(void)
{
  static unsigned int s_page_size;
  if (s_page_size == 0)
  {
    s_page_size = getpagesize();
    if (s_page_size == 0)
    {
      die("getpagesize");
    }
  }
  return s_page_size;
}

static int
vsf_sysutil_translate_memprot(const enum EVSFSysUtilMapPermission perm)
{
  int retval = 0;
  switch (perm)
  {
    case kVSFSysUtilMapProtReadOnly:
      retval = PROT_READ;
      break;
    case kVSFSysUtilMapProtNone:
      retval = PROT_NONE;
      break;
    default:
      bug("bad value in vsf_sysutil_translate_memprot");
      break;
  }
  return retval;
}

void
vsf_sysutil_memprotect(void* p_addr, unsigned int len,
                       const enum EVSFSysUtilMapPermission perm)
{
  int prot = vsf_sysutil_translate_memprot(perm);
  int retval = 0; 
  _Unchecked {
    retval = mprotect(p_addr, len, prot);
  }
  if (retval != 0)
  {
    die("mprotect");
  }
}

void
vsf_sysutil_memunmap(void* p_start, unsigned int length)
{
  int retval = 0;
  _Unchecked {
    retval = munmap(p_start, length);
  }
  if (retval != 0)
  {
    die("munmap");
  }
}

static int
vsf_sysutil_translate_openmode(const enum EVSFSysUtilOpenMode mode)
{
  int retval = 0;
  switch (mode)
  {
    case kVSFSysUtilOpenReadOnly:
      retval = O_RDONLY;
      break;
    case kVSFSysUtilOpenWriteOnly:
      retval = O_WRONLY;
      break;
    case kVSFSysUtilOpenReadWrite:
      retval = O_RDWR;
      break;
    default:
      bug("bad mode in vsf_sysutil_translate_openmode");
      break;
  }
  return retval;
}

int
vsf_sysutil_open_file(const char *p_filename : itype(_Nt_array_ptr<const char>), const enum EVSFSysUtilOpenMode mode) _Unchecked
{
  return open(p_filename, vsf_sysutil_translate_openmode(mode) | O_NONBLOCK);
}

int
vsf_sysutil_create_file_exclusive(const char *p_filename : itype(_Nt_array_ptr<const char>)) _Unchecked
{
  /* umask() also contributes to end mode */
  return open(p_filename, O_CREAT | O_EXCL | O_WRONLY | O_APPEND,
              tunable_file_open_mode);
}

int
vsf_sysutil_create_or_open_file(const char *p_filename : itype(_Nt_array_ptr<const char>), unsigned int mode) _Unchecked
{
  return open(p_filename, O_CREAT | O_WRONLY | O_NONBLOCK, mode);
}

int
vsf_sysutil_create_or_open_file_append(const char *p_filename : itype(_Nt_array_ptr<const char>), unsigned int mode) _Unchecked
{
  return open(p_filename, O_CREAT | O_WRONLY | O_NONBLOCK | O_APPEND, mode);
}

void
vsf_sysutil_dupfd2(int old_fd, int new_fd)
{
  int retval;
  if (old_fd == new_fd)
  {
    return;
  }
  retval = dup2(old_fd, new_fd);
  if (retval != new_fd)
  {
    die("dup2");
  }
}

void
vsf_sysutil_close(int fd)
{
  while (1)
  {
    int retval = close(fd);
    if (retval != 0)
    {
      if (errno == EINTR)
      {
        vsf_sysutil_check_pending_actions(kVSFSysUtilUnknown, 0, 0);
        continue;
      }
      die("close");
    }
    return;
  }
}

int
vsf_sysutil_close_failok(int fd)
{
  return close(fd);
}

int
vsf_sysutil_unlink(const char *p_dead : itype(_Nt_array_ptr<const char>)) _Unchecked
{
  return unlink(p_dead);
}

int
vsf_sysutil_write_access(_Nt_array_ptr<const char> p_filename : count(0)) _Unchecked
{
  int retval = access((char*) p_filename, W_OK);
  return (retval == 0);
}

static void
vsf_sysutil_alloc_statbuf(_Ptr<_Ptr<struct vsf_sysutil_statbuf>> p_ptr)
{
  if (*p_ptr == NULL)
  {
    *p_ptr = vsf_sysutil_malloc<struct vsf_sysutil_statbuf>(sizeof(struct stat));
  }
}

void
vsf_sysutil_fstat(int fd, _Ptr<_Ptr<struct vsf_sysutil_statbuf>> p_ptr)
{
  int retval;
  vsf_sysutil_alloc_statbuf(p_ptr);
  _Unchecked {
    retval = fstat(fd, (struct stat*) (*p_ptr));
  }
  if (retval != 0)
  {
    die("fstat");
  }
}

int
vsf_sysutil_stat(_Nt_array_ptr<const char> p_name : count(0), _Ptr<_Ptr<struct vsf_sysutil_statbuf>> p_ptr)
{
  vsf_sysutil_alloc_statbuf(p_ptr);
  _Unchecked {
    return stat(p_name, (struct stat*) (*p_ptr));
  }
}

int
vsf_sysutil_lstat(_Nt_array_ptr<const char> p_name, _Ptr<_Ptr<struct vsf_sysutil_statbuf>> p_ptr )
{
  vsf_sysutil_alloc_statbuf(p_ptr);
  _Unchecked {
    return lstat(p_name, (struct stat*) (*p_ptr));
  }
}

void
vsf_sysutil_dir_stat(_Ptr<const struct vsf_sysutil_dir> p_dir, _Ptr<_Ptr<struct vsf_sysutil_statbuf>> p_ptr)
{
  int fd = 0;
  _Unchecked {
   fd = dirfd((DIR*) p_dir);
  }
  vsf_sysutil_fstat(fd, p_ptr);
}

int
vsf_sysutil_statbuf_is_regfile(const struct vsf_sysutil_statbuf *p_stat : itype(_Ptr<const struct vsf_sysutil_statbuf>)) _Unchecked
{
  const struct stat* p_realstat = (const struct stat*) p_stat;
  return S_ISREG(p_realstat->st_mode);
}

int
vsf_sysutil_statbuf_is_symlink(const struct vsf_sysutil_statbuf *p_stat : itype(_Ptr<const struct vsf_sysutil_statbuf>)) _Unchecked
{
  const struct stat* p_realstat = (const struct stat*) p_stat;
  return S_ISLNK(p_realstat->st_mode);
}

int
vsf_sysutil_statbuf_is_socket(const struct vsf_sysutil_statbuf *p_stat : itype(_Ptr<const struct vsf_sysutil_statbuf>)) _Unchecked
{
  const struct stat* p_realstat = (const struct stat*) p_stat;
  return S_ISSOCK(p_realstat->st_mode);
}

int
vsf_sysutil_statbuf_is_dir(const struct vsf_sysutil_statbuf *p_stat : itype(_Ptr<const struct vsf_sysutil_statbuf>)) _Unchecked
{
  const struct stat* p_realstat = (const struct stat*) p_stat;
  return S_ISDIR(p_realstat->st_mode);
}

_Nt_array_ptr<const char> vsf_sysutil_statbuf_get_perms(const struct vsf_sysutil_statbuf *p_statbuf : itype(_Ptr<const struct vsf_sysutil_statbuf>)) : count(0)
{
  static char perms _Nt_checked[11];
  int i;
  _Ptr<const struct stat> p_stat = 0;
  _Unchecked {
    p_stat = _Assume_bounds_cast<_Ptr<const struct stat>>(p_statbuf);
  }
  for (i=0; i<10; i++)
  {
    perms[i] = '-';
  }
  perms[0] = '?';
  switch (p_stat->st_mode & S_IFMT)
  {
    case S_IFREG: perms[0] = '-'; break;
    case S_IFDIR: perms[0] = 'd'; break;
    case S_IFLNK: perms[0] = 'l'; break;
    case S_IFIFO: perms[0] = 'p'; break;
    case S_IFSOCK: perms[0] = 's'; break;
    case S_IFCHR: perms[0] = 'c'; break;
    case S_IFBLK: perms[0] = 'b'; break;
    default: break;
  }
  if (p_stat->st_mode & S_IRUSR) perms[1] = 'r';
  if (p_stat->st_mode & S_IWUSR) perms[2] = 'w';
  if (p_stat->st_mode & S_IXUSR) perms[3] = 'x';
  if (p_stat->st_mode & S_IRGRP) perms[4] = 'r';
  if (p_stat->st_mode & S_IWGRP) perms[5] = 'w';
  if (p_stat->st_mode & S_IXGRP) perms[6] = 'x';
  if (p_stat->st_mode & S_IROTH) perms[7] = 'r';
  if (p_stat->st_mode & S_IWOTH) perms[8] = 'w';
  if (p_stat->st_mode & S_IXOTH) perms[9] = 'x';
  if (p_stat->st_mode & S_ISUID) perms[3] = (perms[3] == 'x') ? 's' : 'S';
  if (p_stat->st_mode & S_ISGID) perms[6] = (perms[6] == 'x') ? 's' : 'S';
  if (p_stat->st_mode & S_ISVTX) perms[9] = (perms[9] == 'x') ? 't' : 'T';
  perms[10] = '\0';
  return perms;
}

_Nt_array_ptr<const char> vsf_sysutil_statbuf_get_date(const struct vsf_sysutil_statbuf *p_statbuf : itype(_Ptr<const struct vsf_sysutil_statbuf>), int use_localtime, long curr_time) : count(0)
{
  static char datebuf _Nt_checked[65];
  int retval;
  _Ptr<struct tm> p_tm = ((void *)0);
  _Ptr<const struct stat> p_stat = 0;
  _Unchecked {
    p_stat = _Assume_bounds_cast<_Ptr<const struct stat>>(p_statbuf);
  }

  _Nt_array_ptr<const char> p_date_format = "%b %d %H:%M";
  if (!use_localtime)
  {
    p_tm = gmtime(&p_stat->st_mtime);
  }
  else
  {
    p_tm = localtime(&p_stat->st_mtime);
  }
  /* Is this a future or 6 months old date? If so, we drop to year format */
  if (p_stat->st_mtime > curr_time ||
      (curr_time - p_stat->st_mtime) > 60*60*24*182)
  {
    p_date_format = "%b %d  %Y";
  }
  retval = strftime(datebuf, sizeof(datebuf) - 1, p_date_format, p_tm);
  datebuf[sizeof(datebuf)-1] = '\0';
  if (retval == 0)
  {
    die("strftime");
  }
  return datebuf;
}

_Nt_array_ptr<const char> vsf_sysutil_statbuf_get_numeric_date(const struct vsf_sysutil_statbuf *p_statbuf : itype(_Ptr<const struct vsf_sysutil_statbuf>), int use_localtime) : count(0)
{
  static char datebuf _Nt_checked[16];
  _Ptr<const struct stat> p_stat = 0;
  _Unchecked {
    p_stat = _Assume_bounds_cast<_Ptr<const struct stat>>(p_statbuf);
  }

  _Ptr<struct tm> p_tm = ((void *)0);
  int retval;
  if (!use_localtime)
  {
    p_tm = gmtime(&p_stat->st_mtime);
  }
  else
  {
    p_tm = localtime(&p_stat->st_mtime);
  }
  retval = strftime(datebuf, sizeof(datebuf) - 1, "%Y%m%d%H%M%S", p_tm);
  if (retval == 0)
  {
    die("strftime");
  }
  return datebuf;
}

filesize_t
vsf_sysutil_statbuf_get_size(const struct vsf_sysutil_statbuf *p_statbuf : itype(_Ptr<const struct vsf_sysutil_statbuf>))
{
  _Ptr<const struct stat> p_stat = 0;
  _Unchecked {
    p_stat = _Assume_bounds_cast<_Ptr<const struct stat>>(p_statbuf);
  }

  if (p_stat->st_size < 0)
  {
    die("invalid inode size in vsf_sysutil_statbuf_get_size");
  }
  return p_stat->st_size;
}

int
vsf_sysutil_statbuf_get_uid(const struct vsf_sysutil_statbuf *p_statbuf : itype(_Ptr<const struct vsf_sysutil_statbuf>)) _Unchecked
{
  const struct stat* p_stat = (const struct stat*) p_statbuf;
  return p_stat->st_uid;
}

int
vsf_sysutil_statbuf_get_gid(const struct vsf_sysutil_statbuf *p_statbuf : itype(_Ptr<const struct vsf_sysutil_statbuf>)) _Unchecked
{
  const struct stat* p_stat = (const struct stat*) p_statbuf;
  return p_stat->st_gid;
}

unsigned int
vsf_sysutil_statbuf_get_links(const struct vsf_sysutil_statbuf *p_statbuf : itype(_Ptr<const struct vsf_sysutil_statbuf>)) _Unchecked
{
  const struct stat* p_stat = (const struct stat*) p_statbuf;
  return p_stat->st_nlink;
}

int
vsf_sysutil_statbuf_is_readable_other(const struct vsf_sysutil_statbuf *p_statbuf : itype(_Ptr<const struct vsf_sysutil_statbuf>)) _Unchecked
{
  const struct stat* p_stat = (const struct stat*) p_statbuf;
  if (p_stat->st_mode & S_IROTH)
  {
    return 1;
  }
  return 0;
}

_Nt_array_ptr<const char> vsf_sysutil_statbuf_get_sortkey_mtime(const struct vsf_sysutil_statbuf *p_statbuf : itype(_Ptr<const struct vsf_sysutil_statbuf>)) : count(0) 
{
  static char intbuf _Nt_checked[32];
  _Unchecked {
    const struct stat* p_stat = (const struct stat*) p_statbuf;
    /* This slight hack function must return a character date format such that
     * more recent dates appear later in the alphabet! Most notably, we must
     * make sure we pad to the same length with 0's 
     */
    snprintf(intbuf, sizeof(intbuf), "%030ld", (long) p_stat->st_mtime);
  }
  return intbuf;
}

void
vsf_sysutil_fchown(const int fd, const int uid, const int gid)
{
  if (fchown(fd, uid, gid) != 0)
  {
    die("fchown");
  }
}

void
vsf_sysutil_fchmod(const int fd, unsigned int mode)
{
  mode = mode & 0777;
  if (fchmod(fd, mode))
  {
    die("fchmod");
  }
}

int
vsf_sysutil_chmod(_Nt_array_ptr<const char> p_filename, unsigned int mode)
{
  /* Safety: mask "mode" to just access permissions, e.g. no suid setting! */
  mode = mode & 0777;
  return chmod(p_filename, mode);
}

int
vsf_sysutil_lock_file_write(int fd)
{
  return lock_internal(fd, F_WRLCK);
}

int
vsf_sysutil_lock_file_read(int fd)
{
  return lock_internal(fd, F_RDLCK);
}

static int
lock_internal(int fd, int lock_type)
{
  struct flock the_lock;
  int retval;
  int saved_errno;
  vsf_sysutil_memclr<struct flock>(&the_lock, sizeof(the_lock));
  the_lock.l_type = lock_type;
  the_lock.l_whence = SEEK_SET;
  the_lock.l_start = 0;
  the_lock.l_len = 0;
  do
  {
    _Unchecked {
      retval = fcntl(fd, F_SETLKW, &the_lock);
    }
    saved_errno = errno;
    vsf_sysutil_check_pending_actions(kVSFSysUtilUnknown, 0, 0);
  }
  while (retval < 0 && saved_errno == EINTR);
  return retval;
}

void
vsf_sysutil_unlock_file(int fd)
{
  int retval;
  struct flock the_lock;
  vsf_sysutil_memclr<struct flock>(&the_lock, sizeof(the_lock));
  the_lock.l_type = F_UNLCK;
  the_lock.l_whence = SEEK_SET;
  the_lock.l_start = 0;
  the_lock.l_len = 0;
  _Unchecked {
    retval = fcntl(fd, F_SETLK, &the_lock);
  }
  if (retval != 0)
  {
    die("fcntl");
  }
}

int
vsf_sysutil_readlink(_Nt_array_ptr<const char> p_filename, _Array_ptr<char> p_dest : count(bufsiz), unsigned int bufsiz)
{
  int retval;
  if (bufsiz == 0) {
    return -1;
  }
  _Nt_array_ptr<char> p_dest_nt = 0;
  _Unchecked {
    p_dest_nt = (_Nt_array_ptr<char>) p_dest;
  }
  retval = readlink(p_filename,  p_dest_nt, bufsiz - 1);
  if (retval < 0)
  {
    return retval;
  }
  /* Ensure buffer is NULL terminated; readlink(2) doesn't do that */
  p_dest[retval] = '\0';
  return retval;
}

int
vsf_sysutil_retval_is_error(int retval)
{
  if (retval < 0)
  {
    return 1;
  }
  return 0;
}

enum EVSFSysUtilError
vsf_sysutil_get_error(void)
{
  enum EVSFSysUtilError retval = kVSFSysUtilErrUnknown;
  switch (errno)
  {
    case EADDRINUSE:
      retval = kVSFSysUtilErrADDRINUSE;
      break;
    case ENOSYS:
      retval = kVSFSysUtilErrNOSYS;
      break;
    case EINTR:
      retval = kVSFSysUtilErrINTR;
      break;
    case EINVAL:
      retval = kVSFSysUtilErrINVAL;
      break;
    case EOPNOTSUPP:
      retval = kVSFSysUtilErrOPNOTSUPP;
      break;
    case EACCES:
      retval = kVSFSysUtilErrACCES;
      break;
    case ENOENT:
      retval = kVSFSysUtilErrNOENT;
      break;
    default:
      break;
  }
  return retval;
}

int
vsf_sysutil_get_ipv4_sock(void)
{
  int retval = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (retval < 0)
  {
    die("socket");
  }
  return retval;
}

int
vsf_sysutil_get_ipv6_sock(void)
{
  int retval = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
  if (retval < 0)
  {
    die("socket");
  }
  return retval;
}

struct vsf_sysutil_socketpair_retval
vsf_sysutil_unix_stream_socketpair(void)
{
  struct vsf_sysutil_socketpair_retval retval;
  int the_sockets _Checked[2];
  int sys_retval = socketpair(PF_UNIX, SOCK_STREAM, 0, the_sockets);
  if (sys_retval != 0)
  {
    die("socketpair");
  }
  retval.socket_one = the_sockets[0];
  retval.socket_two = the_sockets[1];
  return retval;
}

int
vsf_sysutil_bind(int fd, _Ptr<const struct vsf_sysutil_sockaddr> p_sockptr)
{
  _Ptr<const struct sockaddr> p_sockaddr = &p_sockptr->u.u_sockaddr;
  int len = 0;
  if (p_sockaddr->sa_family == AF_INET)
  {
    len = sizeof(struct sockaddr_in);
  }
  else if (p_sockaddr->sa_family == AF_INET6)
  {
    len = sizeof(struct sockaddr_in6);
  }
  else
  {
    die("can only support ipv4 and ipv6 currently");
  }
  return bind(fd, p_sockaddr, len);
}

int
vsf_sysutil_listen(int fd, const unsigned int backlog)
{
  int retval = listen(fd, backlog);
  if (vsf_sysutil_retval_is_error(retval) &&
      vsf_sysutil_get_error() != kVSFSysUtilErrADDRINUSE)
  {
    die("listen");
  }
  return retval;
}

/* Warning: callers of this function assume it does NOT make use of any
 * non re-entrant calls such as malloc().
 */
int
vsf_sysutil_accept_timeout(int fd, struct vsf_sysutil_sockaddr *p_sockaddr : itype(_Ptr<struct vsf_sysutil_sockaddr>), unsigned int wait_seconds)
{
  struct vsf_sysutil_sockaddr remote_addr;
  int retval;
  int saved_errno;
  fd_set accept_fdset;
  struct timeval timeout;
  socklen_t socklen = sizeof(remote_addr);
  if (p_sockaddr)
  {
    vsf_sysutil_memclr<struct vsf_sysutil_sockaddr>(p_sockaddr, sizeof(*p_sockaddr));
  }
  if (wait_seconds > 0)
  {
    _Unchecked {
      FD_ZERO(&accept_fdset);
      FD_SET(fd, &accept_fdset);
    }
    timeout.tv_sec = wait_seconds;
    timeout.tv_usec = 0;
    do
    {
      _Unchecked {
        retval = select(fd + 1, &accept_fdset, NULL, NULL, &timeout);
      }
      saved_errno = errno;
      vsf_sysutil_check_pending_actions(kVSFSysUtilUnknown, 0, 0);
    }
    while (retval < 0 && saved_errno == EINTR);
    if (retval <= 0)
    {
      if (retval == 0)
      {
        errno = EAGAIN;
      }
      return -1;
    }
  }

  _Unchecked {
    struct sockaddr *tmp_sock = (struct sockaddr*) &remote_addr.u.u_sockaddr;
    socklen_t *tmp_len = (socklen_t*) &socklen;
    retval = accept(fd, tmp_sock, tmp_len);
  }

  vsf_sysutil_check_pending_actions(kVSFSysUtilUnknown, 0, 0);
  if (retval < 0)
  {
    return retval;
  }
  /* FreeBSD bug / paranoia: ai32@drexel.edu */
  if (socklen == 0)
  {
    return -1;
  }
  if (remote_addr.u.u_sockaddr.sa_family != AF_INET &&
      remote_addr.u.u_sockaddr.sa_family != AF_INET6)
  {
    die("can only support ipv4 and ipv6 currently");
  }
  if (p_sockaddr) 
  {
    if (remote_addr.u.u_sockaddr.sa_family == AF_INET)
    _Unchecked {
      vsf_sysutil_memclr(&remote_addr.u.u_sockaddr_in.sin_zero,
                         sizeof(remote_addr.u.u_sockaddr_in.sin_zero));
      vsf_sysutil_memcpy(p_sockaddr, &remote_addr.u.u_sockaddr_in,
                         sizeof(remote_addr.u.u_sockaddr_in));
    }
    else
    _Unchecked {
      vsf_sysutil_memcpy(p_sockaddr, &remote_addr.u.u_sockaddr_in6,
                         sizeof(remote_addr.u.u_sockaddr_in6));
    }
  }
  return retval;
}

int
vsf_sysutil_connect_timeout(int fd, _Ptr<const struct vsf_sysutil_sockaddr> p_addr, unsigned int wait_seconds)
{
  _Ptr<const struct sockaddr> p_sockaddr = &p_addr->u.u_sockaddr;
  unsigned int addrlen = 0;
  int retval;
  int saved_errno;
  if (p_sockaddr->sa_family == AF_INET)
  {
    addrlen = sizeof(p_addr->u.u_sockaddr_in);
  }
  else if (p_sockaddr->sa_family == AF_INET6)
  {
    addrlen = sizeof(p_addr->u.u_sockaddr_in6);
  }
  else
  {
    die("can only support ipv4 and ipv6 currently");
  }
  if (wait_seconds > 0)
  {
    vsf_sysutil_activate_noblock(fd);
  }
  retval = connect(fd, p_sockaddr, addrlen);
  if (retval < 0 && errno == EINPROGRESS)
  {
    fd_set connect_fdset;
    struct timeval timeout;
    _Unchecked {
      FD_ZERO(&connect_fdset);
      FD_SET(fd, &connect_fdset);
    }
    timeout.tv_sec = wait_seconds;
    timeout.tv_usec = 0;
    do
    {
      _Unchecked {
        retval = select(fd + 1, NULL, &connect_fdset, NULL, &timeout);
      }
      saved_errno = errno;
      vsf_sysutil_check_pending_actions(kVSFSysUtilUnknown, 0, 0);
    }
    while (retval < 0 && saved_errno == EINTR);
    if (retval <= 0)
    {
      if (retval == 0)
      {
        errno = EAGAIN;
      }
      retval = -1;
    }
    else
    {
      socklen_t socklen = sizeof(retval);
      int sockoptret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &retval, &socklen);
      if (sockoptret != 0)
      {
        die("getsockopt");
      }
      if (retval != 0)
      {
        errno = retval;
        retval = -1;
      }
    }
  }
  if (wait_seconds > 0)
  {
    vsf_sysutil_deactivate_noblock(fd);
  }
  return retval;
}

void
vsf_sysutil_getsockname(int fd, _Ptr<_Ptr<struct vsf_sysutil_sockaddr>> p_sockptr)
{
  struct vsf_sysutil_sockaddr the_addr;
  int retval;
  socklen_t socklen = sizeof(the_addr);
  vsf_sysutil_sockaddr_clear(p_sockptr);
  retval = getsockname(fd, &the_addr.u.u_sockaddr, &socklen);
  if (retval != 0)
  {
    die("getsockname");
  }
  if (the_addr.u.u_sockaddr.sa_family != AF_INET &&
      the_addr.u.u_sockaddr.sa_family != AF_INET6)
  {
    die("can only support ipv4 and ipv6 currently");
  }
  vsf_sysutil_sockaddr_alloc(p_sockptr);
  if (socklen > sizeof(the_addr))
  {
    socklen = sizeof(the_addr);
  }

  unchecked_widen_bounds(_Array_ptr<char>, p_sockptr_bytes, byte_count(sizeof(**p_sockptr)), *p_sockptr);
  checked_widen_bounds(_Array_ptr<char>, p_sockptr_bytes_len, byte_count(socklen), p_sockptr_bytes)

  unchecked_widen_bounds(_Array_ptr<char>, the_addr_bytes, byte_count(sizeof(the_addr)), &the_addr);
  checked_widen_bounds(_Array_ptr<char>, the_addr_bytes_len, byte_count(socklen), the_addr_bytes)

  vsf_sysutil_memcpy<char>(p_sockptr_bytes_len, the_addr_bytes_len, socklen);
}

void
vsf_sysutil_getpeername(int fd, _Ptr<_Ptr<struct vsf_sysutil_sockaddr>> p_sockptr)
{
  struct vsf_sysutil_sockaddr the_addr;
  int retval;
  socklen_t socklen = sizeof(the_addr);
  vsf_sysutil_sockaddr_clear(p_sockptr);
  retval = getpeername(fd, &the_addr.u.u_sockaddr, &socklen);
  if (retval != 0)
  {
    die("getpeername");
  }
  if (the_addr.u.u_sockaddr.sa_family != AF_INET &&
      the_addr.u.u_sockaddr.sa_family != AF_INET6)
  {
    die("can only support ipv4 and ipv6 currently");
  }
  vsf_sysutil_sockaddr_alloc(p_sockptr);
  if (socklen > sizeof(the_addr))
  {
    socklen = sizeof(the_addr);
  }

  unchecked_widen_bounds(_Array_ptr<char>, p_sockptr_bytes, byte_count(sizeof(**p_sockptr)), *p_sockptr);
  checked_widen_bounds(_Array_ptr<char>, p_sockptr_bytes_len, byte_count(socklen), p_sockptr_bytes)

  unchecked_widen_bounds(_Array_ptr<char>, the_addr_bytes, byte_count(sizeof(the_addr)), &the_addr);
  checked_widen_bounds(_Array_ptr<char>, the_addr_bytes_len, byte_count(socklen), the_addr_bytes)

  vsf_sysutil_memcpy<char>(p_sockptr_bytes_len, the_addr_bytes_len, socklen);
}

void
vsf_sysutil_shutdown_failok(int fd)
{
  /* SHUT_RDWR is a relatively new addition */
  #ifndef SHUT_RDWR
  #define SHUT_RDWR 2
  #endif
  (void) shutdown(fd, SHUT_RDWR);
}

void
vsf_sysutil_shutdown_read_failok(int fd)
{
  /* SHUT_RD is a relatively new addition */
  #ifndef SHUT_RD
  #define SHUT_RD 0
  #endif
  (void) shutdown(fd, SHUT_RD);
}

void
vsf_sysutil_sockaddr_clear(_Ptr<_Ptr<struct vsf_sysutil_sockaddr>> p_sockptr)
{
  if (*p_sockptr != NULL)
  {
    vsf_sysutil_free<struct vsf_sysutil_sockaddr>(*p_sockptr);
    *p_sockptr = NULL;
  }
}

void
vsf_sysutil_sockaddr_alloc(_Ptr<_Ptr<struct vsf_sysutil_sockaddr>> p_sockptr)
{
  vsf_sysutil_sockaddr_clear(p_sockptr);
  *p_sockptr = vsf_sysutil_malloc<struct vsf_sysutil_sockaddr>(sizeof(**p_sockptr));
  vsf_sysutil_memclr<struct vsf_sysutil_sockaddr>(*p_sockptr, sizeof(**p_sockptr));
}

void
vsf_sysutil_sockaddr_alloc_ipv4(_Ptr<_Ptr<struct vsf_sysutil_sockaddr>> p_sockptr)
{
  vsf_sysutil_sockaddr_alloc(p_sockptr);
  (*p_sockptr)->u.u_sockaddr.sa_family = AF_INET;
}

void
vsf_sysutil_sockaddr_alloc_ipv6(_Ptr<_Ptr<struct vsf_sysutil_sockaddr>> p_sockptr)
{
  vsf_sysutil_sockaddr_alloc(p_sockptr);
  (*p_sockptr)->u.u_sockaddr.sa_family = AF_INET6;
}

void
vsf_sysutil_sockaddr_clone(_Ptr<_Ptr<struct vsf_sysutil_sockaddr>> p_sockptr, _Ptr<const struct vsf_sysutil_sockaddr> p_src)
{
  _Ptr<struct vsf_sysutil_sockaddr> p_sockaddr = 0;
  vsf_sysutil_sockaddr_alloc(p_sockptr);
  p_sockaddr = *p_sockptr;
  if (p_src->u.u_sockaddr.sa_family == AF_INET)
  {
    p_sockaddr->u.u_sockaddr.sa_family = AF_INET;
    vsf_sysutil_memcpy<struct in_addr>(&p_sockaddr->u.u_sockaddr_in.sin_addr,
                       &p_src->u.u_sockaddr_in.sin_addr,
                       sizeof(p_sockaddr->u.u_sockaddr_in.sin_addr));
  }
  else if (p_src->u.u_sockaddr.sa_family == AF_INET6)
  {
    p_sockaddr->u.u_sockaddr.sa_family = AF_INET6;
    vsf_sysutil_memcpy<struct in6_addr>(&p_sockaddr->u.u_sockaddr_in6.sin6_addr,
                       &p_src->u.u_sockaddr_in6.sin6_addr,
                       sizeof(p_sockaddr->u.u_sockaddr_in6.sin6_addr));
    p_sockaddr->u.u_sockaddr_in6.sin6_scope_id =
        p_src->u.u_sockaddr_in6.sin6_scope_id;
  }
  else
  {
    die("can only support ipv4 and ipv6 currently");
  }
}

int
vsf_sysutil_sockaddr_addr_equal(_Ptr<const struct vsf_sysutil_sockaddr> p1, _Ptr<const struct vsf_sysutil_sockaddr> p2)
{
  int family1 = p1->u.u_sockaddr.sa_family;
  int family2 = p2->u.u_sockaddr.sa_family;
  if (family1 != family2)
  {
    if (family1 == AF_INET && family2 == AF_INET6) {
      _Ptr<const void> p_ipv4_addr = vsf_sysutil_sockaddr_ipv6_v4(p2);
      _Unchecked {
        if (p_ipv4_addr &&
            !vsf_sysutil_memcmp((void*) p_ipv4_addr, &p1->u.u_sockaddr_in.sin_addr,
                                sizeof(p1->u.u_sockaddr_in.sin_addr)))
        {
          return 1;
        }
      }
    }
    else if (family1 == AF_INET6 && family2 == AF_INET)
    _Unchecked {
      _Ptr<const void> p_ipv4_addr = vsf_sysutil_sockaddr_ipv6_v4(p1);
      if (p_ipv4_addr &&
          !vsf_sysutil_memcmp((void*) p_ipv4_addr, &p2->u.u_sockaddr_in.sin_addr,
                              sizeof(p2->u.u_sockaddr_in.sin_addr)))
      {
        return 1;
      }
    }
    return 0;
  }
  if (family1 == AF_INET)
  {
    if (vsf_sysutil_memcmp<const struct in_addr>(&p1->u.u_sockaddr_in.sin_addr,
                           &p2->u.u_sockaddr_in.sin_addr,
                           sizeof(p1->u.u_sockaddr_in.sin_addr)) == 0)
    {
      return 1;
    }
  }
  else if (family1 == AF_INET6)
  {
    if (vsf_sysutil_memcmp<const struct in6_addr>(&p1->u.u_sockaddr_in6.sin6_addr,
                           &p2->u.u_sockaddr_in6.sin6_addr,
                           sizeof(p1->u.u_sockaddr_in6.sin6_addr)) == 0)
    {
      return 1;
    }
  }
  return 0;
}

int
vsf_sysutil_sockaddr_is_ipv6(_Ptr<const struct vsf_sysutil_sockaddr> p_sockaddr)
{
  if (p_sockaddr->u.u_sockaddr.sa_family == AF_INET6)
  {
    return 1;
  }
  return 0;
}

void
vsf_sysutil_sockaddr_set_ipv4addr(_Ptr<struct vsf_sysutil_sockaddr> p_sockptr, _Array_ptr<const unsigned char> p_raw : byte_count(sizeof(struct in_addr)))
{
  if (p_sockptr->u.u_sockaddr.sa_family == AF_INET)
  _Unchecked {
    vsf_sysutil_memcpy(&p_sockptr->u.u_sockaddr_in.sin_addr, p_raw,
                       sizeof(p_sockptr->u.u_sockaddr_in.sin_addr));
  }
  else if (p_sockptr->u.u_sockaddr.sa_family == AF_INET6)
  {
    static _Ptr<struct vsf_sysutil_sockaddr> s_p_sockaddr;
    vsf_sysutil_sockaddr_alloc_ipv4(&s_p_sockaddr);
    _Unchecked {
      vsf_sysutil_memcpy(&s_p_sockaddr->u.u_sockaddr_in.sin_addr, p_raw,
                         sizeof(s_p_sockaddr->u.u_sockaddr_in.sin_addr));
      vsf_sysutil_memcpy(&p_sockptr->u.u_sockaddr_in6.sin6_addr,
                         (void*) vsf_sysutil_sockaddr_ipv4_v6(s_p_sockaddr),
                         sizeof(p_sockptr->u.u_sockaddr_in6.sin6_addr));
    }
  }
  else
  {
    bug("bad family");
  }
}

void
vsf_sysutil_sockaddr_set_ipv6addr(_Ptr<struct vsf_sysutil_sockaddr> p_sockptr, _Array_ptr<const unsigned char> p_raw : byte_count(sizeof(struct in6_addr)))
{
  if (p_sockptr->u.u_sockaddr.sa_family == AF_INET6)
  _Unchecked {
    vsf_sysutil_memcpy(&p_sockptr->u.u_sockaddr_in6.sin6_addr, p_raw,
                       sizeof(p_sockptr->u.u_sockaddr_in6.sin6_addr));
  }
  else
  {
    bug("bad family");
  }
}

int
vsf_sysutil_sockaddr_get_ipv6scope(_Ptr<struct vsf_sysutil_sockaddr> p_sockptr)
{
  return p_sockptr->u.u_sockaddr_in6.sin6_scope_id;
}

void
vsf_sysutil_sockaddr_set_ipv6scope(_Ptr<struct vsf_sysutil_sockaddr> p_sockptr, const int scope_id)
{
  p_sockptr->u.u_sockaddr_in6.sin6_scope_id = scope_id;
}

_Ptr<const void>
vsf_sysutil_sockaddr_ipv6_v4(_Ptr<const struct vsf_sysutil_sockaddr> p_addr)
{
  static unsigned char pattern _Checked[12] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF };
  _Array_ptr<const unsigned char> p_addr_start : count(12) = 0;
  if (p_addr->u.u_sockaddr.sa_family != AF_INET6)
  {
    return 0;
  }
  _Array_ptr<unsigned char> tmp : count(12) = 0;
  _Unchecked {
    tmp = _Assume_bounds_cast<_Array_ptr<unsigned char>>(&p_addr->u.u_sockaddr_in6.sin6_addr, count(12));
  }
  if (vsf_sysutil_memcmp<unsigned char>(pattern, tmp, 12))
  {
    return 0;
  }
  _Unchecked {
    p_addr_start = _Assume_bounds_cast<_Array_ptr<const unsigned char>>(&p_addr->u.u_sockaddr_in6.sin6_addr, count(12));
  }
  return &p_addr_start[12];
}

_Ptr<const void>
vsf_sysutil_sockaddr_ipv4_v6(_Ptr<const struct vsf_sysutil_sockaddr> p_addr)
{
  static unsigned char ret _Checked[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF };
  if (p_addr->u.u_sockaddr.sa_family != AF_INET)
  {
    return 0;
  }

  _Array_ptr<unsigned char> tmp : count(4) = 0;
  _Unchecked {
    tmp = _Assume_bounds_cast<_Array_ptr<unsigned char>>(&p_addr->u.u_sockaddr_in.sin_addr, count(4));
  }

  vsf_sysutil_memcpy<unsigned char>(&ret[12], tmp, 4);
  return ret;
}

_Ptr<void>
vsf_sysutil_sockaddr_get_raw_addr(_Ptr<struct vsf_sysutil_sockaddr> p_sockptr)
{
  if (p_sockptr->u.u_sockaddr.sa_family == AF_INET)
  {
    return &p_sockptr->u.u_sockaddr_in.sin_addr;
  }
  else if (p_sockptr->u.u_sockaddr.sa_family == AF_INET6)
  {
    return &p_sockptr->u.u_sockaddr_in6.sin6_addr;
  }
  else
  {
    bug("bad family");
  }
  return 0;
}

unsigned int
vsf_sysutil_get_ipaddr_size(void)
{
  struct vsf_sysutil_sockaddr addr;
  unsigned int size = sizeof(addr.u.u_sockaddr_in.sin_addr);
  unsigned int size2 = sizeof(addr.u.u_sockaddr_in6.sin6_addr);
  if (size2 > size)
  {
    size = size2;
  }
  return size;
}

int
vsf_sysutil_get_ipsock(_Ptr<const struct vsf_sysutil_sockaddr> p_addr)
{
  if (p_addr->u.u_sockaddr.sa_family == AF_INET)
  {
    return vsf_sysutil_get_ipv4_sock();
  }
  else if (p_addr->u.u_sockaddr.sa_family == AF_INET6)
  {
    return vsf_sysutil_get_ipv6_sock();
  }
  else
  {
    bug("bad family");
  }
  return -1;
}

void
vsf_sysutil_sockaddr_set_any(_Ptr<struct vsf_sysutil_sockaddr> p_sockaddr)
{
  if (p_sockaddr->u.u_sockaddr.sa_family == AF_INET)
  {
    vsf_sysutil_memclr<struct in_addr>(&p_sockaddr->u.u_sockaddr_in.sin_addr,
                       sizeof(p_sockaddr->u.u_sockaddr_in.sin_addr));
  }
  else if (p_sockaddr->u.u_sockaddr.sa_family == AF_INET6)
  {
    vsf_sysutil_memclr<struct in6_addr>(&p_sockaddr->u.u_sockaddr_in6.sin6_addr,
                       sizeof(p_sockaddr->u.u_sockaddr_in6.sin6_addr));
  }
  else
  {
    bug("bad family");
  }
}

unsigned short
vsf_sysutil_sockaddr_get_port(_Ptr<const struct vsf_sysutil_sockaddr> p_sockptr)
{
  if (p_sockptr->u.u_sockaddr.sa_family == AF_INET)
  {
    return ntohs(p_sockptr->u.u_sockaddr_in.sin_port);
  }
  else if (p_sockptr->u.u_sockaddr.sa_family == AF_INET6)
  {
    return ntohs(p_sockptr->u.u_sockaddr_in6.sin6_port);
  }
  else
  {
    bug("bad family");
  }
  /* NOTREACHED */
  return 0;
}

void
vsf_sysutil_sockaddr_set_port(_Ptr<struct vsf_sysutil_sockaddr> p_sockptr, unsigned short the_port)
{
  if (p_sockptr->u.u_sockaddr.sa_family == AF_INET)
  {
    p_sockptr->u.u_sockaddr_in.sin_port = htons(the_port);
  }
  else if (p_sockptr->u.u_sockaddr.sa_family == AF_INET6)
  {
    p_sockptr->u.u_sockaddr_in6.sin6_port = htons(the_port);
  }
  else
  {
    bug("bad family");
  }
}

int
vsf_sysutil_is_port_reserved(unsigned short the_port)
{
  if (the_port < IPPORT_RESERVED)
  {
    return 1;
  }
  return 0;
}

_Nt_array_ptr<const char> vsf_sysutil_inet_ntop(_Ptr<const struct vsf_sysutil_sockaddr> p_sockptr) : count(0)
{
  _Ptr<const struct sockaddr> p_sockaddr = &p_sockptr->u.u_sockaddr;
  if (p_sockaddr->sa_family == AF_INET)
  {
    return ((_Nt_array_ptr<char> )inet_ntoa(p_sockptr->u.u_sockaddr_in.sin_addr));
  }
  else if (p_sockaddr->sa_family == AF_INET6)
  {
    static char inaddr_buf _Nt_checked[65];
    _Nt_array_ptr<const char> p_ret = 0;
    _Unchecked {
      p_ret = (_Nt_array_ptr<const char>) inet_ntop(AF_INET6, &p_sockptr->u.u_sockaddr_in6.sin6_addr, inaddr_buf, sizeof(inaddr_buf) - 1);

    }
    inaddr_buf[sizeof(inaddr_buf) - 1] = '\0';
    if (p_ret == NULL)
    {
      inaddr_buf[0] = '\0';
    }
    return inaddr_buf;
  }
  else
  {
    die("can only support ipv4 and ipv6 currently");
    return 0;
  }
}

_Nt_array_ptr<const char> vsf_sysutil_inet_ntoa(_Ptr<const void> p_raw_addr) : count(0) _Unchecked
{
  return ((_Nt_array_ptr<char> )inet_ntoa(*((struct in_addr*)p_raw_addr)));
}

int
vsf_sysutil_inet_aton(_Nt_array_ptr<const char> p_text, _Ptr<struct vsf_sysutil_sockaddr> p_addr)
{
  struct in_addr sin_addr;
  if (p_addr->u.u_sockaddr.sa_family != AF_INET)
  {
    bug("bad family");
  }
  if (inet_aton(p_text, &sin_addr))
  {
    vsf_sysutil_memcpy<struct in_addr>(&p_addr->u.u_sockaddr_in.sin_addr,
                       &sin_addr, sizeof(p_addr->u.u_sockaddr_in.sin_addr));
    return 1;
  }
  else
  {
    return 0;
  }
}

void
vsf_sysutil_dns_resolve(_Ptr<_Ptr<struct vsf_sysutil_sockaddr>> p_sockptr, _Nt_array_ptr<const char> p_name : count(0))
{
  _Ptr<struct hostent> hent = gethostbyname(p_name);
  if (hent == NULL)
  {
    die2("cannot resolve host:", p_name);
  }
  vsf_sysutil_sockaddr_clear(p_sockptr);
  if (hent->h_addrtype == AF_INET)
  {
    unsigned int len = hent->h_length;
    if (len > sizeof((*p_sockptr)->u.u_sockaddr_in.sin_addr))
    {
      len = sizeof((*p_sockptr)->u.u_sockaddr_in.sin_addr);
    }
    vsf_sysutil_sockaddr_alloc_ipv4(p_sockptr);
    _Unchecked {
      vsf_sysutil_memcpy(&(*p_sockptr)->u.u_sockaddr_in.sin_addr,
                         hent->h_addr_list[0], len);
    }
  }
  else if (hent->h_addrtype == AF_INET6)
  {
    unsigned int len = hent->h_length;
    if (len > sizeof((*p_sockptr)->u.u_sockaddr_in6.sin6_addr))
    {
      len = sizeof((*p_sockptr)->u.u_sockaddr_in6.sin6_addr);
    }
    vsf_sysutil_sockaddr_alloc_ipv6(p_sockptr);
    _Unchecked {
      vsf_sysutil_memcpy(&(*p_sockptr)->u.u_sockaddr_in6.sin6_addr,
                         hent->h_addr_list[0], len);
    }
  }
  else
  {
    die("gethostbyname(): neither IPv4 nor IPv6");
  }
}

struct vsf_sysutil_user *vsf_sysutil_getpwuid(const int uid) : itype(_Ptr<struct vsf_sysutil_user>)
{
  if (uid < 0)
  {
    bug("negative uid in vsf_sysutil_getpwuid");
  }
  _Unchecked {
    return (struct vsf_sysutil_user*) getpwuid((unsigned int) uid);
  }
}

struct vsf_sysutil_user *vsf_sysutil_getpwnam(_Nt_array_ptr<const char> p_user) : itype(_Ptr<struct vsf_sysutil_user>) _Unchecked
{
  return (struct vsf_sysutil_user*) getpwnam(p_user);
}

const char *vsf_sysutil_user_getname(const struct vsf_sysutil_user *p_user : itype(_Ptr<const struct vsf_sysutil_user>)) : itype(_Nt_array_ptr<const char>) count(0) _Unchecked
{
  const struct passwd* p_passwd = (const struct passwd*) p_user;
  return p_passwd->pw_name;
}

const char *vsf_sysutil_user_get_homedir(const struct vsf_sysutil_user *p_user : itype(_Ptr<const struct vsf_sysutil_user>)) : itype(_Nt_array_ptr<const char>) count(0) _Unchecked
{
  const struct passwd* p_passwd = (const struct passwd*) p_user;
  return p_passwd->pw_dir;
}

int
vsf_sysutil_user_getuid(const struct vsf_sysutil_user *p_user : itype(_Ptr<const struct vsf_sysutil_user>)) _Unchecked
{
  const struct passwd* p_passwd = (const struct passwd*) p_user;
  return p_passwd->pw_uid;
}

int
vsf_sysutil_user_getgid(const struct vsf_sysutil_user *p_user : itype(_Ptr<const struct vsf_sysutil_user>)) _Unchecked
{ 
  const struct passwd* p_passwd = (const struct passwd*) p_user;
  return p_passwd->pw_gid;
}

struct vsf_sysutil_group *vsf_sysutil_getgrgid(const int gid) : itype(_Ptr<struct vsf_sysutil_group>) _Unchecked
{
  if (gid < 0)
  {
    die("negative gid in vsf_sysutil_getgrgid");
  }
  return (struct vsf_sysutil_group*) getgrgid((unsigned int) gid);
}

const char *vsf_sysutil_group_getname(const struct vsf_sysutil_group *p_group : itype(_Ptr<const struct vsf_sysutil_group>)) : itype(_Nt_array_ptr<const char>) count(0) _Unchecked
{
  const struct group* p_grp = (const struct group*) p_group;
  return p_grp->gr_name;
}

unsigned char
vsf_sysutil_get_random_byte(void)
{
  static int seeded;
  unsigned int uint_res;
  unsigned char c1, c2, c3, c4;
  if (!seeded)
  {
    struct timeval tv;
    int retval = 0;
    _Unchecked {
      retval = gettimeofday(&tv, NULL);
    }
    if (retval != 0)
    {
      die("gettimeofday");
    }
    srand((unsigned)tv.tv_usec);
    seeded = 1;
  }
  uint_res = rand();
  c1 = uint_res & 0x000000ff;
  c2 = (uint_res >> 8) & 0x000000ff;
  c3 = (uint_res >> 16) & 0x000000ff;
  c4 = (uint_res >> 24) & 0x000000ff;
  return c1 ^ c2 ^ c3 ^ c4;    
}

int
vsf_sysutil_running_as_root(void)
{
  return (getuid() == 0);
}

void
vsf_sysutil_setuid(const struct vsf_sysutil_user *p_user : itype(_Ptr<const struct vsf_sysutil_user>)) _Unchecked
{
  const struct passwd* p_passwd = (const struct passwd*) p_user;
  vsf_sysutil_setuid_numeric(p_passwd->pw_uid);
}

void
vsf_sysutil_setuid_numeric(int uid)
{
  int retval = setuid(uid);
  if (retval != 0)
  {
    die("setuid");
  }
}

void
vsf_sysutil_setgid(const struct vsf_sysutil_user *p_user : itype(_Ptr<const struct vsf_sysutil_user>)) _Unchecked
{
  const struct passwd* p_passwd = (const struct passwd*) p_user;
  vsf_sysutil_setgid_numeric(p_passwd->pw_gid);
}

void
vsf_sysutil_setgid_numeric(int gid)
{
  int retval = setgid(gid);
  if (retval != 0)
  {
    die("setgid");
  }
}

int
vsf_sysutil_geteuid(void)
{
  int retval = geteuid();
  if (retval < 0)
  {
    die("geteuid");
  }
  return retval;
}

int
vsf_sysutil_getegid(void)
{
  int retval = getegid();
  if (retval < 0)
  {
    die("getegid");
  }
  return retval;
}

void
vsf_sysutil_seteuid(const struct vsf_sysutil_user *p_user : itype(_Ptr<const struct vsf_sysutil_user>)) _Unchecked
{
  const struct passwd* p_passwd = (const struct passwd*) p_user;
  vsf_sysutil_seteuid_numeric(p_passwd->pw_uid);
}

void
vsf_sysutil_setegid(const struct vsf_sysutil_user *p_user : itype(_Ptr<const struct vsf_sysutil_user>)) _Unchecked
{
  const struct passwd* p_passwd = (const struct passwd*) p_user;
  vsf_sysutil_setegid_numeric(p_passwd->pw_gid);
}

void
vsf_sysutil_seteuid_numeric(int uid)
{
  /* setreuid() would seem to be more portable than seteuid() */
  int retval = setreuid(-1, uid);
  if (retval != 0)
  {
    die("seteuid");
  }
}

void
vsf_sysutil_setegid_numeric(int gid)
{
  /* setregid() would seem to be more portable than setegid() */
  int retval = setregid(-1, gid);
  if (retval != 0)
  {
    die("setegid");
  }
}

void
vsf_sysutil_clear_supp_groups(void)
{
  int retval = setgroups(0, NULL);
  if (retval != 0)
  {
    die("setgroups");
  }
}

void
vsf_sysutil_initgroups(const struct vsf_sysutil_user *p_user : itype(_Ptr<const struct vsf_sysutil_user>)) _Unchecked
{
  const struct passwd* p_passwd = (const struct passwd*) p_user;
  int retval = initgroups(p_passwd->pw_name, p_passwd->pw_gid);
  if (retval != 0)
  {
    die("initgroups");
  }
}

void
vsf_sysutil_chroot(_Nt_array_ptr<const char> p_root_path : count(0))
{
  int retval = chroot(p_root_path);
  if (retval != 0)
  {
    die("chroot");
  }
}

unsigned int
vsf_sysutil_get_umask(void)
{
  return s_current_umask;
}

void
vsf_sysutil_set_umask(unsigned int new_umask)
{
  s_current_umask = (new_umask & 0777);
  (void) umask(s_current_umask);
}

void
vsf_sysutil_make_session_leader(void)
{
  /* This makes us the leader if we are not already */
  (void) setsid();
  /* Check we're the leader */
  if ((int) vsf_sysutil_getpid() != getpgrp())
  {
    die("not session leader");
  }
}

void
vsf_sysutil_reopen_standard_fds(void)
{
  /* This reopens STDIN, STDOUT and STDERR to /dev/null */
  int fd;
  _Unchecked {
    fd = open("/dev/null", O_RDWR, 0);
  }
  if (fd < 0)
  {
    goto error;
  }
  vsf_sysutil_dupfd2(fd, STDIN_FILENO);
  vsf_sysutil_dupfd2(fd, STDOUT_FILENO);
  vsf_sysutil_dupfd2(fd, STDERR_FILENO);
  if ( fd > 2 )
  {
    vsf_sysutil_close(fd);
  }
  return;

error:
  die("reopening standard file descriptors to /dev/null failed");
}

void
vsf_sysutil_tzset(void)
{
  int retval;
  char tzbuf _Nt_checked[8];
  time_t the_time = time(NULL);
  _Ptr<struct tm> p_tm = ((void *)0);
  tzset();
  p_tm = localtime(&the_time);
  if (p_tm == NULL)
  {
    die("localtime");
  }
  /* Set our timezone in the TZ environment variable to cater for the fact
   * that modern glibc does not cache /etc/localtime (which becomes inaccessible
   * when we chroot().
   */
  retval = strftime(tzbuf, sizeof(tzbuf) - 1, "%z", p_tm);
  tzbuf[sizeof(tzbuf) - 1] = '\0';
  if (retval == 5)
  {
    /* Static because putenv() does not copy the string. */
    static char envtz _Checked[sizeof("TZ=UTC-hh:mm")];
    /* Insert a colon so we have e.g. -05:00 instead of -0500 */
    tzbuf[5] = tzbuf[4];
    tzbuf[4] = tzbuf[3];
    tzbuf[3] = ':';
    /* Invert the sign - we just got the offset _from_ UTC but for TZ, we need
     * the offset _to_ UTC.
     */
    if (tzbuf[0] == '+')
    {
      tzbuf[0] = '-';
    }
    else
    {
      tzbuf[0] = '+';
    }
    _Unchecked {
      snprintf((char*) envtz, sizeof(envtz), "TZ=UTC%s", tzbuf);
      putenv((char*) envtz);
    }
    s_timezone = ((tzbuf[1] - '0') * 10 + (tzbuf[2] - '0')) * 60 * 60;
    s_timezone += ((tzbuf[4] - '0') * 10 + (tzbuf[5] - '0')) * 60;
    if (tzbuf[0] == '-')
    {
      s_timezone *= -1;
    }
  }
  /* Call in to the time subsystem again now that TZ is set, trying to force
   * caching of the actual zoneinfo for the timezone.
   */
  p_tm = localtime(&the_time);
  if (p_tm == NULL)
  {
    die("localtime #2");
  }
  p_tm = gmtime(&the_time);
  if (p_tm == NULL)
  {
    die("gmtime");
  }
}

_Nt_array_ptr<const char> vsf_sysutil_get_current_date(void) : count(0)
{
  static char datebuf _Nt_checked[65];
  time_t curr_time;
  _Ptr<const struct tm> p_tm = ((void *)0);
  int i = 0;
  curr_time = vsf_sysutil_get_time_sec();
  p_tm = localtime(&curr_time);
  if (strftime(datebuf, sizeof(datebuf) - 1, "%a %b!%d %H:%M:%S %Y", p_tm) == 0)
  {
    die("strftime");
  }
  datebuf[sizeof(datebuf) - 1] = '\0';
  /* This hack is because %e in strftime() isn't so portable */
  while (datebuf[i] != '!' && datebuf[i] != '\0')
  {
    ++i;
  }
  if (datebuf[i] == '!')
  {
    datebuf[i] = ' ';
    if (datebuf[i+1] == '0')
    {
      datebuf[i+1] = ' ';
    }
  }
  return datebuf;
}

long
vsf_sysutil_get_time_sec(void)
{
  int res = 0;
  _Unchecked {
    res = gettimeofday(&s_current_time, NULL);
  }
  if (res != 0)
  {
    die("gettimeofday");
  }
  return s_current_time.tv_sec;
}

long
vsf_sysutil_get_time_usec(void)
{
  return s_current_time.tv_usec;
}

void
vsf_sysutil_qsort(_Array_ptr<void> p_base : byte_count(num_elem * elem_size), unsigned int num_elem , unsigned int elem_size, _Ptr<int (_Ptr<const void>, _Ptr<const void>)> p_compar)
{
  qsort(p_base, num_elem, elem_size, p_compar);
}

void
vsf_sysutil_sleep(double seconds)
{
  int retval;
  int saved_errno;
  double fractional;
  time_t secs;
  struct timespec ts;
  secs = (time_t) seconds;
  fractional = seconds - (double) secs;
  ts.tv_sec = secs;
  ts.tv_nsec = (long) (fractional * (double) 1000000000);
  do
  {
    _Unchecked {
      retval = nanosleep(&ts, &ts);
    }
    saved_errno = errno;
    vsf_sysutil_check_pending_actions(kVSFSysUtilUnknown, 0, 0);
  } while (retval == -1 && saved_errno == EINTR);
}

_Nt_array_ptr<char> vsf_sysutil_getenv(_Nt_array_ptr<const char> p_var : count(0)) : count(0)
{
  return ((_Nt_array_ptr<char> )getenv(p_var));
}

void
vsf_sysutil_openlog(int force)
{
  int facility = LOG_DAEMON;
  int option = LOG_PID;
  if (!force)
  {
    option |= LOG_NDELAY;
  }
#ifdef LOG_FTP
  facility = LOG_FTP;
#endif
  openlog("vsftpd", option, facility);
}

void
vsf_sysutil_closelog(void)
{
  closelog();
}

void
vsf_sysutil_syslog(_Nt_array_ptr<const char> p_text, int severe)
{
  int prio = LOG_INFO;
  if (severe)
  {
    prio = LOG_WARNING;
  }
  _Unchecked {
    syslog(prio, "%s", p_text);
  }
}

long
vsf_sysutil_parse_time(_Nt_array_ptr<const char> p_text : count(0))
{
  struct tm the_time;
  unsigned int len = vsf_sysutil_strlen(p_text);
  unchecked_widen_bounds(_Nt_array_ptr<const char>, p_text_len, count(len), p_text);
  vsf_sysutil_memclr<struct tm>(&the_time, sizeof(the_time));
  if (len >= 8)
  {
    _Nt_array_ptr<const char> tmp : count(8) = _Dynamic_bounds_cast<_Nt_array_ptr<const char>>(p_text_len, count(8));
    char yr _Nt_checked[6];
    char mon _Nt_checked[4];
    char day _Nt_checked[4];
    vsf_sysutil_strcpy(yr, tmp, 5);
    vsf_sysutil_strcpy(mon, tmp + 4, 3);
    vsf_sysutil_strcpy(day, tmp + 6, 3);
    the_time.tm_year = vsf_sysutil_atoi(yr) - 1900;
    the_time.tm_mon = vsf_sysutil_atoi(mon) - 1;
    the_time.tm_mday = vsf_sysutil_atoi(day);
  }
  if (len >= 14)
  {
    _Nt_array_ptr<const char> tmp : count(14) = _Dynamic_bounds_cast<_Nt_array_ptr<const char>>(p_text_len, count(14));
    char hr _Nt_checked[4];
    char mins _Nt_checked[4];
    char sec _Nt_checked[4];
    vsf_sysutil_strcpy(hr, tmp + 8, 3);
    vsf_sysutil_strcpy(mins, tmp + 10, 3);
    vsf_sysutil_strcpy(sec, tmp + 12, 3);
    the_time.tm_hour = vsf_sysutil_atoi(hr);
    the_time.tm_min = vsf_sysutil_atoi(mins);
    the_time.tm_sec = vsf_sysutil_atoi(sec);
  }
  return mktime(&the_time);
}

int
vsf_sysutil_setmodtime(_Nt_array_ptr<const char> p_file, long the_time, int is_localtime)
{
  struct utimbuf new_times;
  if (!is_localtime)
  {
    the_time -= s_timezone;
  }
  vsf_sysutil_memclr<struct utimbuf>(&new_times, sizeof(new_times));
  new_times.actime = the_time;
  new_times.modtime = the_time;
  return utime(p_file, &new_times);
}

void
vsf_sysutil_ftruncate(int fd)
{
  int ret = ftruncate(fd, 0);
  if (ret != 0)
  {
    die("ftruncate");
  }
}

int
vsf_sysutil_getuid(void)
{
  return getuid();
}

void
vsf_sysutil_set_address_space_limit(unsigned long bytes)
{
  /* Unfortunately, OpenBSD is missing RLIMIT_AS. */
#ifdef RLIMIT_AS
  int ret;
  struct rlimit rlim;
  rlim.rlim_cur = bytes;
  rlim.rlim_max = bytes;
  _Unchecked {
    ret = setrlimit(RLIMIT_AS, &rlim);
  }
  /* Permit EPERM as this could indicate that the shell launching vsftpd already
   * has a lower limit.
   */
  if (ret != 0 && errno != EPERM)
  {
    die("setrlimit");
  }
#endif /* RLIMIT_AS */
  (void) bytes;
}

void
vsf_sysutil_set_no_fds()
{
  int ret;
  struct rlimit rlim;
  rlim.rlim_cur = 0;
  rlim.rlim_max = 0;
  _Unchecked {
    ret = setrlimit(RLIMIT_NOFILE, &rlim);
  }
  if (ret != 0)
  {
    die("setrlimit NOFILE");
  }
}

void
vsf_sysutil_set_no_procs()
{
#ifdef RLIMIT_NPROC
  int ret;
  struct rlimit rlim;
  rlim.rlim_cur = 0;
  rlim.rlim_max = 0;
  _Unchecked {
    ret = setrlimit(RLIMIT_NPROC, &rlim);
  }
  if (ret != 0)
  {
    die("setrlimit NPROC");
  }
#endif
}

static void handle_sigill(int sig) {
  die("spatial memory safety violation");
}

void
vsf_sysutil_post_fork()
{
  int i;
  /* Don't inherit any exit function. */
  s_exit_func = NULL;
  /* Uncached the current PID. */ 
  s_current_pid = -1;
  /* Don't inherit anything relating to the synchronous signal system */
  s_io_handler = NULL;
  for (i=0; i < NSIG; ++i)
  {
    s_sig_details[i].sync_sig_handler = NULL;
  }
  for (i=0; i < NSIG; ++i)
  {
    s_sig_details[i].pending = 0;
  }

  signal(SIGILL, handle_sigill);
}
