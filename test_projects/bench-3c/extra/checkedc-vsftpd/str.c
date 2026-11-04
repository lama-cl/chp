/*
 * Part of Very Secure FTPd
 * Licence: GPL v2
 * Author: Chris Evans
 * str.c
 *
 * Generic string handling functions. The fact that a string is implemented
 * internally using a buffer is not exposed in the API. If you can't see
 * the buffers, you can't handle them in a screwed way. Or so goes the
 * theory, anyway...
 */

/* Anti-lamer measures deployed, sir! */
#define PRIVATE_HANDS_OFF_p_buf p_buf
#define PRIVATE_HANDS_OFF_len len
#define PRIVATE_HANDS_OFF_alloc_bytes alloc_bytes
#include "str.h"

/* Ick. Its for die() */
#include "utility.h"
#include "sysutil.h"

#pragma CHECKED_SCOPE on

/* File local functions */
static void str_split_text_common(struct mystr *p_src : itype(_Ptr<struct mystr>), struct mystr *p_rhs : itype(_Ptr<struct mystr>), const char *p_text : itype(_Nt_array_ptr<const char>), int is_reverse);
static int str_equal_internal(const char *p_buf1 : itype(_Array_ptr<const char>) count(buf1_len), unsigned int buf1_len, const char *p_buf2 : itype(_Array_ptr<const char>) count(buf2_len), unsigned int buf2_len);

/* Private functions */
static void
s_setbuf(struct mystr *p_str : itype(_Ptr<struct mystr>), char *p_newbuf : itype(_Nt_array_ptr<char>) count(len), unsigned int len)
{
  if (p_str->p_buf != 0)
  {
    bug("p_buf not NULL when setting it");
  }
  p_str->alloc_bytes = len, p_str->p_buf = p_newbuf;
}

void
private_str_alloc_memchunk(struct mystr *p_str : itype(_Ptr<struct mystr>), const char *p_src : itype(_Array_ptr<const char>) byte_count(len), unsigned int len)
{
  /* Make sure this will fit in the buffer */
  unsigned int buf_needed;
  if (len + 1 < len)
  {
    bug("integer overflow");
  }
  buf_needed = len + 1;
  if (buf_needed > p_str->alloc_bytes)
  {
    str_free(p_str);
    _Array_ptr<char> arr_buf : count(buf_needed + 1) = vsf_sysutil_malloc<char>(buf_needed + 1);
    arr_buf[buf_needed] = '\0';
    _Nt_array_ptr<char> nt_buf : count(buf_needed) = 0;
    _Unchecked {
      nt_buf = _Assume_bounds_cast<_Nt_array_ptr<char>>(arr_buf, count(buf_needed));
    }
    s_setbuf(p_str, nt_buf, buf_needed);
  }
  _Array_ptr<char> p_str_buf : count(len) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_str->p_buf, count(len));
  vsf_sysutil_memcpy<char>(p_str_buf, p_src, len);
  p_str->p_buf[len] = '\0';
  p_str->len = len;
}

void
private_str_append_memchunk(struct mystr *p_str : itype(_Ptr<struct mystr>), const char *p_src : itype(_Array_ptr<const char>) byte_count(len), unsigned int len)
{
  unsigned int buf_needed;
  if (len + p_str->len < len)
  {
    bug("integer overflow");
  }
  buf_needed = len + p_str->len;
  if (buf_needed + 1 < buf_needed)
  {
    bug("integer overflow");
  }
  buf_needed++;
  if (buf_needed > p_str->alloc_bytes)
  {
    _Array_ptr<char> arr_buf : count(buf_needed + 1) = vsf_sysutil_realloc<char>(p_str->p_buf, buf_needed + 1);
    arr_buf[buf_needed] = '\0';
    _Nt_array_ptr<char> nt_buf : count(buf_needed) = 0;
    _Unchecked {
      nt_buf = _Assume_bounds_cast<_Nt_array_ptr<char>>(arr_buf, count(buf_needed));
    }
    p_str->alloc_bytes = buf_needed, p_str->p_buf = nt_buf;
  }
  _Array_ptr<char> p_str_buf_end : count(len) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_str->p_buf + p_str->len, count(len));
  vsf_sysutil_memcpy<char>(p_str_buf_end, p_src, len);
  p_str->p_buf[p_str->len + len] = '\0';
  p_str->len += len;
}

/* Public functions */
void
str_alloc_text(struct mystr *p_str : itype(_Ptr<struct mystr>), const char *p_src : itype(_Nt_array_ptr<const char>))
{
  unsigned int len = vsf_sysutil_strlen(p_src);
  _Nt_array_ptr<const char> tmp : count(len) = 0;
  _Unchecked {
    tmp = _Assume_bounds_cast<_Nt_array_ptr<const char>>(p_src, count(len));
  }
  private_str_alloc_memchunk(p_str, tmp, len);
}

void
str_copy(struct mystr *p_dest : itype(_Ptr<struct mystr>), const struct mystr *p_src : itype(_Ptr<const struct mystr>))
{
  _Array_ptr<char> p_src_len : count(p_src->len) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_src->p_buf, count(p_src->len));
  private_str_alloc_memchunk(p_dest, p_src_len, p_src->len);
}

const char *str_strdup(const struct mystr *p_str : itype(_Ptr<const struct mystr>)) : itype(_Nt_array_ptr<const char>)
{
  return vsf_sysutil_strdup(((_Nt_array_ptr<const char> )str_getbuf(p_str)));
}

const char *str_strdup_trimmed(const struct mystr *p_str : itype(_Ptr<const struct mystr>)) : itype(_Nt_array_ptr<const char>)
{
  _Nt_array_ptr<const char> p_trimmed : count(p_str->len) = str_getbuf(p_str);
  int h, t;
  unsigned int newlen;

  for (h = 0; h < (int)str_getlen(p_str) && vsf_sysutil_isspace(p_trimmed[h]); h++) ;
  for (t = str_getlen(p_str) - 1; t >= 0 && vsf_sysutil_isspace(p_trimmed[t]); t--) ;
  newlen = t - h + 1;
  _Nt_array_ptr<char> trimmed_offset : count(newlen) = _Dynamic_bounds_cast<_Nt_array_ptr<char>>(p_trimmed + h, count(newlen));
  if (newlen)
    return vsf_sysutil_strndup(trimmed_offset, (unsigned int)newlen);
  return 0;
}

void
str_alloc_alt_term(struct mystr *p_str : itype(_Ptr<struct mystr>), const char *p_src : itype(_Array_ptr<const char>) count(maxlen), char term, unsigned int maxlen)
{
  _Array_ptr<const char> p_search : bounds(p_src, p_src + maxlen) = p_src;
  unsigned int len = 0;
  while (*p_search != term)
  {
    p_search++;
    len++;
    if (len == 0)
    {
      bug("integer overflow");
    }
  }
  _Array_ptr<const char> p_src_len : count(len) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_src, count(len));
  private_str_alloc_memchunk(p_str, p_src_len, len);
}

void
str_alloc_ulong(struct mystr *p_str : itype(_Ptr<struct mystr>), unsigned long the_long)
{
  str_alloc_text(p_str, vsf_sysutil_ulong_to_str(the_long));
}

void
str_alloc_filesize_t(struct mystr *p_str : itype(_Ptr<struct mystr>), filesize_t the_filesize)
{
  str_alloc_text(p_str, vsf_sysutil_filesize_t_to_str(the_filesize));
}

void
str_free(struct mystr *p_str : itype(_Ptr<struct mystr>))
{
  if (p_str->p_buf != 0)
  {
    vsf_sysutil_free<char>(p_str->p_buf);
  }
  p_str->alloc_bytes = 0, p_str->p_buf = 0;
  p_str->len = 0;
}

void
str_empty(struct mystr *p_str : itype(_Ptr<struct mystr>))
{
  /* Ensure a buffer is allocated. */
  (void) str_getbuf(p_str);
  str_trunc(p_str, 0);
}

void
str_trunc(struct mystr *p_str : itype(_Ptr<struct mystr>), unsigned int trunc_len)
{
  if (trunc_len >= p_str->alloc_bytes)
  {
    bug("trunc_len not smaller than alloc_bytes in str_trunc");
  }
  p_str->len = trunc_len;
  p_str->p_buf[p_str->len] = '\0';
}

void
str_reserve(struct mystr *p_str : itype(_Ptr<struct mystr>), unsigned int res_len)
{
  /* Reserve space for the trailing zero as well. */
  res_len++;
  if (res_len == 0)
  {
    bug("integer overflow");
  }
  if (res_len > p_str->alloc_bytes)
  {
    _Array_ptr<char> arr_buf : count(res_len + 1) = vsf_sysutil_realloc<char>(p_str->p_buf, res_len + 1);
    arr_buf[res_len] = '\0';
    _Nt_array_ptr<char> nt_buf : count(res_len) = 0;
    _Unchecked {
      nt_buf = _Assume_bounds_cast<_Nt_array_ptr<char>>(arr_buf, count(res_len));
    }
    p_str->alloc_bytes = res_len, p_str->p_buf = nt_buf;

  }
  p_str->p_buf[res_len - 1] = '\0';
}

int
str_isempty(const struct mystr *p_str : itype(_Ptr<const struct mystr>))
{
  return (p_str->len == 0);
}

unsigned int
str_getlen(const struct mystr *p_str : itype(_Ptr<const struct mystr>))
{
  return p_str->len;
}

const char*
str_getbuf(const struct mystr *p_str : itype(_Ptr<const struct mystr>)) : itype(_Nt_array_ptr<const char>) count(p_str->len)
{
  if (p_str->p_buf == 0)
  {
    if (p_str->len != 0 || p_str->alloc_bytes != 0)
    {
      bug("p_buf NULL and len or alloc_bytes != 0 in str_getbuf");
    }
    private_str_alloc_memchunk((_Ptr<struct mystr>)p_str, 0, 0);
  }
  return _Dynamic_bounds_cast<_Nt_array_ptr<char>>(p_str->p_buf, count(p_str->len));
}

int
str_strcmp(const struct mystr *p_str1 : itype(_Ptr<const struct mystr>), const struct mystr *p_str2 : itype(_Ptr<const struct mystr>))
{

  _Array_ptr<const char> p_str1_buf : count(p_str1->len) = _Dynamic_bounds_cast<_Array_ptr<const char>>(p_str1->p_buf, count(p_str1->len));
  _Array_ptr<const char> p_str2_buf : count(p_str2->len) = _Dynamic_bounds_cast<_Array_ptr<const char>>(p_str2->p_buf, count(p_str2->len));
  return str_equal_internal(p_str1_buf, p_str1->len,
                            p_str2_buf, p_str2->len);
}

static int
str_equal_internal(const char *p_buf1 : itype(_Array_ptr<const char>) count(buf1_len), unsigned int buf1_len, const char *p_buf2 : itype(_Array_ptr<const char>) count(buf2_len), unsigned int buf2_len)
{
  int retval;
  unsigned int minlen = buf1_len;
  if (buf2_len < minlen)
  {
    minlen = buf2_len;
  }
  _Array_ptr<const char> minlen_1 : count(minlen) = _Dynamic_bounds_cast<_Array_ptr<const char>>(p_buf1, count(minlen));
  _Array_ptr<const char> minlen_2 : count(minlen) = _Dynamic_bounds_cast<_Array_ptr<const char>>(p_buf2, count(minlen));
  retval = vsf_sysutil_memcmp<const char>(minlen_1, minlen_2, minlen);
  if (retval != 0 || buf1_len == buf2_len)
  {
    return retval;
  }
  /* Strings equal but lengths differ. The greater one, then, is the longer */
  return (int) (buf1_len - buf2_len);
}

int
str_equal(const struct mystr *p_str1 : itype(_Ptr<const struct mystr>), const struct mystr *p_str2 : itype(_Ptr<const struct mystr>))
{
  return (str_strcmp(p_str1, p_str2) == 0);
}

int
str_equal_text(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const char *p_text : itype(_Nt_array_ptr<const char>))
{
  unsigned int cmplen = vsf_sysutil_strlen(p_text);
  _Nt_array_ptr<const char> p_text_len : count(cmplen) = 0;
  _Unchecked {
    p_text_len = _Assume_bounds_cast<_Nt_array_ptr<const char>>(p_text, count(cmplen));
  }
  _Array_ptr<const char> p_str_buf : count(p_str->len) = _Dynamic_bounds_cast<_Array_ptr<const char>>(p_str->p_buf, count(p_str->len));
  return (str_equal_internal(p_str_buf, p_str->len, p_text_len, cmplen) == 0);
}

void
str_append_str(struct mystr *p_str : itype(_Ptr<struct mystr>), const struct mystr *p_other : itype(_Ptr<const struct mystr>))
{

  _Array_ptr<const char> p_other_buf : count(p_other->len) = _Dynamic_bounds_cast<_Array_ptr<const char>>(p_other->p_buf, count(p_other->len));
  private_str_append_memchunk(p_str, p_other_buf, p_other->len);
}

void
str_append_text(struct mystr *p_str : itype(_Ptr<struct mystr>), const char *p_src : itype(_Nt_array_ptr<const char>))
{
  unsigned int len = vsf_sysutil_strlen(p_src);
  _Nt_array_ptr<const char> src_len : count(len) = 0;
  _Unchecked {
    src_len = _Assume_bounds_cast<_Nt_array_ptr<const char>>(p_src, count(len));
  }
  private_str_append_memchunk(p_str, src_len, len);
}

void
str_append_char(struct mystr *p_str : itype(_Ptr<struct mystr>), char the_char)
{
  private_str_append_memchunk(p_str, &the_char, sizeof(the_char));
}

void
str_append_ulong(struct mystr *p_str : itype(_Ptr<struct mystr>), unsigned long the_ulong)
{
  str_append_text(p_str, vsf_sysutil_ulong_to_str(the_ulong));
}

void
str_append_filesize_t(struct mystr *p_str : itype(_Ptr<struct mystr>), filesize_t the_filesize)
{
  str_append_text(p_str, vsf_sysutil_filesize_t_to_str(the_filesize));
}

void
str_append_double(struct mystr *p_str : itype(_Ptr<struct mystr>), double the_double)
{
  str_append_text(p_str, vsf_sysutil_double_to_str(the_double));
}

void
str_upper(struct mystr *p_str : itype(_Ptr<struct mystr>))
{
  unsigned int i;
  for (i=0; i < p_str->len; i++)
  {
    p_str->p_buf[i] = (char) vsf_sysutil_toupper(p_str->p_buf[i]);
  }
}

void
str_rpad(struct mystr *p_str : itype(_Ptr<struct mystr>), const unsigned int min_width)
{
  unsigned int to_pad;
  if (p_str->len >= min_width)
  {
    return;
  }
  to_pad = min_width - p_str->len;
  while (to_pad--)
  {
    str_append_char(p_str, ' ');
  }
}

void
str_lpad(struct mystr *p_str : itype(_Ptr<struct mystr>), const unsigned int min_width)
{
  static struct mystr s_tmp_str = {};
  unsigned int to_pad;
  if (p_str->len >= min_width)
  {
    return;
  }
  to_pad = min_width - p_str->len;
  str_empty(&s_tmp_str);
  while (to_pad--)
  {
    str_append_char(&s_tmp_str, ' ');
  }
  str_append_str(&s_tmp_str, p_str);
  str_copy(p_str, &s_tmp_str);
}

void
str_replace_char(struct mystr *p_str : itype(_Ptr<struct mystr>), char from, char to)
{
  unsigned int i;
  for (i=0; i < p_str->len; i++)
  {
    if (p_str->p_buf[i] == from)
    {
      p_str->p_buf[i] = to;
    }
  }
}

void
str_replace_text(struct mystr *p_str : itype(_Ptr<struct mystr>), const char *p_from : itype(_Nt_array_ptr<const char>), const char *p_to : itype(_Nt_array_ptr<const char>))
{
  static struct mystr s_lhs_chunk_str = {};
  static struct mystr s_rhs_chunk_str = {};
  unsigned int lhs_len;
  str_copy(&s_lhs_chunk_str, p_str);
  str_free(p_str);
  do
  {
    lhs_len = str_getlen(&s_lhs_chunk_str);
    str_split_text(&s_lhs_chunk_str, &s_rhs_chunk_str, p_from);
    /* Copy lhs to destination */
    str_append_str(p_str, &s_lhs_chunk_str);
    /* If this was a 'hit', append the 'to' text */
    if (str_getlen(&s_lhs_chunk_str) < lhs_len)
    {
      str_append_text(p_str, p_to);
    }
    /* Current rhs becomes new lhs */
    str_copy(&s_lhs_chunk_str, &s_rhs_chunk_str);
  } while (!str_isempty(&s_lhs_chunk_str));
}

void
str_split_char(struct mystr *p_src : itype(_Ptr<struct mystr>), struct mystr *p_rhs : itype(_Ptr<struct mystr>), char c)
{
  /* Just use str_split_text */
  char ministr _Nt_checked[2];
  ministr[0] = c;
  ministr[1] = '\0';
  str_split_text(p_src, p_rhs, ministr);
}

void
str_split_char_reverse(struct mystr *p_src : itype(_Ptr<struct mystr>), struct mystr *p_rhs : itype(_Ptr<struct mystr>), char c)
{
  /* Just use str_split_text_reverse */
  char ministr _Nt_checked[2];
  ministr[0] = c;
  ministr[1] = '\0';
  str_split_text_reverse(p_src, p_rhs, ministr);
}

void
str_split_text(struct mystr *p_src : itype(_Ptr<struct mystr>), struct mystr *p_rhs : itype(_Ptr<struct mystr>), const char *p_text : itype(_Nt_array_ptr<const char>))
{
  str_split_text_common(p_src, p_rhs, p_text, 0);
}

void
str_split_text_reverse(struct mystr *p_src : itype(_Ptr<struct mystr>), struct mystr *p_rhs : itype(_Ptr<struct mystr>), const char *p_text : itype(_Nt_array_ptr<const char>))
{
  str_split_text_common(p_src, p_rhs, p_text, 1);
}

static void
str_split_text_common(struct mystr *p_src : itype(_Ptr<struct mystr>), struct mystr *p_rhs : itype(_Ptr<struct mystr>), const char *p_text : itype(_Nt_array_ptr<const char>), int is_reverse)
{
  struct str_locate_result locate_result;
  unsigned int indexx;
  unsigned int search_len = vsf_sysutil_strlen(p_text);
  if (is_reverse)
  {
    locate_result = str_locate_text_reverse(p_src, p_text);
  }
  else
  {
    locate_result = str_locate_text(p_src, p_text);
  }
  /* Not found? */
  if (!locate_result.found)
  {
    str_empty(p_rhs);
    return;
  }
  indexx = locate_result.index;
  if (indexx + search_len > p_src->len)
  {
    bug("indexx invalid in str_split_text");
  } 
  /* Build rhs */
  _Array_ptr<char> search_buf : count(p_src->len - indexx - search_len) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_src->p_buf + indexx + search_len, count(p_src->len - indexx - search_len));
  private_str_alloc_memchunk(p_rhs, search_buf,
                             p_src->len - indexx - search_len);
  /* Build lhs */
  str_trunc(p_src, indexx);
}

struct str_locate_result
str_locate_str(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const struct mystr *p_look_str : itype(_Ptr<const struct mystr>))
{
  return str_locate_text(p_str, ((_Nt_array_ptr<const char> )str_getbuf(p_look_str)));
}

struct str_locate_result
str_locate_str_reverse(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const struct mystr *p_look_str : itype(_Ptr<const struct mystr>))
{
  return str_locate_text_reverse(p_str, ((_Nt_array_ptr<const char> )str_getbuf(p_look_str)));
}

struct str_locate_result
str_locate_char(const struct mystr *p_str : itype(_Ptr<const struct mystr>), char look_char)
{
  char look_str _Nt_checked[2];
  look_str[0] = look_char;
  look_str[1] = '\0';
  return str_locate_text(p_str, look_str);
}

struct str_locate_result
str_locate_chars(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const char *p_chars : itype(_Nt_array_ptr<const char>))
{
  struct str_locate_result retval;
  unsigned int num_chars = vsf_sysutil_strlen(p_chars);
  unsigned int i = 0;
  retval.found = 0;
  retval.char_found = 0;
  retval.index = 0;
  for (; i < p_str->len; ++i)
  {
    unsigned int j = 0;
    char this_char = p_str->p_buf[i];
    for (; j < num_chars; ++j)
    {
      if (p_chars[j] == this_char)
      {
        retval.found = 1;
        retval.index = i;
        retval.char_found = p_chars[j];
        return retval;
      }
    }
  }
  return retval;
}

struct str_locate_result
str_locate_text(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const char *p_text : itype(_Nt_array_ptr<const char>))
{
  struct str_locate_result retval;
  unsigned int i;
  unsigned int text_len = vsf_sysutil_strlen(p_text);
  _Nt_array_ptr<const char> p_text_len : count(text_len) = 0;
  _Unchecked {
    p_text_len = _Assume_bounds_cast<_Nt_array_ptr<const char>>(p_text, count(text_len));
  }
  retval.found = 0;
  retval.char_found = 0;
  retval.index = 0;
  if (text_len == 0 || text_len > p_str->len)
  {
    /* Not found */
    return retval;
  }
  for (i=0; i <= (p_str->len - text_len); i++)
  {
    _Array_ptr<char> p_buf_len : count(text_len) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_str->p_buf + i, count(text_len));
    if (vsf_sysutil_memcmp<char>(p_buf_len, p_text_len, text_len) == 0)
    {
      retval.found = 1;
      retval.index = i;
      return retval;
    }
  }
  /* Not found */
  return retval;
}

struct str_locate_result
str_locate_text_reverse(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const char *p_text : itype(_Nt_array_ptr<const char>))
{
  struct str_locate_result retval;
  unsigned int i;
  unsigned int text_len = vsf_sysutil_strlen(p_text);
  _Nt_array_ptr<const char> p_text_len : count(text_len) = 0;
  _Unchecked {
    p_text_len = _Assume_bounds_cast<_Nt_array_ptr<const char>>(p_text, count(text_len));
  }
  retval.found = 0;
  retval.char_found = 0;
  retval.index = 0;
  if (text_len == 0 || text_len > p_str->len)
  {
    return retval;
  }
  i = p_str->len - text_len;
  /* Want to go through loop once even if i==0 */
  while (1)
  {
    _Array_ptr<char> p_buf_len : count(text_len) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_str->p_buf + i, count(text_len));
    if (vsf_sysutil_memcmp<char>(p_buf_len, p_text_len, text_len) == 0)
    {
      retval.found = 1;
      retval.index = i;
      return retval;
    }
    if (i == 0)
    {
      break;
    }
    i--;
  }
  /* Not found */
  return retval;
}

void
str_left(const struct mystr *p_str : itype(_Ptr<const struct mystr>), struct mystr *p_out : itype(_Ptr<struct mystr>), unsigned int chars)
{
  if (chars > p_str->len)
  {
    bug("chars invalid in str_left");
  }

  _Array_ptr<char> p_buf_chars : count(chars) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_str->p_buf, count(chars));
  private_str_alloc_memchunk(p_out, p_buf_chars, chars);
}

void
str_right(const struct mystr *p_str : itype(_Ptr<const struct mystr>), struct mystr *p_out : itype(_Ptr<struct mystr>), unsigned int chars)
{
  unsigned int indexx = p_str->len - chars;
  if (chars > p_str->len)
  {
    bug("chars invalid in str_right");
  }
  _Array_ptr<char> p_buf_chars : count(chars) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_str->p_buf + indexx, count(chars));
  private_str_alloc_memchunk(p_out, p_buf_chars, chars);
}

void
str_mid_to_end(const struct mystr *p_str : itype(_Ptr<const struct mystr>), struct mystr *p_out : itype(_Ptr<struct mystr>), unsigned int indexx)
{
  if (indexx > p_str->len)
  {
    bug("invalid indexx in str_mid_to_end");
  }
  _Array_ptr<char> p_buf_chars : count(p_str->len - indexx) = _Dynamic_bounds_cast<_Array_ptr<char>>(p_str->p_buf + indexx, count(p_str->len - indexx));
  private_str_alloc_memchunk(p_out, p_buf_chars,
                             p_str->len - indexx);
}

char
str_get_char_at(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const unsigned int indexx)
{
  if (indexx >= p_str->len)
  {
    bug("bad indexx in str_get_char_at");
  }
  return p_str->p_buf[indexx];
}

int
str_contains_space(const struct mystr *p_str : itype(_Ptr<const struct mystr>))
{
  unsigned int i;
  for (i=0; i < p_str->len; i++)
  {
    if (vsf_sysutil_isspace(p_str->p_buf[i]))
    {
      return 1;
    }
  }
  return 0;
}

int
str_all_space(const struct mystr *p_str : itype(_Ptr<const struct mystr>))
{
  unsigned int i;
  for (i=0; i < p_str->len; i++)
  {
    if (!vsf_sysutil_isspace(p_str->p_buf[i]))
    {
      return 0;
    }
  }
  return 1;
}

int
str_contains_unprintable(const struct mystr *p_str : itype(_Ptr<const struct mystr>))
{
  unsigned int i;
  for (i=0; i < p_str->len; i++)
  {
    if (!vsf_sysutil_isprint(p_str->p_buf[i]))
    {
      return 1;
    }
  }
  return 0;
}

int
str_atoi(const struct mystr *p_str : itype(_Ptr<const struct mystr>))
{
  return vsf_sysutil_atoi(((_Nt_array_ptr<const char> )str_getbuf(p_str)));
}

filesize_t
str_a_to_filesize_t(const struct mystr *p_str : itype(_Ptr<const struct mystr>))
{
  return vsf_sysutil_a_to_filesize_t(((_Nt_array_ptr<const char> )str_getbuf(p_str)));
}

unsigned int
str_octal_to_uint(const struct mystr *p_str : itype(_Ptr<const struct mystr>))
{
  return vsf_sysutil_octal_to_uint(((_Nt_array_ptr<const char> )str_getbuf(p_str)));
}

int
str_getline(const struct mystr *p_str : itype(_Ptr<const struct mystr>), struct mystr *p_line_str : itype(_Ptr<struct mystr>), unsigned int *p_pos : itype(_Ptr<unsigned int>))
{
  unsigned int start_pos = *p_pos;
  unsigned int curr_pos = start_pos;
  unsigned int buf_len = str_getlen(p_str);
  _Nt_array_ptr<const char> p_buf : count(buf_len) = _Dynamic_bounds_cast<_Nt_array_ptr<const char>>(str_getbuf(p_str), count(buf_len));
  unsigned int out_len;
  if (start_pos > buf_len)
  {
    bug("p_pos out of range in str_getline");
  }
  str_empty(p_line_str);
  if (start_pos == buf_len)
  {
    return 0;
  }
  while (curr_pos < buf_len && p_buf[curr_pos] != '\n')
  {
    curr_pos++;
    if (curr_pos == 0)
    {
      bug("integer overflow");
    }
  }
  out_len = curr_pos - start_pos;
  /* If we ended on a \n - skip it */
  if (curr_pos < buf_len && p_buf[curr_pos] == '\n')
  {
    curr_pos++;
    if (curr_pos == 0)
    {
      bug("integer overflow");
    }
  }
  _Nt_array_ptr<const char> p_buf_outlen : count(out_len) = _Dynamic_bounds_cast<_Nt_array_ptr<const char>>(p_buf + start_pos, count(out_len));
  private_str_alloc_memchunk(p_line_str, p_buf_outlen, out_len);
  *p_pos = curr_pos;
  return 1;
}

int
str_contains_line(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const struct mystr *p_line_str : itype(_Ptr<const struct mystr>))
{
  static struct mystr s_curr_line_str = {};
  unsigned int pos = 0;
  while (str_getline(p_str, &s_curr_line_str, &pos))
  {
    if (str_equal(&s_curr_line_str, p_line_str))
    {
      return 1;
    }
  }
  return 0;
}

void
str_replace_unprintable(struct mystr *p_str : itype(_Ptr<struct mystr>), char new_char)
{
  unsigned int i;
  for (i=0; i < p_str->len; i++)
  {
    if (!vsf_sysutil_isprint(p_str->p_buf[i]))
    {
      p_str->p_buf[i] = new_char;
    }
  }
}

void
str_basename (struct mystr *d_str : itype(_Ptr<struct mystr>), const struct mystr *path : itype(_Ptr<const struct mystr>))
{
  static struct mystr tmp = {};

  str_copy (&tmp, path);
  str_split_char_reverse(&tmp, d_str, '/');

  if (str_isempty(d_str))
   str_copy (d_str, path);
}
