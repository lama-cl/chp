// This file is for code that is believed to be useful for Checked C programming
// in general (not just thttpd) and may eventually be moved to the Checked C
// system headers or a separate external library.

#ifndef _CHECKEDC_UTILS_H_
#define _CHECKEDC_UTILS_H_

#include <stddef.h> // for size_t

#pragma CHECKED_SCOPE push
#pragma CHECKED_SCOPE on

// `malloc` and `realloc` wrappers for _Nt_array_ptr that encapsulate the
// unchecked code to write the null terminator.

// For `char *` only because that's all we need and otherwise it's too much
// trouble to figure out how to insert the null terminator correctly.
char *malloc_nt(size_t size) : itype(_Nt_array_ptr<char>) count(size);

// Like the original `realloc`, this poses a temporal safety concern that's
// outside the scope of Checked C.
char *realloc_nt(char *ptr
                 : itype(_Nt_array_ptr<char>), size_t size)
    : itype(_Nt_array_ptr<char>) count(size);

// Recommended replacement functions for strcpy, strcat, and sprintf.
// These behave the same as strlcpy, strlcat, and snprintf except that:
//
// - The `size` argument does not include the null terminator. Checked C's
//   declaration of `snprintf` (which takes the size including the null
//   terminator) uses a complicated expression for the count that the current
//   compiler doesn't seem to be able to analyze, so we do this instead. The `b`
//   in the name (for "bound") is intended to indicate this API difference. (An
//   argument could be made to use `c` for Checked C's `count`, but `strccpy` is
//   a bit hard to read.)
//
// - These functions raise a runtime error on overflow. The `x` in the name is
//   intended to indicate this API difference.

size_t xstrbcpy(char *restrict dest
                : itype(restrict _Nt_array_ptr<char>) count(count),
                  const char *restrict src
                : itype(restrict _Nt_array_ptr<const char>), size_t count);

size_t xstrbcat(char *restrict dest
                : itype(restrict _Nt_array_ptr<char>) count(count),
                  const char *restrict src
                : itype(restrict _Nt_array_ptr<const char>), size_t count);

// _Unchecked only because of the varargs. Hopefully someday Checked C will
// allow varargs functions in checked regions if all the varargs are verified by
// -Wformat.
_Unchecked int xsbprintf(char *restrict s
                         : itype(restrict _Nt_array_ptr<char>) count(count),
                           size_t count, const char *restrict format
                         : itype(restrict _Nt_array_ptr<const char>), ...) __attribute__((format(printf, 3, 4)));

typedef struct {
  char *ptr : itype(_Nt_array_ptr<char>);
} nt_box;

_Nt_array_ptr<char> get_after_spn(_Nt_array_ptr<char> str, _Nt_array_ptr<char> search);
_Nt_array_ptr<char> get_after_cspn(_Nt_array_ptr<char> str, _Nt_array_ptr<char> search);

int __isxdigit(char c);
int __isdigit(char c);
int __isupper(char c);
int __tolower(char c);

#pragma CHECKED_SCOPE pop

#endif /* _CHECKEDC_UTILS_H_ */
