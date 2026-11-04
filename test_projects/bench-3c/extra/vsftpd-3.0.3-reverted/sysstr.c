/*
 * Part of Very Secure FTPd
 * Licence: GPL v2
 * Author: Chris Evans
 * sysstr.c
 *
 * This file basically wraps system functions so that we can deal in our
 * nice abstracted string buffer objects.
 */

#include "sysstr.h"
#include "str.h"
#include "secbuf.h"
#include "sysutil.h"
#include "defs.h"
#include "utility.h"
#include "tunables.h"
#include "hacks.h"


void
str_getcwd(struct mystr * p_str)
{
  static char * p_getcwd_buf ;
  char * p_ret = ((void *)0);
  if (p_getcwd_buf == 0)
  {
    vsf_secbuf_static_alloc(p_getcwd_buf, VSFTP_PATH_MAX);
  }
  /* In case getcwd() fails */
  str_empty(p_str);
  p_ret = vsf_sysutil_getcwd(p_getcwd_buf, VSFTP_PATH_MAX);
  if (p_ret != 0)
  {
    str_alloc_text(p_str, p_ret);
  }
}

int
str_write_loop(const struct mystr * p_str, const int fd)
{
  int len = str_getlen(p_str);
  unchecked_widen_bounds(const char *, tmp, count(len), str_getbuf(p_str))
  return vsf_sysutil_write_loop(fd, tmp, len);
}

int
str_read_loop(struct mystr * p_str, const int fd)
{
  int len = str_getlen(p_str);
  unchecked_widen_bounds(const char *, tmp, count(len), str_getbuf(p_str))
  return vsf_sysutil_read_loop(fd, tmp, len);
}

int
str_mkdir(const struct mystr * p_str, const unsigned int mode)
{
  return vsf_sysutil_mkdir(((const char * )str_getbuf(p_str)), mode);
}

int
str_rmdir(const struct mystr * p_str)
{
  return vsf_sysutil_rmdir(((const char * )str_getbuf(p_str)));
}

int
str_unlink(const struct mystr * p_str)
{
  return vsf_sysutil_unlink(str_getbuf(p_str));
}

int
str_chdir(const struct mystr * p_str)
{
  return vsf_sysutil_chdir(((const char * )str_getbuf(p_str)));
}

int
str_open(const struct mystr * p_str, const enum EVSFSysStrOpenMode mode)
{
  enum EVSFSysUtilOpenMode open_mode = kVSFSysStrOpenUnknown;
  switch (mode)
  {
    case kVSFSysStrOpenReadOnly:
      open_mode = kVSFSysUtilOpenReadOnly;
      break;
    case kVSFSysStrOpenUnknown:
      /* Fall through */
    default:
      bug("unknown mode value in str_open");
      break;
  }
  return vsf_sysutil_open_file(str_getbuf(p_str), open_mode);
}

int
str_stat(const struct mystr * p_str, struct vsf_sysutil_statbuf * * p_ptr)
{
  return vsf_sysutil_stat(((const char * )str_getbuf(p_str)), p_ptr);
}

int
str_lstat(const struct mystr * p_str, struct vsf_sysutil_statbuf * * p_ptr)
{
  return vsf_sysutil_lstat(((const char * )str_getbuf(p_str)), p_ptr);
}

int
str_create_exclusive(const struct mystr * p_str)
{
  return vsf_sysutil_create_file_exclusive(str_getbuf(p_str));
}

int
str_create(const struct mystr * p_str)
{
  return vsf_sysutil_create_or_open_file(
      str_getbuf(p_str), tunable_file_open_mode);
}

int
str_chmod(const struct mystr * p_str, unsigned int mode)
{
  return vsf_sysutil_chmod(((const char * )str_getbuf(p_str)), mode);
}

int
str_rename(const struct mystr * p_from_str, const struct mystr * p_to_str)
{
  return vsf_sysutil_rename(((const char * )str_getbuf(p_from_str)), ((const char * )str_getbuf(p_to_str)));
}

struct vsf_sysutil_dir * str_opendir(const struct mystr * p_str)
{
  return vsf_sysutil_opendir(str_getbuf(p_str));
}

void
str_next_dirent(struct mystr * p_filename_str, struct vsf_sysutil_dir * p_dir)
{
  const char * p_filename  = ((const char * )vsf_sysutil_next_dirent(p_dir));
  str_empty(p_filename_str);
  if (p_filename != 0)
  {
    str_alloc_text(p_filename_str, p_filename);
  }
}

int
str_readlink(struct mystr * p_str, const struct mystr * p_filename_str)
{
  static char * p_readlink_buf ;
  int retval;
  if (p_readlink_buf == 0)
  {
    vsf_secbuf_static_alloc(p_readlink_buf, VSFTP_PATH_MAX);
  }

  /* In case readlink() fails */
  str_empty(p_str);
  /* Note: readlink(2) does not NULL terminate, but our wrapper does */
  retval = vsf_sysutil_readlink(str_getbuf(p_filename_str), p_readlink_buf, VSFTP_PATH_MAX);
  if (vsf_sysutil_retval_is_error(retval))
  {
    return retval;
  }

  char * readlink_nt_buf = 0;
   { readlink_nt_buf = (char *) p_readlink_buf; }

  str_alloc_text(p_str, readlink_nt_buf);
  return 0;
}

struct vsf_sysutil_user * str_getpwnam(const struct mystr * p_user_str)
{
  return vsf_sysutil_getpwnam(((const char * )str_getbuf(p_user_str)));
}

void
str_syslog(const struct mystr * p_str, int severe)
{
  vsf_sysutil_syslog(((const char * )str_getbuf(p_str)), severe);
}

