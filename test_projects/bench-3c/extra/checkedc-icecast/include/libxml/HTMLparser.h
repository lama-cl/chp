/*
 * Summary: interface for an HTML 4.0 non-verifying parser
 * Description: this module implements an HTML 4.0 non-verifying parser
 *              with API compatible with the XML parser ones. It should
 *              be able to parse "real world" HTML, even if severely
 *              broken from a specification point of view.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __HTML_PARSER_H__
#define __HTML_PARSER_H__
#include <libxml/xmlversion.h>
#include <libxml/parser.h>

#ifdef LIBXML_HTML_ENABLED

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Most of the back-end structures from XML and HTML are shared.
 */
typedef xmlParserCtxt htmlParserCtxt;
typedef xmlParserCtxtPtr htmlParserCtxtPtr;
typedef xmlParserNodeInfo htmlParserNodeInfo;
typedef xmlSAXHandler htmlSAXHandler;
typedef xmlSAXHandlerPtr htmlSAXHandlerPtr;
typedef xmlParserInput htmlParserInput;
typedef xmlParserInputPtr htmlParserInputPtr;
typedef xmlDocPtr htmlDocPtr;
typedef xmlNodePtr htmlNodePtr;

/*
 * Internal description of an HTML element, representing HTML 4.01
 * and XHTML 1.0 (which share the same structure).
 */
typedef struct _htmlElemDesc htmlElemDesc;
typedef htmlElemDesc *htmlElemDescPtr;
struct _htmlElemDesc {
    const char *name : itype(_Ptr<const char>);	/* The tag name */
    char startTag;      /* Whether the start tag can be implied */
    char endTag;        /* Whether the end tag can be implied */
    char saveEndTag;    /* Whether the end tag should be saved */
    char empty;         /* Is this an empty element ? */
    char depr;          /* Is this a deprecated element ? */
    char dtd;           /* 1: only in Loose DTD, 2: only Frameset one */
    char isinline;      /* is this a block 0 or inline 1 element */
    const char *desc : itype(_Ptr<const char>);   /* the description */

/* NRK Jan.2003
 * New fields encapsulating HTML structure
 *
 * Bugs:
 *	This is a very limited representation.  It fails to tell us when
 *	an element *requires* subelements (we only have whether they're
 *	allowed or not), and it doesn't tell us where CDATA and PCDATA
 *	are allowed.  Some element relationships are not fully represented:
 *	these are flagged with the word MODIFIER
 */
    const char **subelts : itype(_Ptr<_Ptr<const char>>);		/* allowed sub-elements of this element */
    const char *defaultsubelt : itype(_Ptr<const char>);	/* subelement for suggested auto-repair
					   if necessary or NULL */
    const char **attrs_opt : itype(_Ptr<_Ptr<const char>>);		/* Optional Attributes */
    const char **attrs_depr : itype(_Ptr<_Ptr<const char>>);		/* Additional deprecated attributes */
    const char **attrs_req : itype(_Ptr<_Ptr<const char>>);		/* Required attributes */
};

/*
 * Internal description of an HTML entity.
 */
typedef struct _htmlEntityDesc htmlEntityDesc;
typedef htmlEntityDesc *htmlEntityDescPtr;
struct _htmlEntityDesc {
    unsigned int value;	/* the UNICODE value for the character */
    const char *name : itype(_Ptr<const char>);	/* The entity name */
    const char *desc : itype(_Ptr<const char>);   /* the description */
};

/*
 * There is only few public functions.
 */
XMLPUBFUN const htmlElemDesc *htmlTagLookup(const xmlChar *tag : itype(_Ptr<const xmlChar>)) : itype(_Ptr<const htmlElemDesc>);
XMLPUBFUN const htmlEntityDesc *htmlEntityLookup(const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<const htmlEntityDesc>);
XMLPUBFUN const htmlEntityDesc *htmlEntityValueLookup(unsigned int value) : itype(_Ptr<const htmlEntityDesc>);

XMLPUBFUN int XMLCALL
			htmlIsAutoClosed(htmlDocPtr doc : itype(xmlDocPtr), htmlNodePtr elem : itype(xmlNodePtr));
XMLPUBFUN int XMLCALL
			htmlAutoCloseTag(htmlDocPtr doc : itype(xmlDocPtr), const xmlChar *name : itype(_Ptr<const xmlChar>), htmlNodePtr elem : itype(xmlNodePtr));
XMLPUBFUN const htmlEntityDesc *htmlParseEntityRef(htmlParserCtxtPtr ctxt : itype(xmlParserCtxtPtr), const xmlChar **str : itype(_Ptr<_Ptr<const xmlChar>>)) : itype(_Ptr<const htmlEntityDesc>);
XMLPUBFUN int XMLCALL
			htmlParseCharRef(htmlParserCtxtPtr ctxt : itype(xmlParserCtxtPtr));
XMLPUBFUN void XMLCALL
			htmlParseElement(htmlParserCtxtPtr ctxt : itype(xmlParserCtxtPtr));

XMLPUBFUN htmlParserCtxtPtr htmlNewParserCtxt(void) : itype(xmlParserCtxtPtr);

XMLPUBFUN htmlParserCtxtPtr htmlCreateMemoryParserCtxt(const char *buffer : itype(_Ptr<const char>), int size) : itype(xmlParserCtxtPtr);

XMLPUBFUN int XMLCALL
			htmlParseDocument(htmlParserCtxtPtr ctxt : itype(xmlParserCtxtPtr));
XMLPUBFUN htmlDocPtr htmlSAXParseDoc(const xmlChar *cur : itype(_Ptr<const xmlChar>), const char *encoding : itype(_Ptr<const char>), htmlSAXHandlerPtr sax : itype(xmlSAXHandlerPtr), void *userData) : itype(xmlDocPtr);
XMLPUBFUN htmlDocPtr htmlParseDoc(const xmlChar *cur : itype(_Ptr<const xmlChar>), const char *encoding : itype(_Ptr<const char>)) : itype(xmlDocPtr);
XMLPUBFUN htmlDocPtr htmlSAXParseFile(const char *filename : itype(_Ptr<const char>), const char *encoding : itype(_Ptr<const char>), htmlSAXHandlerPtr sax : itype(xmlSAXHandlerPtr), void *userData) : itype(xmlDocPtr);
XMLPUBFUN htmlDocPtr htmlParseFile(const char *filename : itype(_Ptr<const char>), const char *encoding : itype(_Ptr<const char>)) : itype(xmlDocPtr);
XMLPUBFUN int XMLCALL
			UTF8ToHtml	(unsigned char *out : itype(_Ptr<unsigned char>), int *outlen : itype(_Ptr<int>), const unsigned char *in : itype(_Ptr<const unsigned char>), int *inlen : itype(_Ptr<int>));
XMLPUBFUN int XMLCALL
			htmlEncodeEntities(unsigned char *out : itype(_Ptr<unsigned char>), int *outlen : itype(_Ptr<int>), const unsigned char *in : itype(_Ptr<const unsigned char>), int *inlen : itype(_Ptr<int>), int quoteChar);
XMLPUBFUN int XMLCALL
			htmlIsScriptAttribute(const xmlChar *name : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
			htmlHandleOmittedElem(int val);

#ifdef LIBXML_PUSH_ENABLED
/**
 * Interfaces for the Push mode.
 */
XMLPUBFUN htmlParserCtxtPtr htmlCreatePushParserCtxt(htmlSAXHandlerPtr sax : itype(xmlSAXHandlerPtr), void *user_data, const char *chunk : itype(_Ptr<const char>), int size, const char *filename : itype(_Ptr<const char>), xmlCharEncoding enc) : itype(xmlParserCtxtPtr);
XMLPUBFUN int XMLCALL
			htmlParseChunk		(htmlParserCtxtPtr ctxt : itype(xmlParserCtxtPtr), const char *chunk : itype(_Ptr<const char>), int size, int terminate);
#endif /* LIBXML_PUSH_ENABLED */

XMLPUBFUN void XMLCALL
			htmlFreeParserCtxt	(htmlParserCtxtPtr ctxt : itype(xmlParserCtxtPtr));

/*
 * New set of simpler/more flexible APIs
 */
/**
 * xmlParserOption:
 *
 * This is the set of XML parser options that can be passed down
 * to the xmlReadDoc() and similar calls.
 */
typedef enum {
    HTML_PARSE_RECOVER  = 1<<0, /* Relaxed parsing */
    HTML_PARSE_NODEFDTD = 1<<2, /* do not default a doctype if not found */
    HTML_PARSE_NOERROR	= 1<<5,	/* suppress error reports */
    HTML_PARSE_NOWARNING= 1<<6,	/* suppress warning reports */
    HTML_PARSE_PEDANTIC	= 1<<7,	/* pedantic error reporting */
    HTML_PARSE_NOBLANKS	= 1<<8,	/* remove blank nodes */
    HTML_PARSE_NONET	= 1<<11,/* Forbid network access */
    HTML_PARSE_NOIMPLIED= 1<<13,/* Do not add implied html/body... elements */
    HTML_PARSE_COMPACT  = 1<<16,/* compact small text nodes */
    HTML_PARSE_IGNORE_ENC=1<<21 /* ignore internal document encoding hint */
} htmlParserOption;

XMLPUBFUN void XMLCALL
		htmlCtxtReset		(htmlParserCtxtPtr ctxt : itype(xmlParserCtxtPtr));
XMLPUBFUN int XMLCALL
		htmlCtxtUseOptions	(htmlParserCtxtPtr ctxt : itype(xmlParserCtxtPtr), int options);
XMLPUBFUN htmlDocPtr htmlReadDoc(const xmlChar *cur : itype(_Ptr<const xmlChar>), const char *URL : itype(_Ptr<const char>), const char *encoding : itype(_Ptr<const char>), int options) : itype(xmlDocPtr);
XMLPUBFUN htmlDocPtr htmlReadFile(const char *URL : itype(_Ptr<const char>), const char *encoding : itype(_Ptr<const char>), int options) : itype(xmlDocPtr);
XMLPUBFUN htmlDocPtr htmlReadMemory(const char *buffer : itype(_Ptr<const char>), int size, const char *URL : itype(_Ptr<const char>), const char *encoding : itype(_Ptr<const char>), int options) : itype(xmlDocPtr);
XMLPUBFUN htmlDocPtr htmlReadFd(int fd, const char *URL : itype(_Ptr<const char>), const char *encoding : itype(_Ptr<const char>), int options) : itype(xmlDocPtr);
XMLPUBFUN htmlDocPtr htmlReadIO(xmlInputReadCallback ioread : itype(_Ptr<int (void *, char *, int)>), xmlInputCloseCallback ioclose : itype(_Ptr<int (void *)>), void *ioctx, const char *URL : itype(_Ptr<const char>), const char *encoding : itype(_Ptr<const char>), int options) : itype(xmlDocPtr);
XMLPUBFUN htmlDocPtr htmlCtxtReadDoc(xmlParserCtxtPtr ctxt : itype(_Ptr<xmlParserCtxt>), const xmlChar *cur : itype(_Ptr<const xmlChar>), const char *URL : itype(_Ptr<const char>), const char *encoding : itype(_Ptr<const char>), int options) : itype(xmlDocPtr);
XMLPUBFUN htmlDocPtr htmlCtxtReadFile(xmlParserCtxtPtr ctxt : itype(_Ptr<xmlParserCtxt>), const char *filename : itype(_Ptr<const char>), const char *encoding : itype(_Ptr<const char>), int options) : itype(xmlDocPtr);
XMLPUBFUN htmlDocPtr htmlCtxtReadMemory(xmlParserCtxtPtr ctxt : itype(_Ptr<xmlParserCtxt>), const char *buffer : itype(_Ptr<const char>), int size, const char *URL : itype(_Ptr<const char>), const char *encoding : itype(_Ptr<const char>), int options) : itype(xmlDocPtr);
XMLPUBFUN htmlDocPtr htmlCtxtReadFd(xmlParserCtxtPtr ctxt : itype(_Ptr<xmlParserCtxt>), int fd, const char *URL : itype(_Ptr<const char>), const char *encoding : itype(_Ptr<const char>), int options) : itype(xmlDocPtr);
XMLPUBFUN htmlDocPtr htmlCtxtReadIO(xmlParserCtxtPtr ctxt : itype(_Ptr<xmlParserCtxt>), xmlInputReadCallback ioread : itype(_Ptr<int (void *, char *, int)>), xmlInputCloseCallback ioclose : itype(_Ptr<int (void *)>), void *ioctx, const char *URL : itype(_Ptr<const char>), const char *encoding : itype(_Ptr<const char>), int options) : itype(xmlDocPtr);

/* NRK/Jan2003: further knowledge of HTML structure
 */
typedef enum {
  HTML_NA = 0 ,		/* something we don't check at all */
  HTML_INVALID = 0x1 ,
  HTML_DEPRECATED = 0x2 ,
  HTML_VALID = 0x4 ,
  HTML_REQUIRED = 0xc /* VALID bit set so ( & HTML_VALID ) is TRUE */
} htmlStatus ;

/* Using htmlElemDesc rather than name here, to emphasise the fact
   that otherwise there's a lookup overhead
*/
XMLPUBFUN htmlStatus XMLCALL htmlAttrAllowed(const htmlElemDesc * : itype(_Ptr<const htmlElemDesc>), const xmlChar * : itype(_Ptr<const xmlChar>), int) ;
XMLPUBFUN int XMLCALL htmlElementAllowedHere(const htmlElemDesc * : itype(_Ptr<const htmlElemDesc>), const xmlChar * : itype(_Ptr<const xmlChar>)) ;
XMLPUBFUN htmlStatus XMLCALL htmlElementStatusHere(const htmlElemDesc * : itype(_Ptr<const htmlElemDesc>), const htmlElemDesc * : itype(_Ptr<const htmlElemDesc>)) ;
XMLPUBFUN htmlStatus XMLCALL htmlNodeStatus(const htmlNodePtr : itype(const xmlNodePtr), int) ;
/**
 * htmlDefaultSubelement:
 * @elt: HTML element
 *
 * Returns the default subelement for this element
 */
#define htmlDefaultSubelement(elt) elt->defaultsubelt
/**
 * htmlElementAllowedHereDesc:
 * @parent: HTML parent element
 * @elt: HTML element
 *
 * Checks whether an HTML element description may be a
 * direct child of the specified element.
 *
 * Returns 1 if allowed; 0 otherwise.
 */
#define htmlElementAllowedHereDesc(parent,elt) \
	htmlElementAllowedHere((parent), (elt)->name)
/**
 * htmlRequiredAttrs:
 * @elt: HTML element
 *
 * Returns the attributes required for the specified element.
 */
#define htmlRequiredAttrs(elt) (elt)->attrs_req


#ifdef __cplusplus
}
#endif

#endif /* LIBXML_HTML_ENABLED */
#endif /* __HTML_PARSER_H__ */
