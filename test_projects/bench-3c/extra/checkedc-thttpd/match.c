/* match.c - simple shell-style filename matcher
**
** Only does ? * and **, and multiple patterns separated by |.  Returns 1 or 0.
**
** Copyright © 1995,2000 by Jef Poskanzer <jef@mail.acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/


#include <string.h>

#include "match.h"

#pragma CHECKED_SCOPE on

static int match_one(const char *pattern : itype(_Array_ptr<const char>) count(patternlen), unsigned int patternlen, const char *string : itype(_Nt_array_ptr<const char>));

_Checked int
match(const char *pattern : itype(_Nt_array_ptr<const char>), const char *string : itype(_Nt_array_ptr<const char>))
    {
    for (;;)
	{
	_Nt_array_ptr<const char> or = strchr( pattern, '|' );
	if ( or == 0 ) {
            unsigned int slen = strlen(pattern);
            _Array_ptr<char> tmp_p : count(slen) = 0;
            _Unchecked { tmp_p = _Assume_bounds_cast<_Array_ptr<char>>(pattern, count(slen)); }
	    return match_one( tmp_p, slen, string );
        }

        unsigned int len = or - pattern;
        _Array_ptr<char> tmp : count(len) = _Dynamic_bounds_cast<_Array_ptr<char>>(pattern, count(len));
	if ( match_one( tmp, len, string ) )
	    return 1;

	pattern = or + 1;
	}
    }


_Checked static int
match_one(const char *pattern : itype(_Array_ptr<const char>) count(patternlen), unsigned int patternlen, const char *string : itype(_Nt_array_ptr<const char>))
    {
    _Array_ptr<const char> __3c_tmp_p : count(patternlen) = ((void *)0);
    _Array_ptr<const char> p : bounds(__3c_tmp_p, __3c_tmp_p + patternlen) = __3c_tmp_p;

    for ( __3c_tmp_p = pattern, p = __3c_tmp_p; p - pattern < patternlen; ++p, ++string )
	{
	if ( *p == '?' && *string != '\0' )
	    continue;
	if ( *p == '*' )
	    {
            unsigned int pl;
            int i;
	    ++p;
	    if ( *p == '*' )
		{
		/* Double-wildcard matches anything. */
		++p;
		i = strlen( string );
		}
	    else
		/* Single-wildcard matches anything but slash. */
		i = strcspn( string, "/" );
	    pl = patternlen - ( p - pattern );
	    for ( ; i >= 0; --i )
                {
                _Array_ptr<char> tmp_p : count(pl) = _Dynamic_bounds_cast<_Array_ptr<char>>(p, count(pl));
                _Nt_array_ptr<const char> tmp_str = string + i;
		if ( match_one( tmp_p, pl, tmp_str ) )
		    return 1;
                }
	    return 0;
	    }
	if ( *p != *string )
	    return 0;
	}
    if ( *string == '\0' )
	return 1;
    return 0;
    }
