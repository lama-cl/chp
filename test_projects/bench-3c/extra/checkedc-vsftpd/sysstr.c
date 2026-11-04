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

#pragma CHECKED_SCOPE on

void
str_getcwd(struct mystr *p_str : itype(_Ptr<struct mystr>))
{
  static _Nt_array_ptr<char> p_getcwd_buf : count(VSFTP_PATH_MAX) = 0;
  _Nt_array_ptr<char> p_ret = ((void *)0);
  if (p_getcwd_buf == 0)
  {
    _Array_ptr<char> tmp : count(VSFTP_PATH_MAX + 1) = 0;

    _Ptr<struct secbuf> __tmp_secbuf = &(struct secbuf){0, 0, VSFTP_PATH_MAX + 1, 0};
    vsf_secbuf_alloc(__tmp_secbuf);
    tmp = _Dynamic_bounds_cast<_Array_ptr<char>>(__tmp_secbuf->p_ptr, count(VSFTP_PATH_MAX + 1));

    tmp[VSFTP_PATH_MAX] = '\0';
    _Unchecked {
      p_getcwd_buf = (_Nt_array_ptr<char>) tmp;
    }
  }
  /* In case getcwd() fails */
  str_empty(p_str);
  p_ret = vsf_sysutil_getcwd(p_getcwd_buf, VSFTP_PATH_MAX);
  if (p_ret != 0)
  {
    str_alloc_text(p_str, p_getcwd_buf);
  }
}

int
str_write_loop(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const int fd)
{
  unsigned int len = str_getlen(p_str);
  _Nt_array_ptr<char> buf : count(len) = _Dynamic_bounds_cast<_Nt_array_ptr<char>>(str_getbuf(p_str), count(len));
  return vsf_sysutil_write_loop<const char>(fd, buf, len);
}

int
str_read_loop(struct mystr *p_str : itype(_Ptr<struct mystr>), const int fd)
{
  unsigned int len = str_getlen(p_str);
  _Nt_array_ptr<char> buf : count(len) = _Dynamic_bounds_cast<_Nt_array_ptr<char>>(str_getbuf(p_str), count(len));
  return vsf_sysutil_read_loop(fd, buf, len);
}

int
str_mkdir(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const unsigned int mode)
{
  return vsf_sysutil_mkdir(str_getbuf(p_str), mode);
}

int
str_rmdir(const struct mystr *p_str : itype(_Ptr<const struct mystr>))
{
  return vsf_sysutil_rmdir(str_getbuf(p_str));
}

int
str_unlink(const struct mystr *p_str : itype(_Ptr<const struct mystr>))
{
  return vsf_sysutil_unlink(str_getbuf(p_str));
}

int
str_chdir(const struct mystr *p_str : itype(_Ptr<const struct mystr>))
{
  return vsf_sysutil_chdir(str_getbuf(p_str));
}

int
str_open(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const enum EVSFSysStrOpenMode mode)
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
str_stat(const struct mystr *p_str : itype(_Ptr<const struct mystr>), struct vsf_sysutil_statbuf **p_ptr : itype(_Ptr<_Ptr<struct vsf_sysutil_statbuf>>))
{
  return vsf_sysutil_stat(str_getbuf(p_str), p_ptr);
}

int
str_lstat(const struct mystr *p_str : itype(_Ptr<const struct mystr>), struct vsf_sysutil_statbuf **p_ptr : itype(_Ptr<_Ptr<struct vsf_sysutil_statbuf>>))
{
  return vsf_sysutil_lstat(str_getbuf(p_str), p_ptr);
}

int
str_create_exclusive(const struct mystr *p_str : itype(_Ptr<const struct mystr>))
{
  return vsf_sysutil_create_file_exclusive(str_getbuf(p_str));
}

int
str_create(const struct mystr *p_str : itype(_Ptr<const struct mystr>))
{
  return vsf_sysutil_create_or_open_file(
      str_getbuf(p_str), tunable_file_open_mode);
}

int
str_chmod(const struct mystr *p_str : itype(_Ptr<const struct mystr>), unsigned int mode)
{
  return vsf_sysutil_chmod(str_getbuf(p_str), mode);
}

int
str_rename(const struct mystr *p_from_str : itype(_Ptr<const struct mystr>), const struct mystr *p_to_str : itype(_Ptr<const struct mystr>))
{
  return vsf_sysutil_rename(str_getbuf(p_from_str), str_getbuf(p_to_str));
}

struct vsf_sysutil_dir *str_opendir(const struct mystr *p_str : itype(_Ptr<const struct mystr>)) : itype(_Ptr<struct vsf_sysutil_dir>)
{
  return vsf_sysutil_opendir(str_getbuf(p_str));
}

void
str_next_dirent(struct mystr *p_filename_str : itype(_Ptr<struct mystr>), struct vsf_sysutil_dir *p_dir : itype(_Ptr<struct vsf_sysutil_dir>))
{
  _Nt_array_ptr<const char> p_filename = ((_Nt_array_ptr<const char> )vsf_sysutil_next_dirent(p_dir));
  str_empty(p_filename_str);
  if (p_filename != 0)
  {
    str_alloc_text(p_filename_str, p_filename);
  }
}

int
str_readlink(struct mystr *p_str : itype(_Ptr<struct mystr>), const struct mystr *p_filename_str : itype(_Ptr<const struct mystr>))
{
  static _Nt_array_ptr<char> p_readlink_buf : count(VSFTP_PATH_MAX) = 0;
  int retval;
  if (p_readlink_buf == 0)
  {
    _Array_ptr<char> tmp : count(VSFTP_PATH_MAX + 1) = 0;

    _Ptr<struct secbuf> __tmp_secbuf = &(struct secbuf){0, 0, VSFTP_PATH_MAX + 1, 0};
    vsf_secbuf_alloc(__tmp_secbuf);
    tmp = _Dynamic_bounds_cast<_Array_ptr<char>>(__tmp_secbuf->p_ptr, count(VSFTP_PATH_MAX + 1));

    tmp[VSFTP_PATH_MAX] = '\0';
    _Unchecked {
      p_readlink_buf = (_Nt_array_ptr<char>) tmp;
    }
  }
  /* In case readlink() fails */
  str_empty(p_str);
  /* Note: readlink(2) does not NULL terminate, but our wrapper does */
  retval = vsf_sysutil_readlink(str_getbuf(p_filename_str), p_readlink_buf,
                                VSFTP_PATH_MAX);
  if (vsf_sysutil_retval_is_error(retval))
  {
    return retval;
  }
  str_alloc_text(p_str, p_readlink_buf);
  return 0;
}

struct vsf_sysutil_user *str_getpwnam(const struct mystr *p_user_str : itype(_Ptr<const struct mystr>)) : itype(_Ptr<struct vsf_sysutil_user>)
{
  return vsf_sysutil_getpwnam(str_getbuf(p_user_str));
}

void
str_syslog(const struct mystr *p_str : itype(_Ptr<const struct mystr>), int severe)
{
  vsf_sysutil_syslog(str_getbuf(p_str), severe);
}

