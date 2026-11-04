/* libhttpd.h - defines for libhttpd
**
** Copyright © 1995,1998,1999,2000,2001 by Jef Poskanzer <jef@mail.acme.com>.
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

#ifndef _LIBHTTPD_H_
#define _LIBHTTPD_H_

#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <sys/stat.h>

#if defined(AF_INET6) && defined(IN6_IS_ADDR_V4MAPPED)
#define USE_IPV6
#endif


/* A few convenient defines. */

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#define NEW(t,n) (malloc<t>( sizeof(t) * (n) ))
#define RENEW(o,t,n) (realloc<t>( o, sizeof(t) * (n) ))

/* Do overlapping strcpy safely, by using memmove. */
char *ol_strcpy(char *dst : itype(_Array_ptr<char>), char *src : itype(_Nt_array_ptr<char>)) : itype(_Ptr<char>);

/* The httpd structs. */

/* A multi-family sockaddr. */
typedef union {
    struct sockaddr sa;
    struct sockaddr_in sa_in;
#ifdef USE_IPV6
    struct sockaddr_in6 sa_in6;
    struct sockaddr_storage sa_stor;
#endif /* USE_IPV6 */
    } httpd_sockaddr;

/* A server. */
typedef struct {
    char* binding_hostname : itype(_Nt_array_ptr<char>);
    char* server_hostname : itype(_Nt_array_ptr<char>);
    unsigned short port;
    char* cgi_pattern : itype(_Nt_array_ptr<char>);
    int cgi_limit, cgi_count;
    char* charset : itype(_Nt_array_ptr<char>);
    char* p3p : itype(_Nt_array_ptr<char>);
    int max_age;
    char* cwd : itype(_Nt_array_ptr<char>);
    int listen4_fd, listen6_fd;
    int no_log;
    FILE* logfp : itype(_Ptr<FILE>);
    int no_symlink_check;
    int vhost;
    int global_passwd;
    char* url_pattern : itype(_Nt_array_ptr<char>);
    char* local_pattern : itype(_Nt_array_ptr<char>);
    int no_empty_referrers;
    } httpd_server;

/* A connection. */
typedef struct {
    int initialized;
    httpd_server* hs : itype(_Ptr<httpd_server>);
    httpd_sockaddr client_addr;
    char* read_buf : itype(_Nt_array_ptr<char>) count(read_size);
    size_t read_size, read_idx, checked_idx;
    int checked_state;
    int method;
    int status;
    off_t bytes_to_send;
    off_t bytes_sent;
    char* encodedurl : itype(_Nt_array_ptr<char>);
    char* decodedurl : itype(_Nt_array_ptr<char>) count(maxdecodedurl);
    char* protocol : itype(_Nt_array_ptr<char>);
    char* origfilename : itype(_Nt_array_ptr<char>) count(maxorigfilename);
    char* expnfilename : itype(_Nt_array_ptr<char>) count(maxexpnfilename);
    char* encodings : itype(_Nt_array_ptr<char>) count(maxencodings);
    char* pathinfo : itype(_Nt_array_ptr<char>) count(maxpathinfo);
    char* query : itype(_Nt_array_ptr<char>) count(maxquery);
    char* referrer : itype(_Nt_array_ptr<char>);
    char* useragent : itype(_Nt_array_ptr<char>);
    char* accept : itype(_Nt_array_ptr<char>) count(maxaccept);
    char* accepte : itype(_Nt_array_ptr<char>) count(maxaccepte);
    char* acceptl : itype(_Nt_array_ptr<char>);
    char* cookie : itype(_Nt_array_ptr<char>);
    char* contenttype : itype(_Nt_array_ptr<char>);
    char* reqhost : itype(_Nt_array_ptr<char>) count(maxreqhost);
    char* hdrhost : itype(_Nt_array_ptr<char>);
    char* hostdir : itype(_Nt_array_ptr<char>) count(maxhostdir);
    char* authorization : itype(_Nt_array_ptr<char>);
    char* remoteuser : itype(_Nt_array_ptr<char>) count(maxremoteuser);
    char* response : itype(_Nt_array_ptr<char>) count(maxresponse);
    size_t maxdecodedurl, maxorigfilename, maxexpnfilename, maxencodings,
	maxpathinfo, maxquery, maxaccept, maxaccepte, maxreqhost, maxhostdir,
	maxremoteuser, maxresponse;
#ifdef TILDE_MAP_2
    char* altdir;
    size_t maxaltdir;
#endif /* TILDE_MAP_2 */
    size_t responselen;
    time_t if_modified_since, range_if;
    size_t contentlength;
    char* type : itype(_Nt_array_ptr<char>);		/* not malloc()ed */
    char* hostname : itype(_Nt_array_ptr<char>);	/* not malloc()ed */
    int mime_flag;
    int one_one;	/* HTTP/1.1 or better */
    int got_range;
    int tildemapped;	/* this connection got tilde-mapped */
    off_t first_byte_index, last_byte_index;
    int keep_alive;
    int should_linger;
    struct stat sb;
    int conn_fd;
    char* file_address : itype(_Nt_array_ptr<char>);
    } httpd_conn;

/* Methods. */
#define METHOD_UNKNOWN 0
#define METHOD_GET 1
#define METHOD_HEAD 2
#define METHOD_POST 3
#define METHOD_PUT 4
#define METHOD_DELETE 5
#define METHOD_TRACE 6

/* States for checked_state. */
#define CHST_FIRSTWORD 0
#define CHST_FIRSTWS 1
#define CHST_SECONDWORD 2
#define CHST_SECONDWS 3
#define CHST_THIRDWORD 4
#define CHST_THIRDWS 5
#define CHST_LINE 6
#define CHST_LF 7
#define CHST_CR 8
#define CHST_CRLF 9
#define CHST_CRLFCR 10
#define CHST_BOGUS 11


/* Initializes.  Does the socket(), bind(), and listen().   Returns an
** httpd_server* which includes a socket fd that you can select() on.
** Return (httpd_server*) 0 on error.
*/
httpd_server *httpd_initialize(char *hostname : itype(_Nt_array_ptr<char>), httpd_sockaddr *sa4P : itype(_Ptr<httpd_sockaddr>), httpd_sockaddr *sa6P : itype(_Ptr<httpd_sockaddr>), unsigned short port, char *cgi_pattern : itype(_Nt_array_ptr<char>), int cgi_limit, char *charset : itype(_Nt_array_ptr<char>), char *p3p : itype(_Nt_array_ptr<char>), int max_age, char *cwd : itype(_Nt_array_ptr<char>) count(4096), int no_log, FILE *logfp : itype(_Ptr<FILE>), int no_symlink_check, int vhost, int global_passwd, char *url_pattern : itype(_Nt_array_ptr<char>), char *local_pattern : itype(_Nt_array_ptr<char>), int no_empty_referrers) : itype(_Ptr<httpd_server>);

/* Change the log file. */
void httpd_set_logfp(httpd_server *hs : itype(_Ptr<httpd_server>), FILE *logfp : itype(_Ptr<FILE>));

/* Call to unlisten/close socket(s) listening for new connections. */
void httpd_unlisten(httpd_server *hs : itype(_Ptr<httpd_server>));

/* Call to shut down. */
void httpd_terminate(httpd_server *hs : itype(_Ptr<httpd_server>));


/* When a listen fd is ready to read, call this.  It does the accept() and
** returns an httpd_conn* which includes the fd to read the request from and
** write the response to.  Returns an indication of whether the accept()
** failed, succeeded, or if there were no more connections to accept.
**
** In order to minimize malloc()s, the caller passes in the httpd_conn.
** The caller is also responsible for setting initialized to zero before the
** first call using each different httpd_conn.
*/
int httpd_get_conn(httpd_server *hs : itype(_Ptr<httpd_server>), int listen_fd, httpd_conn *hc : itype(_Ptr<httpd_conn>));
#define GC_FAIL 0
#define GC_OK 1
#define GC_NO_MORE 2

/* Checks whether the data in hc->read_buf constitutes a complete request
** yet.  The caller reads data into hc->read_buf[hc->read_idx] and advances
** hc->read_idx.  This routine checks what has been read so far, using
** hc->checked_idx and hc->checked_state to keep track, and returns an
** indication of whether there is no complete request yet, there is a
** complete request, or there won't be a valid request due to a syntax error.
*/
int httpd_got_request(httpd_conn *hc : itype(_Ptr<httpd_conn>));
#define GR_NO_REQUEST 0
#define GR_GOT_REQUEST 1
#define GR_BAD_REQUEST 2

/* Parses the request in hc->read_buf.  Fills in lots of fields in hc,
** like the URL and the various headers.
**
** Returns -1 on error.
*/
int httpd_parse_request(httpd_conn *hc : itype(_Ptr<httpd_conn>));

/* Starts sending data back to the client.  In some cases (directories,
** CGI programs), finishes sending by itself - in those cases, hc->file_fd
** is <0.  If there is more data to be sent, then hc->file_fd is a file
** descriptor for the file to send.  If you don't have a current timeval
** handy just pass in 0.
**
** Returns -1 on error.
*/
int httpd_start_request(httpd_conn *hc : itype(_Ptr<httpd_conn>), struct timeval *nowP : itype(_Ptr<struct timeval>));

/* Actually sends any buffered response text. */
void httpd_write_response(httpd_conn *hc : itype(_Ptr<httpd_conn>));

/* Call this to close down a connection and free the data.  A fine point,
** if you fork() with a connection open you should still call this in the
** parent process - the connection will stay open in the child.
** If you don't have a current timeval handy just pass in 0.
*/
void httpd_close_conn(httpd_conn *hc : itype(_Ptr<httpd_conn>), struct timeval *nowP : itype(_Ptr<struct timeval>));

/* Call this to de-initialize a connection struct and *really* free the
** mallocced strings.
*/
void httpd_destroy_conn(httpd_conn *hc : itype(_Ptr<httpd_conn>));


/* Send an error message back to the client. */
void httpd_send_err(httpd_conn *hc : itype(_Ptr<httpd_conn>), int status, char *title : itype(_Nt_array_ptr<char>), char *extraheads : itype(_Nt_array_ptr<char>) count(0), char *form : itype(_Nt_array_ptr<char>), char *arg : itype(_Nt_array_ptr<char>));

/* Some error messages. */
extern char* httpd_err400title : itype(_Nt_array_ptr<char>);
extern char* httpd_err400form : itype(_Nt_array_ptr<char>);
extern char* httpd_err408title : itype(_Nt_array_ptr<char>);
extern char* httpd_err408form : itype(_Nt_array_ptr<char>);
extern char* httpd_err503title : itype(_Nt_array_ptr<char>);
extern char* httpd_err503form : itype(_Nt_array_ptr<char>);

/* Generate a string representation of a method number. */
char *httpd_method_str(int method) : itype(_Nt_array_ptr<char>);

/* Reallocate a string. */
void httpd_realloc_str(char **strP : itype(_Ptr<_Nt_array_ptr<char>>), size_t *maxsizeP : itype(_Ptr<size_t>), size_t size);

// Test the new Checked C way starting at one call site.

struct strbuf {
  _Nt_array_ptr<char> str : count(maxsize);
  size_t maxsize;
};

// The returned pointer is the same one stored in the structure, but with a
// promise that it's as big as the caller requested.
_Nt_array_ptr<char> httpd_realloc_strbuf(_Ptr<struct strbuf> sbuf, size_t size) : count(size);

// httpd_realloc_str_ccl(tmp_foo, stored_foo, stored_maxfoo, size):
//
// Expand `stored_foo` to at least `size` and update `stored_maxfoo` to the
// allocated size. Also generate a local variable `_Nt_array_ptr<char> tmp_foo :
// count(size)` containing the same pointer as `stored_foo`. In the name, `cc`
// stands for "Checked C" and `l` stands for "local".

// `L` means lvalue.
//
// It doesn't seem we can do a `( _Checked { ... })` expression statement, so do
// the best we can.
//
// Hm, while we want the _Checked annotations long-term, they seem to block 3C
// from starting. So disable them for the moment.
#define httpd_realloc_str_ccl(_tmp_str_var, _strL, _maxsizeL, _size) \
  _Nt_array_ptr<char> _tmp_str_var : count(_size) = 0; \
  /*_Checked*/ { \
    struct strbuf _sbuf = {_strL, _maxsizeL}; \
    _tmp_str_var = httpd_realloc_strbuf(&_sbuf, _size); \
    _maxsizeL = _sbuf.maxsize, _strL = _sbuf.str; /* BOUNDS WARNING REVIEWED */ \
  }

// Same but don't generate the local variable. Use if you're going to access
// _strL directly using _maxsizeL as a bound or you just aren't going to access
// the buffer at all yet.
#define httpd_realloc_str_cc(_strL, _maxsizeL, _size) \
  /*_Checked*/ { \
    struct strbuf _sbuf = {_strL, _maxsizeL}; \
    httpd_realloc_strbuf(&_sbuf, _size); \
    _maxsizeL = _sbuf.maxsize, _strL = _sbuf.str; /* BOUNDS WARNING REVIEWED */ \
  }

/* Format a network socket to a string representation. */
char *httpd_ntoa(httpd_sockaddr *saP : itype(_Ptr<httpd_sockaddr>)) : itype(_Nt_array_ptr<char>);

/* Set NDELAY mode on a socket. */
void httpd_set_ndelay( int fd );

/* Clear NDELAY mode on a socket. */
void httpd_clear_ndelay( int fd );

/* Read the requested buffer completely, accounting for interruptions. */
_Itype_for_any(T) int httpd_read_fully( int fd, void* buf : itype(_Array_ptr<T>) byte_count(nbytes), size_t nbytes );

/* Write the requested buffer completely, accounting for interruptions. */
int httpd_write_fully(int fd, const char *buf : itype(_Array_ptr<const char>) count(nbytes), size_t nbytes);

/* Generate debugging statistics syslog message. */
void httpd_logstats( long secs );

#endif /* _LIBHTTPD_H_ */
