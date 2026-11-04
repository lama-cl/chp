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

#pragma CHECKED_SCOPE on

void
str_getcwd(_Ptr<struct mystr> p_str)
{
  static _Array_ptr<char> p_getcwd_buf : count(VSFTP_PATH_MAX);
  _Nt_array_ptr<char> p_ret = ((void *)0);
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
str_write_loop(_Ptr<const struct mystr> p_str, const int fd)
{
  int len = str_getlen(p_str);
  unchecked_widen_bounds(_Nt_array_ptr<const char>, tmp, count(len), str_getbuf(p_str))
  return vsf_sysutil_write_loop<const char>(fd, tmp, len);
}

int
str_read_loop(_Ptr<struct mystr> p_str, const int fd)
{
  int len = str_getlen(p_str);
  unchecked_widen_bounds(_Nt_array_ptr<const char>, tmp, count(len), str_getbuf(p_str))
  return vsf_sysutil_read_loop<char>(fd, tmp, len);
}

int
str_mkdir(_Ptr<const struct mystr> p_str, const unsigned int mode)
{
  return vsf_sysutil_mkdir(((_Nt_array_ptr<const char> )str_getbuf(p_str)), mode);
}

int
str_rmdir(_Ptr<const struct mystr> p_str)
{
  return vsf_sysutil_rmdir(((_Nt_array_ptr<const char> )str_getbuf(p_str)));
}

int
str_unlink(_Ptr<const struct mystr> p_str)
{
  return vsf_sysutil_unlink(str_getbuf(p_str));
}

int
str_chdir(_Ptr<const struct mystr> p_str)
{
  return vsf_sysutil_chdir(((_Nt_array_ptr<const char> )str_getbuf(p_str)));
}

int
str_open(_Ptr<const struct mystr> p_str, const enum EVSFSysStrOpenMode mode)
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
str_stat(_Ptr<const struct mystr> p_str, _Ptr<_Ptr<struct vsf_sysutil_statbuf>> p_ptr)
{
  return vsf_sysutil_stat(((_Nt_array_ptr<const char> )str_getbuf(p_str)), p_ptr);
}

int
str_lstat(_Ptr<const struct mystr> p_str, _Ptr<_Ptr<struct vsf_sysutil_statbuf>> p_ptr)
{
  return vsf_sysutil_lstat(((_Nt_array_ptr<const char> )str_getbuf(p_str)), p_ptr);
}

int
str_create_exclusive(_Ptr<const struct mystr> p_str)
{
  return vsf_sysutil_create_file_exclusive(str_getbuf(p_str));
}

int
str_create(_Ptr<const struct mystr> p_str)
{
  return vsf_sysutil_create_or_open_file(
      str_getbuf(p_str), tunable_file_open_mode);
}

int
str_chmod(_Ptr<const struct mystr> p_str, unsigned int mode)
{
  return vsf_sysutil_chmod(((_Nt_array_ptr<const char> )str_getbuf(p_str)), mode);
}

int
str_rename(_Ptr<const struct mystr> p_from_str, _Ptr<const struct mystr> p_to_str)
{
  return vsf_sysutil_rename(((_Nt_array_ptr<const char> )str_getbuf(p_from_str)), ((_Nt_array_ptr<const char> )str_getbuf(p_to_str)));
}

_Ptr<struct vsf_sysutil_dir> str_opendir(_Ptr<const struct mystr> p_str)
{
  return vsf_sysutil_opendir(str_getbuf(p_str));
}

void
str_next_dirent(_Ptr<struct mystr> p_filename_str, _Ptr<struct vsf_sysutil_dir> p_dir)
{
  _Nt_array_ptr<const char> p_filename : count(0) = ((_Nt_array_ptr<const char> )vsf_sysutil_next_dirent(p_dir));
  str_empty(p_filename_str);
  if (p_filename != 0)
  {
    str_alloc_text(p_filename_str, p_filename);
  }
}

int
str_readlink(_Ptr<struct mystr> p_str, _Ptr<const struct mystr> p_filename_str)
{
  static _Array_ptr<char> p_readlink_buf : count(VSFTP_PATH_MAX);
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

  _Nt_array_ptr<char> readlink_nt_buf = 0;
  _Unchecked { readlink_nt_buf = (_Nt_array_ptr<char>) p_readlink_buf; }

  str_alloc_text(p_str, readlink_nt_buf);
  return 0;
}

_Ptr<struct vsf_sysutil_user> str_getpwnam(_Ptr<const struct mystr> p_user_str)
{
  return vsf_sysutil_getpwnam(((_Nt_array_ptr<const char> )str_getbuf(p_user_str)));
}

void
str_syslog(_Ptr<const struct mystr> p_str, int severe)
{
  vsf_sysutil_syslog(((_Nt_array_ptr<const char> )str_getbuf(p_str)), severe);
}

