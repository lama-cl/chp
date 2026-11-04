/**
 * Summary: library of generic URI related routines
 * Description: library of generic URI related routines
 *              Implements RFC 2396
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_URI_H__
#define __XML_URI_H__

#include <libxml/xmlversion.h>
#include <libxml/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * xmlURI:
 *
 * A parsed URI reference. This is a struct containing the various fields
 * as described in RFC 2396 but separated for further processing.
 *
 * Note: query is a deprecated field which is incorrectly unescaped.
 * query_raw takes precedence over query if the former is set.
 * See: http://mail.gnome.org/archives/xml/2007-April/thread.html#00127
 */
typedef struct _xmlURI xmlURI;
typedef xmlURI *xmlURIPtr;
struct _xmlURI {
    char *scheme : itype(_Ptr<char>);	/* the URI scheme */
    char *opaque : itype(_Ptr<char>);	/* opaque part */
    char *authority : itype(_Ptr<char>);	/* the authority part */
    char *server : itype(_Nt_array_ptr<char>);	/* the server part */
    char *user : itype(_Ptr<char>);		/* the user part */
    int port;		/* the port number */
    char *path : itype(_Nt_array_ptr<char>);		/* the path string */
    char *query : itype(_Ptr<char>);	/* the query string (deprecated - use with caution) */
    char *fragment : itype(_Ptr<char>);	/* the fragment identifier */
    int  cleanup;	/* parsing potentially unclean URI */
    char *query_raw : itype(_Ptr<char>);	/* the query string (as it appears in the URI) */
};

/*
 * This function is in tree.h:
 * xmlChar *	xmlNodeGetBase	(xmlDocPtr doc,
 *                               xmlNodePtr cur);
 */
XMLPUBFUN xmlURIPtr xmlCreateURI(void) : itype(_Ptr<xmlURI>);
XMLPUBFUN xmlChar *xmlBuildURI(const xmlChar *URI : itype(_Ptr<const xmlChar>), const xmlChar *base : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlBuildRelativeURI(const xmlChar *URI : itype(_Ptr<const xmlChar>), const xmlChar *base : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlURIPtr xmlParseURI(const char *str : itype(_Ptr<const char>)) : itype(_Ptr<xmlURI>);
XMLPUBFUN xmlURIPtr xmlParseURIRaw(const char *str : itype(_Ptr<const char>), int raw) : itype(_Ptr<xmlURI>);
XMLPUBFUN int XMLCALL
		xmlParseURIReference	(xmlURIPtr uri : itype(_Ptr<xmlURI>), const char *str : itype(_Ptr<const char>));
XMLPUBFUN xmlChar *xmlSaveUri(xmlURIPtr uri : itype(_Ptr<xmlURI>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN void XMLCALL
		xmlPrintURI		(FILE *stream : itype(_Ptr<FILE>), xmlURIPtr uri : itype(_Ptr<xmlURI>));
XMLPUBFUN xmlChar *xmlURIEscapeStr(const xmlChar *str : itype(_Ptr<const xmlChar>), const xmlChar *list : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN char *xmlURIUnescapeString(const char *str : itype(_Ptr<const char>), int len, char *target : itype(_Ptr<char>)) : itype(_Ptr<char>);
XMLPUBFUN int XMLCALL
		xmlNormalizeURIPath	(char *path : itype(_Ptr<char>));
XMLPUBFUN xmlChar *xmlURIEscape(const xmlChar *str : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN void XMLCALL
		xmlFreeURI		(xmlURIPtr uri : itype(_Ptr<xmlURI>));
XMLPUBFUN xmlChar *xmlCanonicPath(const xmlChar *path : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlPathToURI(const xmlChar *path : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);

#ifdef __cplusplus
}
#endif
#endif /* __XML_URI_H__ */
