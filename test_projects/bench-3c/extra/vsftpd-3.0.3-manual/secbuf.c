/*
 * Part of Very Secure FTPd
 * Licence: GPL v2
 * Author: Chris Evans
 * secbuf.c
 *
 * Here are some routines providing the (possibly silly) concept of a secure
 * buffer. A secure buffer may not be overflowed. A single byte overflow
 * will cause the program to safely terminate.
 */

#include "secbuf.h"
#include "utility.h"
#include "sysutil.h"
#include "sysdeputil.h"

#pragma CHECKED_SCOPE on

static unsigned int
round_up_page_size(unsigned int buf_size) {
  /* Round up to next page size */
  unsigned int round_up = 0;
  unsigned int page_size = vsf_sysutil_getpagesize();
  unsigned int page_offset = buf_size % page_size;
  if (page_offset)
  {
    unsigned int num_pages = buf_size / page_size;
    num_pages++;
    round_up = num_pages * page_size;
  }
  else
  {
    /* Allocation is on a page-size boundary */
    round_up = buf_size;
  }
  /* Add on another two pages to make inaccessible */
  round_up += page_size * 2;
  return round_up;
}

void
vsf_secbuf_alloc(_Ptr<struct secbuf> buf)
{
  unsigned int page_size = vsf_sysutil_getpagesize();
  unsigned int page_offset = buf->size % page_size;

  /* Free any previous buffer */
  vsf_secbuf_free(buf);

  unsigned int round_up = round_up_page_size(buf->size);

  _Array_ptr<char> p_mmap : byte_count(round_up) = vsf_sysutil_map_anon_pages<char>(round_up);

  /* Map the first and last page inaccessible */
  _Array_ptr<char> p_no_access_page : byte_count(page_size) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_mmap + round_up - page_size, byte_count(page_size));
  vsf_sysutil_memprotect<char>(p_no_access_page, page_size, kVSFSysUtilMapProtNone);

  _Array_ptr<char> p_no_access_page2 : byte_count(page_size) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_mmap, byte_count(page_size));
  vsf_sysutil_memprotect<char>(p_no_access_page2, page_size, kVSFSysUtilMapProtNone);

  unsigned int p_mmap_offset = page_size;
  if (page_offset)
  {
    p_mmap_offset += (page_size - page_offset);
  }

  buf->p_ptr = p_mmap + p_mmap_offset;
  buf->map_offset = p_mmap_offset;
}

void
vsf_secbuf_free(_Ptr<struct secbuf> buf)
{
  if (buf->p_ptr == 0)
  {
    return;
  }

  _Array_ptr<char> p_mmap_offset : count(buf->map_offset + buf->size) = _Dynamic_bounds_cast<_Array_ptr<char>>(buf->p_ptr - buf->map_offset,  count(buf->map_offset + buf->size));

  /* First make the first page readable so we can get the size */
  unsigned int page_size = vsf_sysutil_getpagesize();
  vsf_sysutil_memprotect<char>(p_mmap_offset, page_size, kVSFSysUtilMapProtReadOnly);

  unsigned int map_size = round_up_page_size(buf->size);
  /* Lose the mapping */
  vsf_sysutil_memunmap<char>(p_mmap_offset, map_size);
}
