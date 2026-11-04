#ifndef VSFTP_STR_H
#define VSFTP_STR_H

/* TODO - document these functions ;-) */

#ifndef VSF_FILESIZE_H
#include "filesize.h"
#endif

struct mystr
{
  char* PRIVATE_HANDS_OFF_p_buf : itype(_Nt_array_ptr<char>) count(PRIVATE_HANDS_OFF_alloc_bytes);
  /* Internally, EXCLUDES trailing null */
  unsigned int PRIVATE_HANDS_OFF_len; 
  unsigned int PRIVATE_HANDS_OFF_alloc_bytes;
};

#define INIT_MYSTR \
  { (void*)0, 0, 0 }

#ifdef VSFTP_STRING_HELPER
#define str_alloc_memchunk private_str_alloc_memchunk
#endif
void private_str_alloc_memchunk(struct mystr *p_str : itype(_Ptr<struct mystr>), const char *p_src : itype(_Array_ptr<const char>) byte_count(len), unsigned int len);

void str_alloc_text(struct mystr *p_str : itype(_Ptr<struct mystr>), const char *p_src : itype(_Nt_array_ptr<const char>));
/* NOTE: String buffer data does NOT include terminating character */
void str_alloc_alt_term(struct mystr *p_str : itype(_Ptr<struct mystr>), const char *p_src : itype(_Array_ptr<const char>) count(len), char term, unsigned int len);
void str_alloc_ulong(struct mystr *p_str : itype(_Ptr<struct mystr>), unsigned long the_ulong);
void str_alloc_filesize_t(struct mystr *p_str : itype(_Ptr<struct mystr>), filesize_t the_filesize);
void str_copy(struct mystr *p_dest : itype(_Ptr<struct mystr>), const struct mystr *p_src : itype(_Ptr<const struct mystr>));
const char *str_strdup(const struct mystr *p_str : itype(_Ptr<const struct mystr>)) : itype(_Nt_array_ptr<const char>);
const char *str_strdup_trimmed(const struct mystr *p_str : itype(_Ptr<const struct mystr>)) : itype(_Nt_array_ptr<const char>);
void str_empty(struct mystr *p_str : itype(_Ptr<struct mystr>));
void str_free(struct mystr *p_str : itype(_Ptr<struct mystr>));
void str_trunc(struct mystr *p_str : itype(_Ptr<struct mystr>), unsigned int trunc_len);
void str_reserve(struct mystr *p_str : itype(_Ptr<struct mystr>), unsigned int res_len);

int str_isempty(const struct mystr *p_str : itype(_Ptr<const struct mystr>));
unsigned int str_getlen(const struct mystr *p_str : itype(_Ptr<const struct mystr>));
const char* str_getbuf(const struct mystr *p_str : itype(_Ptr<const struct mystr>)) : itype(_Nt_array_ptr<const char>) count(p_str->PRIVATE_HANDS_OFF_len);

int str_strcmp(const struct mystr *p_str1 : itype(_Ptr<const struct mystr>), const struct mystr *p_str2 : itype(_Ptr<const struct mystr>));
int str_equal(const struct mystr *p_str1 : itype(_Ptr<const struct mystr>), const struct mystr *p_str2 : itype(_Ptr<const struct mystr>));
int str_equal_text(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const char *p_text : itype(_Nt_array_ptr<const char>));

void str_append_str(struct mystr *p_str : itype(_Ptr<struct mystr>), const struct mystr *p_other : itype(_Ptr<const struct mystr>));
void str_append_text(struct mystr *p_str : itype(_Ptr<struct mystr>), const char *p_src : itype(_Nt_array_ptr<const char>));
void str_append_ulong(struct mystr *p_str : itype(_Ptr<struct mystr>), unsigned long the_long);
void str_append_filesize_t(struct mystr *p_str : itype(_Ptr<struct mystr>), filesize_t the_filesize);
void str_append_char(struct mystr *p_str : itype(_Ptr<struct mystr>), char the_char);
void str_append_double(struct mystr *p_str : itype(_Ptr<struct mystr>), double the_double);

void str_upper(struct mystr *p_str : itype(_Ptr<struct mystr>));
void str_rpad(struct mystr *p_str : itype(_Ptr<struct mystr>), const unsigned int min_width);
void str_lpad(struct mystr *p_str : itype(_Ptr<struct mystr>), const unsigned int min_width);
void str_replace_char(struct mystr *p_str : itype(_Ptr<struct mystr>), char from, char to);
void str_replace_text(struct mystr *p_str : itype(_Ptr<struct mystr>), const char *p_from : itype(_Nt_array_ptr<const char>), const char *p_to : itype(_Nt_array_ptr<const char>));

void str_split_char(struct mystr *p_src : itype(_Ptr<struct mystr>), struct mystr *p_rhs : itype(_Ptr<struct mystr>), char c);
void str_split_char_reverse(struct mystr *p_src : itype(_Ptr<struct mystr>), struct mystr *p_rhs : itype(_Ptr<struct mystr>), char c);
void str_split_text(struct mystr *p_src : itype(_Ptr<struct mystr>), struct mystr *p_rhs : itype(_Ptr<struct mystr>), const char *p_text : itype(_Nt_array_ptr<const char>));
void str_split_text_reverse(struct mystr *p_src : itype(_Ptr<struct mystr>), struct mystr *p_rhs : itype(_Ptr<struct mystr>), const char *p_text : itype(_Nt_array_ptr<const char>));

struct str_locate_result
{
  int found;
  unsigned int index;
  char char_found;
};

struct str_locate_result str_locate_char(const struct mystr *p_str : itype(_Ptr<const struct mystr>), char look_char);
struct str_locate_result str_locate_str(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const struct mystr *p_look_str : itype(_Ptr<const struct mystr>));
struct str_locate_result str_locate_str_reverse(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const struct mystr *p_look_str : itype(_Ptr<const struct mystr>));
struct str_locate_result str_locate_text(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const char *p_text : itype(_Nt_array_ptr<const char>));
struct str_locate_result str_locate_text_reverse(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const char *p_text : itype(_Nt_array_ptr<const char>));
struct str_locate_result str_locate_chars(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const char *p_chars : itype(_Nt_array_ptr<const char>));

void str_left(const struct mystr *p_str : itype(_Ptr<const struct mystr>), struct mystr *p_out : itype(_Ptr<struct mystr>), unsigned int chars);
void str_right(const struct mystr *p_str : itype(_Ptr<const struct mystr>), struct mystr *p_out : itype(_Ptr<struct mystr>), unsigned int chars);
void str_mid_to_end(const struct mystr *p_str : itype(_Ptr<const struct mystr>), struct mystr *p_out : itype(_Ptr<struct mystr>), unsigned int indexx);

char str_get_char_at(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const unsigned int indexx);
int str_contains_space(const struct mystr *p_str : itype(_Ptr<const struct mystr>));
int str_all_space(const struct mystr *p_str : itype(_Ptr<const struct mystr>));
int str_contains_unprintable(const struct mystr *p_str : itype(_Ptr<const struct mystr>));
void str_replace_unprintable(struct mystr *p_str : itype(_Ptr<struct mystr>), char new_char);
int str_atoi(const struct mystr *p_str : itype(_Ptr<const struct mystr>));
filesize_t str_a_to_filesize_t(const struct mystr *p_str : itype(_Ptr<const struct mystr>));
unsigned int str_octal_to_uint(const struct mystr *p_str : itype(_Ptr<const struct mystr>));
void str_basename (struct mystr *d_str : itype(_Ptr<struct mystr>), const struct mystr *path : itype(_Ptr<const struct mystr>));

/* PURPOSE: Extract a line of text (delimited by \n or EOF) from a string
 * buffer, starting at character position 'p_pos'. The extracted line will
 * not contain the '\n' terminator.
 *
 * RETURNS: 0 if no more lines are available, 1 otherwise.
 * The extracted text line is stored in 'p_line_str', which is
 * emptied if there are no more lines. 'p_pos' is updated to point to the
 * first character after the end of the line just extracted.
 */
int str_getline(const struct mystr *p_str : itype(_Ptr<const struct mystr>), struct mystr *p_line_str : itype(_Ptr<struct mystr>), unsigned int *p_pos : itype(_Ptr<unsigned int>));

/* PURPOSE: Detect whether or not a string buffer contains a specific line
 * of text (delimited by \n or EOF).
 *
 * RETURNS: 1 if there is a matching line, 0 otherwise.
 */
int str_contains_line(const struct mystr *p_str : itype(_Ptr<const struct mystr>), const struct mystr *p_line_str : itype(_Ptr<const struct mystr>));

#endif /* VSFTP_STR_H */

