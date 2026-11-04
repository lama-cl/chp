/* Httpp.c
**
** http parsing engine
** 
** This program is distributed under the GNU General Public License, version 2.
** A copy of this license is included with this source.
*/

#ifdef HAVE_CONFIG_H
 #include <config.h>
#endif

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <avl/avl.h>
#include "httpp.h"

#if defined(_WIN32) && !defined(HAVE_STRCASECMP)
#define strcasecmp stricmp
#endif

#define MAX_HEADERS 32

/* internal functions */

/* misc */
static _Nt_array_ptr<char> _lowercase(_Nt_array_ptr<char> str);

/* for avl trees */
static int _compare_vars(void *compare_arg, void *a, void *b);
static int _free_vars(void *key);

http_parser_t *httpp_create_parser(void) : itype(_Ptr<http_parser_t>)
{
    return (_Ptr<http_parser_t>)malloc<http_parser_t>(sizeof(http_parser_t));
}

void httpp_initialize(http_parser_t *parser : itype(_Ptr<http_parser_t>), http_varlist_t *defaults : itype(_Ptr<http_varlist_t>))
{
    _Ptr<http_varlist_t> list = ((void *)0);

    parser->req_type = httpp_req_none;
    parser->uri = NULL;
    parser->vars = avl_tree_new<void>((_compare_vars), NULL);
    parser->queryvars = avl_tree_new<void>((_compare_vars), NULL);

    /* now insert the default variables */
    list = defaults;
    while (list != NULL) {
        httpp_setvar(parser, list->var.name, list->var.value);
        list = list->next;
    }
}

static int split_headers(_Nt_array_ptr<char> data : count(len), unsigned long len, _Array_ptr<_Nt_array_ptr<char>> line : count(32))
{
    /* first we count how many lines there are 
    ** and set up the line[] array     
    */
    int lines = 0;
    unsigned long i;
    line[lines] = data;
    for (i = 0; i < len && lines < MAX_HEADERS; i++) {
        if (data[i] == '\r')
            data[i] = '\0';
        if (data[i] == '\n') {
            lines++;
            data[i] = '\0';
            if (lines >= MAX_HEADERS)
                return MAX_HEADERS;
            if (i + 1 < len) {
                if (data[i + 1] == '\n' || data[i + 1] == '\r')
                    break;
                line[lines] = &data[i + 1];
            }
        }
    }

    i++;
    while (i < len && data[i] == '\n') i++;

    return lines;
}

static void parse_headers(_Ptr<http_parser_t> parser, _Array_ptr<_Nt_array_ptr<char>> line : count(32), int lines)
{
    int l;
    unsigned int i;
    int whitespace;
    _Nt_array_ptr<char> name = NULL;
    _Nt_array_ptr<char> value = NULL;

    /* parse the name: value lines. */
    for (l = 1; l < lines; l++) {
        whitespace = 0;
        name = line[l];
        value = NULL;
        _Nt_array_ptr<char> thisline = line[l];
        unsigned int slen = strlen(thisline) _Where thisline : bounds(thisline, thisline + slen);
        for (i = 0; i < slen; i++) {
            if (thisline[i] == ':') {
                whitespace = 1;
                thisline[i] = '\0';
            } else {
                if (whitespace) {
                    whitespace = 0;
                    while (i < slen && thisline[i] == ' ')
                        i++;

                    if (i < slen)
                        value = &thisline[i];
                    
                    break;
                }
            }
        }
        
        if (name != NULL && value != NULL) {
            httpp_setvar(parser, _lowercase(name), value);
            name = NULL; 
            value = NULL;
        }
    }
}

int httpp_parse_response(http_parser_t *parser : itype(_Ptr<http_parser_t>), const char *http_data : itype(_Array_ptr<const char>) count(4095), unsigned long len, const char *uri : itype(_Nt_array_ptr<const char>))
{
    _Nt_array_ptr<char> line _Checked[MAX_HEADERS] = {((void *)0)};
    int lines, slen,i, whitespace=0, where=0,code;
    _Nt_array_ptr<char> version =NULL;
_Nt_array_ptr<char> resp_code =NULL;
_Nt_array_ptr<char> message =NULL;

    
    if(http_data == NULL)
        return 0;

    /* make a local copy of the data, including 0 terminator */
    _Nt_array_ptr<char> data : count(len) = (_Nt_array_ptr<char>) malloc<char>(len+1);
    if (data == NULL) return 0;
    memcpy<char>(data, http_data, len);
    data[len] = 0;

    lines = split_headers(data, len, line);

    /* In this case, the first line contains:
     * VERSION RESPONSE_CODE MESSAGE, such as HTTP/1.0 200 OK
     */
    slen = strlen(line[0]);
    version = line[0];
    for(i=0; i < slen; i++) {
        if(line[0][i] == ' ') {
            line[0][i] = 0;
            whitespace = 1;
        } else if(whitespace) {
            whitespace = 0;
            where++;
            if(where == 1)
                resp_code = &line[0][i];
            else {
                message = &line[0][i];
                break;
            }
        }
    }

    if(version == NULL || resp_code == NULL || message == NULL) {
        free<char>(data);
        return 0;
    }

    httpp_setvar(parser, HTTPP_VAR_ERROR_CODE, resp_code);
    code = atoi(resp_code);
    if(code < 200 || code >= 300) {
        httpp_setvar(parser, HTTPP_VAR_ERROR_MESSAGE, message);
    }

    httpp_setvar(parser, HTTPP_VAR_URI, uri);
    httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "NONE");

    parse_headers(parser, line, lines);

    free<char>(data);

    return 1;
}

static int hex(char c)
{
    if(c >= '0' && c <= '9')
        return c - '0';
    else if(c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else if(c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else
        return -1;
}

static char *url_escape(_Nt_array_ptr<const char> src) : itype(_Nt_array_ptr<char>)
{
    int len = strlen(src);
    unsigned char *decoded;
    int i;
    char *dst;
    int done = 0;

    decoded = calloc<unsigned char>(1, len + 1);

    dst = (char *)decoded;

    for(i=0; i < len; i++) {
        switch(src[i]) {
        case '%':
            if(i+2 >= len) {
                free<unsigned char>(decoded);
                return NULL;
            }
            if(hex(src[i+1]) == -1 || hex(src[i+2]) == -1 ) {
                free<unsigned char>(decoded);
                return NULL;
            }

            *dst++ = hex(src[i+1]) * 16  + hex(src[i+2]);
            i+= 2;
            break;
        case '+':
            *dst++ = ' ';
            break;
        case '#':
            done = 1;
            break;
        case 0:
            free<unsigned char>(decoded);
            return NULL;
            break;
        default:
            *dst++ = src[i];
            break;
        }
        if(done)
            break;
    }

    *dst = 0; /* null terminator */

    return (char *)decoded;
}

/** TODO: This is almost certainly buggy in some cases */
static void parse_query(_Ptr<http_parser_t> parser, _Nt_array_ptr<char> query)
{
    int len;
    int i=0;
    _Nt_array_ptr<char> key = query;
    _Nt_array_ptr<char> val =NULL;

    if(!query || !*query)
        return;

    len = strlen(query);

    while(i<len) {
        switch(query[i]) {
        case '&':
            query[i] = 0;
            if(val && key)
                httpp_set_query_param(parser, key, val);
            key = query+i+1;
            break;
        case '=':
            query[i] = 0;
            val = query+i+1;
            break;
        }
        i++;
    }

    if(val && key) {
        httpp_set_query_param(parser, key, val);
    }
}

int httpp_parse(http_parser_t *parser : itype(_Ptr<http_parser_t>), const char *http_data : itype(_Array_ptr<const char>), unsigned long len)
{
_Nt_array_ptr<char> tmp = ((void *)0);

    _Nt_array_ptr<char> line _Checked[MAX_HEADERS] = {((void *)0)}; /* limited to 32 lines, should be more than enough */
    int i;
    int lines;
    _Nt_array_ptr<char> req_type = NULL;
    _Nt_array_ptr<char> uri = NULL;
    _Nt_array_ptr<char> version = NULL;
    int whitespace, where ;

    if (http_data == NULL)
        return 0;

    /* make a local copy of the data, including 0 terminator */
    _Nt_array_ptr<char> data : count(len) = (_Nt_array_ptr<char>)malloc<char>(len+1);
    if (data == NULL) return 0;
    memcpy<char>(data, http_data, len);
    data[len] = 0;

    lines = split_headers(data, len, line);

    /* parse the first line special
    ** the format is:
    ** REQ_TYPE URI VERSION
    ** eg:
    ** GET /index.html HTTP/1.0
    */
    where = 0;
    whitespace = 0;
    _Nt_array_ptr<char> thisline = line[0];
    size_t slen = strlen(thisline) _Where thisline : bounds(thisline, thisline + slen);
    req_type = line[0];
    for (i = 0; i < slen; i++) {
        if (thisline[i] == ' ') {
            whitespace = 1;
            thisline[i] = '\0';
        } else {
            /* we're just past the whitespace boundry */
            if (whitespace) {
                whitespace = 0;
                where++;
                switch (where) {
                case 1:
                    uri = &thisline[i];
                    break;
                case 2:
                    version = &thisline[i];
                    break;
                }
            }
        }
    }

    if (strcasecmp("GET", req_type) == 0) {
        parser->req_type = httpp_req_get;
    } else if (strcasecmp("POST", req_type) == 0) {
        parser->req_type = httpp_req_post;
    } else if (strcasecmp("PUT", req_type) == 0) {
        parser->req_type = httpp_req_put;
    } else if (strcasecmp("HEAD", req_type) == 0) {
        parser->req_type = httpp_req_head;
    } else if (strcasecmp("SOURCE", req_type) == 0) {
        parser->req_type = httpp_req_source;
    } else if (strcasecmp("PLAY", req_type) == 0) {
        parser->req_type = httpp_req_play;
    } else if (strcasecmp("STATS", req_type) == 0) {
        parser->req_type = httpp_req_stats;
    } else {
        parser->req_type = httpp_req_unknown;
    }

    if (uri != NULL && strlen(uri) > 0) {
        _Nt_array_ptr<char> query = ((void *)0);
        if((query = ((_Nt_array_ptr<char> )strchr(uri, '?'))) != NULL) {
            httpp_setvar(parser, HTTPP_VAR_RAWURI, uri);
            httpp_setvar(parser, HTTPP_VAR_QUERYARGS, query);
            *query = 0;
            query++;
            parse_query(parser, query);
        }

        parser->uri = ((_Nt_array_ptr<char> )strdup(uri));
    } else {
        free<char>(data);
        return 0;
    }

    if ((version != NULL) && ((tmp = ((_Nt_array_ptr<char> )strchr(version, '/'))) != NULL)) {
        tmp[0] = '\0';
        if ((strlen(version) > 0) && (strlen(&tmp[1]) > 0)) {
            httpp_setvar(parser, HTTPP_VAR_PROTOCOL, version);
            httpp_setvar(parser, HTTPP_VAR_VERSION, &tmp[1]);
        } else {
            free<char>(data);
            return 0;
        }
    } else {
        free<char>(data);
        return 0;
    }

    if (parser->req_type != httpp_req_none && parser->req_type != httpp_req_unknown) {
        switch (parser->req_type) {
        case httpp_req_get:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "GET");
            break;
        case httpp_req_post:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "POST");
            break;
        case httpp_req_put:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "PUT");
            break;
        case httpp_req_head:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "HEAD");
            break;
        case httpp_req_source:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "SOURCE");
            break;
        case httpp_req_play:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "PLAY");
            break;
        case httpp_req_stats:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "STATS");
            break;
        default:
            break;
        }
    } else {
        free<char>(data);
        return 0;
    }

    if (parser->uri != NULL) {
        httpp_setvar(parser, HTTPP_VAR_URI, parser->uri);
    } else {
        free<char>(data);
        return 0;
    }

    parse_headers(parser, line, lines);

    free<char>(data);

    return 1;
}

void httpp_deletevar(http_parser_t *parser : itype(_Ptr<http_parser_t>), const char *name : itype(_Nt_array_ptr<const char>))
{
    http_var_t var = {};

    if (parser == NULL || name == NULL)
        return;
    var.name = (_Nt_array_ptr<char>)name;
    var.value = NULL;
    avl_delete<void>(parser->vars, (void *)&var, (_free_vars));
}

void httpp_setvar(http_parser_t *parser : itype(_Ptr<http_parser_t>), const char *name : itype(_Nt_array_ptr<const char>), const char *value : itype(_Nt_array_ptr<const char>))
{
    http_var_t *var;

    if (name == NULL || value == NULL)
        return;

    var = (http_var_t *)malloc<http_var_t>(sizeof(http_var_t));
    if (var == NULL) return;

    var->name = ((_Nt_array_ptr<char> )strdup(name));
    var->value = ((_Nt_array_ptr<char> )strdup(value));

    if (httpp_getvar(parser, name) == NULL) {
        avl_insert<void>(parser->vars, (void *)var);
    } else {
        avl_delete<void>(parser->vars, (void *)var, (_free_vars));
        avl_insert<void>(parser->vars, (void *)var);
    }
}

const char *httpp_getvar(http_parser_t *parser : itype(_Ptr<http_parser_t>), const char *name : itype(_Nt_array_ptr<const char>)) : itype(_Nt_array_ptr<const char>)
{
    http_var_t var = {};
    http_var_t *found;
    void *fp;

    if (parser == NULL || name == NULL)
        return NULL;

    fp = &found;
    var.name = (_Nt_array_ptr<char>)name;
    var.value = NULL;

    if (avl_get_by_key<http_var_t>(parser->vars, &var, fp) == 0)
        return found->value;
    else
        return NULL;
}

void httpp_set_query_param(http_parser_t *parser : itype(_Ptr<http_parser_t>), const char *name : itype(_Nt_array_ptr<const char>), const char *value : itype(_Nt_array_ptr<const char>))
{
    http_var_t *var;

    if (name == NULL || value == NULL)
        return;

    var = (http_var_t *)malloc<http_var_t>(sizeof(http_var_t));
    if (var == NULL) return;

    var->name = ((_Nt_array_ptr<char> )strdup(name));
    var->value = ((_Nt_array_ptr<char> )url_escape(value));

    if (httpp_get_query_param(parser, name) == NULL) {
        avl_insert<void>(parser->queryvars, (void *)var);
    } else {
        avl_delete<void>(parser->queryvars, (void *)var, (_free_vars));
        avl_insert<void>(parser->queryvars, (void *)var);
    }
}

const char *httpp_get_query_param(http_parser_t *parser : itype(_Ptr<http_parser_t>), const char *name : itype(_Nt_array_ptr<const char>)) : itype(_Nt_array_ptr<const char>)
{
    http_var_t var = {};
    http_var_t *found;
    void *fp;

    fp = &found;
    var.name = (_Nt_array_ptr<char>)name;
    var.value = NULL;

    if (avl_get_by_key<void>(parser->queryvars, (void *)&var, fp) == 0)
        return found->value;
    else
        return NULL;
}

void httpp_clear(http_parser_t *parser : itype(_Ptr<http_parser_t>))
{
    parser->req_type = httpp_req_none;
    if (parser->uri)
        free<char>(parser->uri);
    parser->uri = NULL;
    avl_tree_free(parser->vars, (_free_vars));
    avl_tree_free(parser->queryvars, (_free_vars));
    parser->vars = NULL;
}

void httpp_destroy(http_parser_t *parser : itype(_Ptr<http_parser_t>))
{
    httpp_clear(parser);
    free<http_parser_t>(parser);
}

static _Nt_array_ptr<char> _lowercase(_Nt_array_ptr<char> str)
{
    _Nt_array_ptr<char> p = str;
    for (; *p != '\0'; p++)
        *p = tolower(*p);

    return str;
}

static int _compare_vars(void *compare_arg, void *a, void *b)
{
    http_var_t *vara, *varb;

    vara = (http_var_t *)a;
    varb = (http_var_t *)b;

    return strcmp(vara->name, varb->name);
}

static int _free_vars(void *key)
{
    http_var_t *var;

    var = (http_var_t *)key;

    if (var->name)
        free<char>(var->name);
    if (var->value)
        free<char>(var->value);
    free<http_var_t>(var);

    return 1;
}

