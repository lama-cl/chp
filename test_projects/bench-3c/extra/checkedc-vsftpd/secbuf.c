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

#include<stdio.h>
void
vsf_secbuf_alloc(struct secbuf *buf : itype(_Ptr<struct secbuf>))
{
  unsigned int page_size = vsf_sysutil_getpagesize();
  unsigned int page_offset = buf->size % page_size;

  /* Free any previous buffer */
  vsf_secbuf_free(buf);

  unsigned int round_up = round_up_page_size(buf->size);

  _Array_ptr<char>  p_mmap : count(round_up) = vsf_sysutil_map_anon_pages<char>(round_up);

  /* Map the first and last page inaccessible */
  unsigned int no_access_start = round_up - page_size;
  _Array_ptr<char> p_no_access_page : bounds(p_mmap + no_access_start, p_mmap + round_up) = p_mmap + no_access_start;
  _Array_ptr<char> p_no_access_page_page_size : count(page_size) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_no_access_page, count(page_size));
  vsf_sysutil_memprotect<char>(p_no_access_page_page_size, page_size, kVSFSysUtilMapProtNone);

  _Array_ptr<char> p_no_access_page2 : count(page_size) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_mmap, count(page_size));
  vsf_sysutil_memprotect<char>(p_no_access_page2, page_size, kVSFSysUtilMapProtNone);

  unsigned int p_mmap_offset = page_size;
  if (page_offset)
  {
    p_mmap_offset += (page_size - page_offset);
  }

  // Bounds casting required while porting to make assignments to struct fields work.
  buf->map_offset = p_mmap_offset,
    buf->p_ptr = _Dynamic_bounds_cast<_Array_ptr<char>>(p_mmap + p_mmap_offset, count(buf->size)),
    buf->noaccess_page = _Dynamic_bounds_cast<_Array_ptr<char>>(p_mmap, bounds(buf->p_ptr - buf->map_offset, buf->p_ptr));
}

void
vsf_secbuf_free(struct secbuf *buf : itype(_Ptr<struct secbuf>))
{
  if (buf->p_ptr == 0)
  {
    return;
  }

  _Array_ptr<char> p_mmap_offset : bounds(buf->p_ptr - buf->map_offset, buf->p_ptr + buf->size) = buf->p_ptr - buf->map_offset;

  /* First make the first page readable so we can get the size */
  unsigned int page_size = vsf_sysutil_getpagesize();
  _Array_ptr<char> p_mmap_offset_page_size : count(page_size) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_mmap_offset, count(page_size));
  vsf_sysutil_memprotect<char>(p_mmap_offset_page_size, page_size, kVSFSysUtilMapProtReadOnly);

  /* Lose the mapping */
  _Array_ptr<char> p_mmap_offset_full : count(buf->size + buf->map_offset) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_mmap_offset, count(buf->size + buf->map_offset));
  vsf_sysutil_memunmap<char>(p_mmap_offset_full, buf->size + buf->map_offset);
}
