/* ssi - server-side-includes CGI program
**
** Copyright © 1995 by Jef Poskanzer <jef@mail.acme.com>.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../checkedc_utils.h"

#include "config.h"
#include "match.h"


#define ST_GROUND 0
#define ST_LESSTHAN 1
#define ST_BANG 2
#define ST_MINUS1 3
#define ST_MINUS2 4

#pragma CHECKED_SCOPE on

static void read_file(char *vfilename : itype(_Nt_array_ptr<char>), char *filename : itype(_Nt_array_ptr<char>), FILE *fp : itype(_Ptr<FILE>));


static char *argv0 : itype(_Nt_array_ptr<char>) = ((void *)0);
static char* url : itype(_Nt_array_ptr<char>);

static char timefmt[100] : itype(char _Nt_checked[100]);
static int sizefmt;
#define SF_BYTES 0
#define SF_ABBREV 1
static struct stat sb;


_Checked static void
internal_error(char *reason : itype(_Nt_array_ptr<char>))
    {
    _Nt_array_ptr<char> title : byte_count(18) = "500 Internal Error";

    (void) printf( "\
<HTML><HEAD><TITLE>%s</TITLE></HEAD>\n\
<BODY><H2>%s</H2>\n\
Something unusual went wrong during a server-side-includes request:\n\
<BLOCKQUOTE>\n\
%s\n\
</BLOCKQUOTE>\n\
</BODY></HTML>\n", title, title, reason );
    }


_Checked static void
not_found(char *filename : itype(_Nt_array_ptr<char>))
    {
    _Nt_array_ptr<char> title : byte_count(13) = "404 Not Found";

    (void) printf( "\
<HTML><HEAD><TITLE>%s</TITLE></HEAD>\n\
<BODY><H2>%s</H2>\n\
The requested server-side-includes filename, %s,\n\
does not seem to exist.\n\
</BODY></HTML>\n", title, title, filename );
    }


_Checked static void
not_found2(char *directive : itype(_Nt_array_ptr<char>), char *tag : itype(_Nt_array_ptr<char>), char *filename2 : itype(_Nt_array_ptr<char>))
    {
    _Nt_array_ptr<char> title : byte_count(9) = "Not Found";

    (void) printf( "\
<HR><H2>%s</H2>\n\
The filename requested in a %s %s directive, %s,\n\
does not seem to exist.\n\
<HR>\n", title, directive, tag, filename2 );
    }


_Checked static void
not_permitted(char *directive : itype(_Nt_array_ptr<char>), char *tag : itype(_Nt_array_ptr<char>), char *val : itype(_Nt_array_ptr<char>))
    {
    _Nt_array_ptr<char> title : byte_count(13) = "Not Permitted";

    (void) printf( "\
<HR><H2>%s</H2>\n\
The filename requested in the %s %s=%s directive\n\
may not be fetched.\n\
<HR>\n", title, directive, tag, val );
    }


_Checked static void
unknown_directive(char *filename : itype(_Nt_array_ptr<char>), char *directive : itype(_Nt_array_ptr<char>))
    {
    _Nt_array_ptr<char> title : byte_count(17) = "Unknown Directive";

    (void) printf( "\
<HR><H2>%s</H2>\n\
The requested server-side-includes filename, %s,\n\
tried to use an unknown directive, %s.\n\
<HR>\n", title, filename, directive );
    }


_Checked static void
unknown_tag(char *filename : itype(_Nt_array_ptr<char>), char *directive : itype(_Nt_array_ptr<char>), char *tag : itype(_Nt_array_ptr<char>))
    {
    _Nt_array_ptr<char> title : byte_count(11) = "Unknown Tag";

    (void) printf( "\
<HR><H2>%s</H2>\n\
The requested server-side-includes filename, %s,\n\
tried to use the directive %s with an unknown tag, %s.\n\
<HR>\n", title, filename, directive, tag );
    }


_Checked static void
unknown_value(char *filename : itype(_Nt_array_ptr<char>), char *directive : itype(_Nt_array_ptr<char>), char *tag : itype(_Nt_array_ptr<char>), char *val : itype(_Nt_array_ptr<char>))
    {
    _Nt_array_ptr<char> title : byte_count(13) = "Unknown Value";

    (void) printf( "\
<HR><H2>%s</H2>\n\
The requested server-side-includes filename, %s,\n\
tried to use the directive %s %s with an unknown value, %s.\n\
<HR>\n", title, filename, directive, tag, val );
    }


_Checked static int
get_filename(char *_vfilename : itype(_Nt_array_ptr<char>), char *_filename : itype(_Nt_array_ptr<char>), char *directive : itype(_Nt_array_ptr<char>), char *tag : itype(_Nt_array_ptr<char>), char *val : itype(_Nt_array_ptr<char>), char *_fn : itype(_Nt_array_ptr<char>) count(fnsize), size_t fnsize)
    {
    _Nt_array_ptr<char> vfilename = _vfilename;
    _Nt_array_ptr<char> filename = _filename;
    _Nt_array_ptr<char> fn : count(fnsize) = _fn;

    /* Used for the various commands that accept a file name.
    ** These commands accept two tags:
    **   virtual
    **     Gives a virtual path to a document on the server.
    **   file
    **     Gives a pathname relative to the current directory. ../ cannot
    **     be used in this pathname, nor can absolute paths be used.
    */
    size_t vl = strlen( vfilename ) _Where vfilename : bounds(vfilename, vfilename + vl);
    size_t fl = strlen( filename ) _Where filename : bounds(filename, filename + fl);
    if ( strcmp( tag, "virtual" ) == 0 )
	{
	if ( strstr( val, "../" ) !=  0 )
	    {
	    not_permitted( directive, tag, val );
	    return -1;
	    }
	/* Figure out root using difference between vfilename and filename. */
        _Nt_array_ptr<char> tmp = _Dynamic_bounds_cast<_Nt_array_ptr<char>>(filename + (fl - vl), count(0));
        int res =  strcmp( vfilename, tmp);
	if ( vl > fl || res != 0 )
	    return -1;
	if ( fl - vl + strlen( val ) >= fnsize )
	    return -1;
        _Nt_array_ptr<char> fntmp : count(fl - vl) = _Dynamic_bounds_cast<_Nt_array_ptr<char>>(fn, count(fl - vl));
	(void) xstrbcpy( fntmp, filename, fl - vl );
        _Nt_array_ptr<char> fntmp2 : count(fnsize - (fl - vl)) = _Dynamic_bounds_cast<_Nt_array_ptr<char>>(fn + (fl - vl), count(fnsize - (fl - vl)));
	(void) xstrbcpy( fntmp2, val, fnsize - (fl - vl) );
	}
    else if ( strcmp( tag, "file" ) == 0 )
	{
	if ( val[0] == '/' || strstr( val, "../" ) !=  0 )
	    {
	    not_permitted( directive, tag, val );
	    return -1;
	    }
	if ( fl + 1 + strlen( val ) >= fnsize )
	    return -1;
	(void) xstrbcpy( fn, filename, fnsize );
        _Nt_array_ptr<char> cp : bounds(fn, fn + fnsize) = 0;
        _Unchecked {
          cp = _Assume_bounds_cast<_Nt_array_ptr<char>>(strrchr( fn, '/' ), bounds(fn, fn + fnsize));
        }
	if ( cp ==  0 )
	    {
            size_t fn_len = strlen(fn);
	    cp = fn + fn_len;
	    *cp = '/';
	    }
        _Nt_array_ptr<char> new_cp : bounds(fn,  fn + fnsize) = cp + 1;
        size_t cp_size = fnsize - (new_cp - fn) - 1;
        _Nt_array_ptr<char> tmp : count(cp_size) = _Dynamic_bounds_cast<_Nt_array_ptr<char>>(new_cp, count(cp_size));
	(void) xstrbcpy(tmp, val, cp_size);
	}
    else
	{
	unknown_tag( filename, directive, tag );
	return -1;
	}
    return 0;
    }


_Checked static int
check_filename(char *_filename : itype(_Nt_array_ptr<char>))
    {
    _Nt_array_ptr<char> filename = _filename;
    size_t fnl = strlen(filename) _Where filename : bounds(filename, filename + fnl);
    static int inited = 0;
    static _Nt_array_ptr<char> cgi_pattern = ((void *)0);
    _Nt_array_ptr<char> cp = ((void *)0);
    struct stat sb2;
    int r;

    if ( ! inited )
	{
	/* Get the cgi pattern. */
	cgi_pattern = ((_Nt_array_ptr<char> )getenv( "CGI_PATTERN" ));
#ifdef CGI_PATTERN
	if ( cgi_pattern == (char*) 0 )
	    cgi_pattern = CGI_PATTERN;
#endif /* CGI_PATTERN */
	inited = 1;
	}

    /* ../ is not permitted. */
    if ( strstr( filename, "../" ) != 0 )
	return 0;

#ifdef AUTH_FILE
    /* Ensure that we are not reading a basic auth password file. */
    _Nt_array_ptr<char> tmp = _Dynamic_bounds_cast<_Nt_array_ptr<char>>(filename + (fnl - sizeof(AUTH_FILE) + 1), count(0));
    if ( strcmp( filename, AUTH_FILE ) == 0 ||
	 ( fnl >= sizeof(AUTH_FILE) &&
	   strcmp( tmp, AUTH_FILE ) == 0 &&
	   filename[fnl - sizeof(AUTH_FILE)] == '/' ) )
	return 0;

    /* Check for an auth file in the same directory.  We can't do an actual
    ** auth password check here because CGI programs are not given the
    ** authorization header, for security reasons.  So instead we just
    ** prohibit access to all auth-protected files.
    */
    _Nt_array_ptr<char> dirname_tmp = strdup(filename);
    size_t dirname_len = strlen(dirname_tmp) _Where dirname_tmp : bounds(dirname_tmp, dirname_tmp + dirname_len);
    _Nt_array_ptr<char> dirname : count(fnl) = _Dynamic_bounds_cast<_Nt_array_ptr<char>>(dirname_tmp, count(fnl));
    if ( dirname ==  0 )
	return 0;	/* out of memory */
    cp = strrchr( dirname, '/' );
    if ( cp ==  0 )
	(void) xstrbcpy( dirname, ".", fnl );
    else
	*cp = '\0';

    size_t authsize = strlen( dirname ) + 1 + sizeof(AUTH_FILE);
    _Nt_array_ptr<char> authname : count(authsize) = ((void *)0);
    authname = ((_Nt_array_ptr<char> )malloc_nt( authsize ));
    if ( authname ==  0 )
	return 0;	/* out of memory */
    (void) xsbprintf( authname, authsize - 1, "%s/%s", dirname, AUTH_FILE );
    r = stat( authname, &sb2 );
    free<char>( dirname );
    free<char>( authname );
    if ( r == 0 )
	return 0;
#endif /* AUTH_FILE */

    /* Ensure that we are not reading a CGI file. */
    if ( cgi_pattern !=  0 && match( cgi_pattern, filename ) )
	return 0;

    return 1;
    }


_Checked static void
show_time( time_t t, int gmt )
    {
    _Ptr<struct tm> tmP = ((void *)0);
    char tbuf _Nt_checked[500];

    if ( gmt )
	tmP = gmtime( &t );
    else
	tmP = localtime( &t );
    if ( strftime( tbuf, sizeof(tbuf), timefmt, tmP ) > 0 )
	(void) fputs( tbuf, stdout );
    }


_Checked static void
show_size( off_t size )
    {
    switch ( sizefmt )
	{
	case SF_BYTES:
	(void) printf( "%ld", (long) size );  /* spec says should have commas */
	break;
	case SF_ABBREV:
	if ( size < 1024 )
	    (void) printf( "%ld", (long) size );
	else if ( size < 1024 )
	    (void) printf( "%ldK", (long) size / 1024L );
	else if ( size < 1024*1024 )
	    (void) printf( "%ldM", (long) size / (1024L*1024L) );
	else
	    (void) printf( "%ldG", (long) size / (1024L*1024L*1024L) );
	break;
	}
    }


_Checked static void
do_config(char *vfilename : itype(_Nt_array_ptr<char>), char *filename : itype(_Nt_array_ptr<char>), FILE *fp : itype(_Ptr<FILE>), char *directive : itype(_Nt_array_ptr<char>), char *tag : itype(_Nt_array_ptr<char>), char *val : itype(_Nt_array_ptr<char>))
    {
    /* The config directive controls various aspects of the file parsing.
    ** There are two valid tags:
    **   timefmt
    **     Gives the server a new format to use when providing dates.  This
    **     is a string compatible with the strftime library call.
    **   sizefmt
    **     Determines the formatting to be used when displaying the size of
    **     a file.  Valid choices are bytes, for a formatted byte count
    **     (formatted as 1,234,567), or abbrev for an abbreviated version
    **     displaying the number of kilobytes or megabytes the file occupies.
    */

    if ( strcmp( tag, "timefmt" ) == 0 )
	{
	(void) xstrbcpy( timefmt, val, sizeof(timefmt) - 1 );
	timefmt[sizeof(timefmt) - 1] = '\0';
	}
    else if ( strcmp( tag, "sizefmt" ) == 0 )
	{
	if ( strcmp( val, "bytes" ) == 0 )
	    sizefmt = SF_BYTES;
	else if ( strcmp( val, "abbrev" ) == 0 )
	    sizefmt = SF_ABBREV;
	else
	    unknown_value( filename, directive, tag, val );
	}
    else
	unknown_tag( filename, directive, tag );
    }


_Checked static void
do_include(char *vfilename : itype(_Nt_array_ptr<char>), char *filename : itype(_Nt_array_ptr<char>), FILE *fp : itype(_Ptr<FILE>), char *directive : itype(_Nt_array_ptr<char>), char *tag : itype(_Nt_array_ptr<char>), char *val : itype(_Nt_array_ptr<char>))
    {
    char vfilename2 _Nt_checked[1000];
    char filename2 _Nt_checked[1000];
    _Ptr<FILE> fp2 = ((void *)0);

    /* Inserts the text of another document into the parsed document. */

    if ( get_filename(
	     vfilename, filename, directive, tag, val, filename2,
	     sizeof(filename2) - 1 ) < 0 )
	return;

    if ( ! check_filename( filename2 ) )
	{
	not_permitted( directive, tag, filename2 );
	return;
	}

    fp2 = fopen( filename2, "r" );
    if ( fp2 ==  0 )
	{
	not_found2( directive, tag, filename2 );
	return;
	}

    if ( strcmp( tag, "virtual" ) == 0 )
	{
	if ( strlen( val ) < sizeof( vfilename2 ) )
	    (void) xstrbcpy( vfilename2, val, sizeof(vfilename2) - 1);
	else
	    (void) xstrbcpy( vfilename2, filename2, sizeof(vfilename2) - 1);  /* same size, has to fit */
	}
    else
	{
	if ( strlen( vfilename ) + 1 + strlen( val ) < sizeof(vfilename2) )
	    {
	    (void) xstrbcpy( vfilename2, vfilename, sizeof(vfilename2) - 1);

            _Nt_array_ptr<char> cp : bounds(vfilename2, vfilename2 + sizeof(vfilename2)) = 0;
            _Unchecked {
              cp = _Assume_bounds_cast<_Nt_array_ptr<char>>(strrchr( vfilename2, '/' ), bounds(vfilename2, vfilename2 + sizeof(vfilename2)));
            }

	    if ( cp == 0 )
		{
		cp = &vfilename2[strlen( vfilename2 )];
		*cp = '/';
		}
            cp++;
            size_t cp_count = sizeof(filename2) - 1 - (cp - vfilename2);
            _Nt_array_ptr<char> tmp : count(cp_count) = _Dynamic_bounds_cast<_Nt_array_ptr<char>>(cp, count(cp_count));
	    (void) xstrbcpy( tmp, val, cp_count );
	    }
	else
	    (void) xstrbcpy( vfilename2, filename2, sizeof(vfilename2) - 1);  /* same size, has to fit */
	}

    read_file( vfilename2, filename2, fp2 );
    (void) fclose( fp2 );
    }


_Checked static void
do_echo(char *vfilename : itype(_Nt_array_ptr<char>), char *filename : itype(_Nt_array_ptr<char>), FILE *fp : itype(_Ptr<FILE>), char *directive : itype(_Nt_array_ptr<char>), char *tag : itype(_Nt_array_ptr<char>), char *val : itype(_Nt_array_ptr<char>))
    {
    _Nt_array_ptr<char> cp = ((void *)0);
    time_t t;

    /* Prints the value of one of the include variables.  Any dates are
    ** printed subject to the currently configured timefmt.  The only valid
    ** tag is var, whose value is the name of the variable you wish to echo.
    */

    if ( strcmp( tag, "var" ) != 0 )
	unknown_tag( filename, directive, tag );
    else
	{
	if ( strcmp( val, "DOCUMENT_NAME" ) == 0 )
	    {
	    /* The current filename. */
	    (void) fputs( filename, stdout );
	    }
	else if ( strcmp( val, "DOCUMENT_URI" ) == 0 )
	    {
	    /* The virtual path to this file (such as /~robm/foo.shtml). */
	    (void) fputs( vfilename, stdout );
	    }
	else if ( strcmp( val, "QUERY_STRING_UNESCAPED" ) == 0 )
	    {
	    /* The unescaped version of any search query the client sent. */
	    cp = ((_Nt_array_ptr<char> )getenv( "QUERY_STRING" ));
	    if ( cp !=  0 )
		(void) fputs( cp, stdout );
	    }
	else if ( strcmp( val, "DATE_LOCAL" ) == 0 )
	    {
	    /* The current date, local time zone. */
	    t = time(  0 );
	    show_time( t, 0 );
	    }
	else if ( strcmp( val, "DATE_GMT" ) == 0 )
	    {
	    /* Same as DATE_LOCAL but in Greenwich mean time. */
	    t = time(  0 );
	    show_time( t, 1 );
	    }
	else if ( strcmp( val, "LAST_MODIFIED" ) == 0 )
	    {
	    /* The last modification date of the current document. */
	    if ( fstat( fileno( fp ), &sb ) >= 0 )
		show_time( sb.st_mtime, 0 );
	    }
	else
	    {
	    /* Try an environment variable. */
	    cp = ((_Nt_array_ptr<char> )getenv( val ));
	    if ( cp ==  0 )
		unknown_value( filename, directive, tag, val );
	    else
		(void) fputs( cp, stdout );
	    }
	}
    }


_Checked static void
do_fsize(char *vfilename : itype(_Nt_array_ptr<char>), char *filename : itype(_Nt_array_ptr<char>), FILE *fp : itype(_Ptr<FILE>), char *directive : itype(_Nt_array_ptr<char>), char *tag : itype(_Nt_array_ptr<char>), char *val : itype(_Nt_array_ptr<char>))
    {
    char filename2 _Nt_checked[1000];

    /* Prints the size of the specified file. */

    if ( get_filename(
	     vfilename, filename, directive, tag, val, filename2,
	     sizeof(filename2) - 1 ) < 0 )
	return;
    if ( stat( filename2, &sb ) < 0 )
	{
	not_found2( directive, tag, filename2 );
	return;
	}
    show_size( sb.st_size );
    }


_Checked static void
do_flastmod(char *vfilename : itype(_Nt_array_ptr<char>), char *filename : itype(_Nt_array_ptr<char>), FILE *fp : itype(_Ptr<FILE>), char *directive : itype(_Nt_array_ptr<char>), char *tag : itype(_Nt_array_ptr<char>), char *val : itype(_Nt_array_ptr<char>))
    {
    char filename2 _Nt_checked[1000];

    /* Prints the last modification date of the specified file. */

    if ( get_filename(
	     vfilename, filename, directive, tag, val, filename2,
	     sizeof(filename2) - 1 ) < 0 )
	return;
    if ( stat( filename2, &sb ) < 0 )
	{
	not_found2( directive, tag, filename2 );
	return;
	}
    show_time( sb.st_mtime, 0 );
    }


_Checked static void
parse(char *vfilename : itype(_Nt_array_ptr<char>), char *filename : itype(_Nt_array_ptr<char>), FILE *fp : itype(_Ptr<FILE>), char *str : itype(_Nt_array_ptr<char>))
    {
    _Nt_array_ptr<char> cp = ((void *)0);
    int ntags;
    _Nt_array_ptr<char> tags _Checked[200] = {((void *)0)};
    int dirn;
#define DI_CONFIG 0
#define DI_INCLUDE 1
#define DI_ECHO 2
#define DI_FSIZE 3
#define DI_FLASTMOD 4
    int i;
    _Nt_array_ptr<char> val = 0;

    _Nt_array_ptr<char> tmp_directive = str;
    size_t space_spn = strspn( tmp_directive, " \t\n\r" ) _Where tmp_directive : bounds(tmp_directive, tmp_directive + space_spn);
    _Nt_array_ptr<char> directive = tmp_directive + space_spn;

    ntags = 0;
    cp = directive;
    for (;;)
	{
	cp = ((_Nt_array_ptr<char> )strpbrk( cp, " \t\n\r\"" ));
	if ( cp == 0 )
	    break;
	if (*cp != '\0' &&  *cp == '"' )
	    {
	    cp = ((_Nt_array_ptr<char> )strpbrk( cp + 1, "\"" ));
	    ++cp;
	    if ( *cp == '\0' )
		break;
	    }
	*cp++ = '\0';
        _Nt_array_ptr<char> cp_tmp = cp;
        size_t cp_space_spn = strspn( cp_tmp, " \t\n\r" ) _Where cp_tmp : bounds(cp_tmp, cp_tmp + cp_space_spn);
	cp = cp_tmp + cp_space_spn;
	if ( *cp == '\0' )
	    break;
	if ( ntags < sizeof(tags)/sizeof(*tags) )
            {
	    tags[ntags] = cp;
            ntags++;
            } 
	}
    
    if ( strcmp( directive, "config" ) == 0 )
	dirn = DI_CONFIG;
    else if ( strcmp( directive, "include" ) == 0 )
	dirn = DI_INCLUDE;
    else if ( strcmp( directive, "echo" ) == 0 )
	dirn = DI_ECHO;
    else if ( strcmp( directive, "fsize" ) == 0 )
	dirn = DI_FSIZE;
    else if ( strcmp( directive, "flastmod" ) == 0 )
	dirn = DI_FLASTMOD;
    else
	{
	unknown_directive( filename, directive );
	return;
	}

    for ( i = 0; i < ntags; ++i )
	{
	if ( i > 0 )
	    putchar( ' ' );
	val = ((_Nt_array_ptr<char> )strchr( tags[i], '=' ));
	if ( val ==  0 )
	    val = "";
	else
            {
            *val = '\0';
            val++;
            }
	if ( *val != '\0' && *val == '"' && val[strlen( val ) - 1] == '"' )
	    {
	    val[strlen( val ) - 1] = '\0';
	    ++val;
	    }
	switch( dirn )
	    {
	    case DI_CONFIG:
	    do_config( vfilename, filename, fp, directive, tags[i], val );
	    break;
	    case DI_INCLUDE:
	    do_include( vfilename, filename, fp, directive, tags[i], val );
	    break;
	    case DI_ECHO:
	    do_echo( vfilename, filename, fp, directive, tags[i], val );
	    break;
	    case DI_FSIZE:
	    do_fsize( vfilename, filename, fp, directive, tags[i], val );
	    break;
	    case DI_FLASTMOD:
	    do_flastmod( vfilename, filename, fp, directive, tags[i], val );
	    break;
	    }
	}
    }


_Checked static void
slurp(char *vfilename : itype(_Nt_array_ptr<char>), char *filename : itype(_Nt_array_ptr<char>), FILE *fp : itype(_Ptr<FILE>))
    {
    char buf _Nt_checked[1000];
    int i;
    int state;
    int ich;

    /* Now slurp in the rest of the comment from the input file. */
    i = 0;
    state = ST_GROUND;
    while ( ( ich = getc( fp ) ) != EOF )
	{
	switch ( state )
	    {
	    case ST_GROUND:
	    if ( ich == '-' )
		state = ST_MINUS1;
	    break;
	    case ST_MINUS1:
	    if ( ich == '-' )
		state = ST_MINUS2;
	    else
		state = ST_GROUND;
	    break;
	    case ST_MINUS2:
	    if ( ich == '>' )
		{
		buf[i - 2] = '\0';
		parse( vfilename, filename, fp, buf );
		return;
		}
	    else if ( ich != '-' )
		state = ST_GROUND;
	    break;
	    }
	if ( i < sizeof(buf) - 1 )
	    buf[i++] = (char) ich;
	}
    }


_Checked static void
read_file(char *vfilename : itype(_Nt_array_ptr<char>), char *filename : itype(_Nt_array_ptr<char>), FILE *fp : itype(_Ptr<FILE>))
    {
    int ich;
    int state;

    /* Copy it to output, while running a state-machine to look for
    ** SSI directives.
    */
    state = ST_GROUND;
    while ( ( ich = getc( fp ) ) != EOF )
	{
	switch ( state )
	    {
	    case ST_GROUND:
	    if ( ich == '<' )
		{ state = ST_LESSTHAN; continue; }
	    break;
	    case ST_LESSTHAN:
	    if ( ich == '!' )
		{ state = ST_BANG; continue; }
	    else
		{ state = ST_GROUND; putchar( '<' ); }
	    break;
	    case ST_BANG:
	    if ( ich == '-' )
		{ state = ST_MINUS1; continue; }
	    else
		{ state = ST_GROUND; (void) fputs ( "<!", stdout ); }
	    break;
	    case ST_MINUS1:
	    if ( ich == '-' )
		{ state = ST_MINUS2; continue; }
	    else
		{ state = ST_GROUND; (void) fputs ( "<!-", stdout ); }
	    break;
	    case ST_MINUS2:
	    if ( ich == '#' )
		{
		slurp( vfilename, filename, fp );
		state = ST_GROUND;
		continue;
		}
	    else
		{ state = ST_GROUND; (void) fputs ( "<!--", stdout ); }
	    break;
	    }
	putchar( (char) ich );
	}
    }


_Checked int
main(int argc, char **argv : itype(_Array_ptr<_Nt_array_ptr<char>>) count(argc))
    {
    _Nt_array_ptr<char> script_name = ((void *)0);
    _Nt_array_ptr<char> path_info : byte_count(0) = ((void *)0);
    _Nt_array_ptr<char> path_translated = ((void *)0);
    _Ptr<FILE> fp = ((void *)0);

    argv0 = argv[0];

    /* Default formats. */
    (void) xstrbcpy( timefmt, "%a %b %e %T %Z %Y", sizeof(timefmt) - 1 );
    sizefmt = SF_BYTES;
    /* The MIME type has to be text/html. */
    (void) fputs( "Content-type: text/html\n\n", stdout );

    /* Get the name that we were run as. */
    script_name = ((_Nt_array_ptr<char> )getenv( "SCRIPT_NAME" ));
    if ( script_name ==  0 )
	{
	internal_error( "Couldn't get SCRIPT_NAME environment variable." );
	exit( 1 );
	}

    /* Append the PATH_INFO, if any, to get the full URL. */
    path_info = ((_Nt_array_ptr<char> )getenv( "PATH_INFO" ));
    if ( path_info ==  0 )
	path_info = "";
    size_t urlsize = strlen( script_name ) + strlen( path_info ) + 1;
    _Nt_array_ptr<char> local_url : count(urlsize) =  malloc_nt( urlsize  );
    url = local_url;
    if ( url ==  0 )
	{
	internal_error( "Out of memory." );
	exit( 1 );
	}
    (void) xsbprintf( local_url, urlsize, "%s%s", script_name, path_info );

    /* Get the name of the file to parse. */
    path_translated = ((_Nt_array_ptr<char> )getenv( "PATH_TRANSLATED" ));
    if ( path_translated ==  0 )
	{
	internal_error( "Couldn't get PATH_TRANSLATED environment variable." );
	exit( 1 );
	}

    if ( ! check_filename( path_translated ) )
	{
	not_permitted( "initial", "PATH_TRANSLATED", path_translated );
	exit( 1 );
	}

    /* Open it. */
    fp = fopen( path_translated, "r" );
    if ( fp ==  0 )
	{
	not_found( path_translated );
	exit( 1 );
	}

    /* Read and handle the file. */
    read_file( path_info, path_translated, fp );

    (void) fclose( fp );
    exit( 0 );
    }
