/*
 * Part of Very Secure FTPd
 * Licence: GPL v2
 * Author: Chris Evans
 * ascii.c
 *
 * Routines to handle ASCII mode tranfers. Yuk.
 */

#include "ascii.h"

#pragma CHECKED_SCOPE on

struct ascii_to_bin_ret
vsf_ascii_ascii_to_bin(_Array_ptr<char> p_buf : count(in_len + 1), unsigned int in_len, int prev_cr)
{
  /* Task: translate all \r\n into plain \n. A plain \r not followed by \n must
   * not be removed.
   */
  int last_was_cr = 0;
  unsigned int indexx = 0;
  unsigned int written = 0;
  _Array_ptr<char> p_out : count(in_len) = p_buf + 1;
  _Array_ptr<char> p_ret_buf : count(in_len) = 0;
  if (prev_cr && (!in_len || p_out[0] != '\n'))
  {
    p_buf[0] = '\r';
    p_ret_buf = p_buf;
    written++;
  }
  else
  {
    p_ret_buf = p_out;
  }
  unsigned int p_out_offset = 0;
  while (indexx < in_len)
  {
    char the_char = p_buf[indexx + 1];
    if (the_char != '\r')
    {
      p_out[p_out_offset] = the_char;
      p_out_offset++;
      written++;
    }
    else if (indexx == in_len - 1)
    {
      last_was_cr = 1;
    }
    else if (p_buf[indexx + 2] != '\n')
    {
      p_out[p_out_offset] = the_char;
      p_out_offset++;
      written++;
    }
    indexx++;
  }
  return (struct ascii_to_bin_ret){written, last_was_cr, p_ret_buf};
}

struct bin_to_ascii_ret
vsf_ascii_bin_to_ascii(_Array_ptr<const char> p_in : count(in_len), _Array_ptr<char> p_out : count(in_len * 2), unsigned int in_len, int prev_cr)
{
  /* Task: translate all \n not preceeded by \r into \r\n.
   * Note that \r\n stays as \r\n. We used to map it to \r\r\n like wu-ftpd
   * but have switched to leaving it, like the more popular proftpd.
   */
  struct bin_to_ascii_ret ret = { 0, 0 };
  unsigned int indexx = 0;
  unsigned int written = 0;
  char last_char = 0;
  if (prev_cr)
  {
    last_char = '\r';
    ret.last_was_cr = 1;
  }
  unsigned int p_out_offset = 0;
  while (indexx < in_len)
  {
    char the_char = p_in[indexx];
    if (the_char == '\n' && last_char != '\r')
    {
      p_out[p_out_offset] = '\r';
      p_out_offset++;
      written++;
    }
    p_out[p_out_offset] = the_char;
    p_out_offset++;
    written++;
    indexx++;
    last_char = the_char;
    if (the_char == '\r')
    {
      ret.last_was_cr = 1;
    }
    else
    {
      ret.last_was_cr = 0;
    }
  }
  ret.stored = written;
  return ret;
}

