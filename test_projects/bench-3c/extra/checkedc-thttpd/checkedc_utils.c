#include "checkedc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#pragma CHECKED_SCOPE on

char *malloc_nt(size_t size)
    : itype(_Nt_array_ptr<char>) count(size) _Unchecked {
  size_t raw_size = size + 1;
  _Array_ptr<char> buf
      : count(raw_size) =
            malloc<char>(raw_size * sizeof(char)); /* BOUNDS WARNING REVIEWED */
  buf[size] = '\0';
  return _Assume_bounds_cast<_Nt_array_ptr<char>>(buf, count(size));
}

char *realloc_nt(char *ptr
                 : itype(_Nt_array_ptr<char>), size_t size)
    : itype(_Nt_array_ptr<char>) count(size) _Unchecked {
  size_t raw_size = size + 1;
  _Array_ptr<char> buf
      : count(raw_size) = realloc<char>(
            ptr, raw_size * sizeof(char)); /* BOUNDS WARNING REVIEWED */
  buf[size] = '\0';
  return _Assume_bounds_cast<_Nt_array_ptr<char>>(buf, count(size));
}

// If we had implementations of the original strlcpy and strlcat (e.g., from
// libbsd), then xstrbcpy and xstrbcat could be simple wrappers like xsbprintf.
// But we don't, so instead, let's use this as an interesting exercise in trying
// to implement them in fully Checked C. They could probably use some
// optimization, but that's not the point.

// The `size` parameter is not read by our code; it is used only by the Checked
// C compiler to automatically generate the correct runtime check for overflow
// when we write to `dest` (so we don't have to do any work ourselves to
// implement this part of the API!).
size_t xstrbcpy(char *restrict dest
                : itype(restrict _Nt_array_ptr<char>) count(size),
                  const char *restrict src
                : itype(restrict _Nt_array_ptr<const char>), size_t size) {
  size_t i = 0;

  // We have to set up a new pointer to `src` with `i` as the bound, otherwise
  // the compiler inserts a runtime check that will fail as soon as we access an
  // offset greater than the original bound of 0. This may be surprising to new
  // Checked C programmers; arguably the compiler should issue a warning.

  // Compiler warning that goes away if I remove `restrict` from `src`: a bug?
  //
  //_Nt_array_ptr<const char> src1 = src;
  //
  // For now, suppress the warning with a _Dynamic_bounds_cast to reduce
  // distraction. I'm not sure what's the lesser evil: a runtime check that
  // could fail or a warning suppression that could become inappropriate (once
  // we set up a way to do that at all).
  _Nt_array_ptr<const char> src1 =
      _Dynamic_bounds_cast<_Nt_array_ptr<const char>>(src, count(0));

  // The compiler doesn't seem to know that this is safe as long as `i` remains
  // 0. Suppress the error with a _Dynamic_bounds_cast.
  //
  //_Nt_array_ptr<const char> src2 : count(i) = src1;
  _Nt_array_ptr<const char> src2
      : count(i) =
            _Dynamic_bounds_cast<_Nt_array_ptr<const char>>(src1, count(i));

  // Copy the data. If the destination overflows, Checked C will raise a runtime
  // error for us based on the declared bound.
  for (; src2[i] != '\0'; i++) {
    dest[i] = src2[i];
  }

  // Here i might equal the bound of dest. The compiler lets us write exactly
  // at the bound _if_ the character being written is null, otherwise it's a
  // runtime error.
  dest[i] = '\0';

  // Here, normal strlcpy would finish computing the length of the source string
  // for the return value, but we know we would have already hit a runtime error
  // if the source string were any longer.
  return i;
}

size_t xstrbcat(char *restrict dest
                : itype(restrict _Nt_array_ptr<char>) count(size),
                  const char *restrict src
                : itype(restrict _Nt_array_ptr<const char>), size_t size) {
  // For code below that is analogous to code in xstrbcpy, the same comments
  // apply.
  _Dynamic_check(size > 0);
  size_t dest_i;
  for (dest_i = 0; dest[dest_i] != '\0'; dest_i++)
    ;
  size_t src_i = 0;
  _Nt_array_ptr<const char> src1 =
      _Dynamic_bounds_cast<_Nt_array_ptr<const char>>(src, count(0));
  _Nt_array_ptr<const char> src2
      : count(src_i) =
            _Dynamic_bounds_cast<_Nt_array_ptr<const char>>(src1, count(src_i));
  // If we reached the given size without hitting a null terminator in dest,
  // then regular strlcat would return `size`, but we will have already failed a
  // runtime check.
  for (; src2[src_i] != '\0'; dest_i++, src_i++) {
    dest[dest_i] = src2[src_i];
  }
  dest[dest_i] = '\0';
  return dest_i;
}

_Unchecked int xsbprintf(char *restrict s
                         : itype(restrict _Nt_array_ptr<char>) count(size),
                           size_t size, const char *restrict format
                         : itype(restrict _Nt_array_ptr<const char>), ...) {
  // Wrapper; nothing interesting here except the overflow check at the end.
  va_list ap;
  int ret;
  va_start(ap, format);
  ret = vsnprintf(s, size + 1, format, ap);
  va_end(ap);
  _Dynamic_check(ret <= size);
  return ret;
}

_Checked _Nt_array_ptr<char> get_after_spn(_Nt_array_ptr<char> str, _Nt_array_ptr<char> search) {
  size_t spn = strspn(str, search) _Where str : bounds(str, str + spn);
  _Nt_array_ptr<char> out : bounds(str, str + spn) = str + spn;
  return _Dynamic_bounds_cast<_Nt_array_ptr<char>>(out, count(0));
}

_Nt_array_ptr<char> get_after_cspn(_Nt_array_ptr<char> str, _Nt_array_ptr<char> search) {
  size_t spn = strcspn(str, search) _Where str : bounds(str, str + spn);
  _Nt_array_ptr<char> out : bounds(str, str + spn) = str + spn;
  return _Dynamic_bounds_cast<_Nt_array_ptr<char>>(out, count(0));
}

int __isxdigit(char c) _Unchecked {
  return isxdigit(c);
}

int __isdigit(char c) _Unchecked {
  return isdigit(c);
}

int __isupper(char c) _Unchecked {
  return isupper(c);
}

int __tolower(char c) _Unchecked {
  return tolower(c);
}

