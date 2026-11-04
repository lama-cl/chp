/*
 * Summary: set of utilities for the XSLT engine
 * Description: interfaces for the utilities module of the XSLT engine.
 *              things like message handling, profiling, and other
 *              generally useful routines.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_XSLTUTILS_H__
#define __XML_XSLTUTILS_H__

#include <libxslt/xsltconfig.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#include <libxml/xpath.h>
#include <libxml/dict.h>
#include <libxml/xmlerror.h>
#include "xsltexports.h"
#include "xsltInternals.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * XSLT_TODO:
 *
 * Macro to flag unimplemented blocks.
 */
#define XSLT_TODO							\
    xsltGenericError(xsltGenericErrorContext,				\
	    "Unimplemented block at %s:%d\n",				\
            __FILE__, __LINE__);

/**
 * XSLT_STRANGE:
 *
 * Macro to flag that a problem was detected internally.
 */
#define XSLT_STRANGE							\
    xsltGenericError(xsltGenericErrorContext,				\
	    "Internal error at %s:%d\n",				\
            __FILE__, __LINE__);

/**
 * IS_XSLT_ELEM:
 *
 * Checks that the element pertains to XSLT namespace.
 */
#define IS_XSLT_ELEM(n)							\
    (((n) != NULL) && ((n)->type == XML_ELEMENT_NODE) &&                \
     ((n)->ns != NULL) && (xmlStrEqual((n)->ns->href, XSLT_NAMESPACE)))

/**
 * IS_XSLT_NAME:
 *
 * Checks the value of an element in XSLT namespace.
 */
#define IS_XSLT_NAME(n, val)						\
    (xmlStrEqual((n)->name, (const xmlChar *) (val)))

/**
 * IS_XSLT_REAL_NODE:
 *
 * Check that a node is a 'real' one: document, element, text or attribute.
 */
#define IS_XSLT_REAL_NODE(n)						\
    (((n) != NULL) &&							\
     (((n)->type == XML_ELEMENT_NODE) ||				\
      ((n)->type == XML_TEXT_NODE) ||					\
      ((n)->type == XML_CDATA_SECTION_NODE) ||				\
      ((n)->type == XML_ATTRIBUTE_NODE) ||				\
      ((n)->type == XML_DOCUMENT_NODE) ||				\
      ((n)->type == XML_HTML_DOCUMENT_NODE) ||				\
      ((n)->type == XML_COMMENT_NODE) ||				\
      ((n)->type == XML_PI_NODE)))

/*
 * Our own version of namespaced attributes lookup.
 */
XSLTPUBFUN xmlChar *xsltGetNsProp(xmlNodePtr node : itype(_Ptr<xmlNode>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *nameSpace : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XSLTPUBFUN const xmlChar *xsltGetCNsProp(xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>), xmlNodePtr node : itype(_Ptr<xmlNode>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *nameSpace : itype(_Ptr<const xmlChar>)) : itype(_Ptr<const xmlChar>);
XSLTPUBFUN int XSLTCALL
		xsltGetUTF8Char	(const unsigned char *utf : itype(_Ptr<const unsigned char>), int *len : itype(_Ptr<int>));

/*
 * XSLT Debug Tracing Tracing Types
 */
typedef enum {
	XSLT_TRACE_ALL =		-1,
	XSLT_TRACE_NONE =		0,
	XSLT_TRACE_COPY_TEXT =		1<<0,
	XSLT_TRACE_PROCESS_NODE =	1<<1,
	XSLT_TRACE_APPLY_TEMPLATE =	1<<2,
	XSLT_TRACE_COPY =		1<<3,
	XSLT_TRACE_COMMENT =		1<<4,
	XSLT_TRACE_PI =			1<<5,
	XSLT_TRACE_COPY_OF =		1<<6,
	XSLT_TRACE_VALUE_OF =		1<<7,
	XSLT_TRACE_CALL_TEMPLATE =	1<<8,
	XSLT_TRACE_APPLY_TEMPLATES =	1<<9,
	XSLT_TRACE_CHOOSE =		1<<10,
	XSLT_TRACE_IF =			1<<11,
	XSLT_TRACE_FOR_EACH =		1<<12,
	XSLT_TRACE_STRIP_SPACES =	1<<13,
	XSLT_TRACE_TEMPLATES =		1<<14,
	XSLT_TRACE_KEYS =		1<<15,
	XSLT_TRACE_VARIABLES =		1<<16
} xsltDebugTraceCodes;

/**
 * XSLT_TRACE:
 *
 * Control the type of xsl debugtrace messages emitted.
 */
#define XSLT_TRACE(ctxt,code,call)	\
	if (ctxt->traceCode && (*(ctxt->traceCode) & code)) \
	    call

XSLTPUBFUN void XSLTCALL
		xsltDebugSetDefaultTrace(xsltDebugTraceCodes val);
XSLTPUBFUN xsltDebugTraceCodes XSLTCALL
		xsltDebugGetDefaultTrace(void);

/*
 * XSLT specific error and debug reporting functions.
 */
XSLTPUBVAR xmlGenericErrorFunc xsltGenericError;
XSLTPUBVAR void *xsltGenericErrorContext;
XSLTPUBVAR xmlGenericErrorFunc xsltGenericDebug;
XSLTPUBVAR void *xsltGenericDebugContext;

XSLTPUBFUN void XSLTCALL
		xsltPrintErrorContext		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>), xmlNodePtr node : itype(_Ptr<xmlNode>));
XSLTPUBFUN void XSLTCALL
		xsltMessage			(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>));
XSLTPUBFUN void XSLTCALL
		xsltSetGenericErrorFunc		(void *ctx, xmlGenericErrorFunc handler);
XSLTPUBFUN void XSLTCALL
		xsltSetGenericDebugFunc		(void *ctx, xmlGenericErrorFunc handler);
XSLTPUBFUN void XSLTCALL
		xsltSetTransformErrorFunc	(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), void *ctx, xmlGenericErrorFunc handler); 
XSLTPUBFUN void XSLTCALL
		xsltTransformError		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>), xmlNodePtr node : itype(_Ptr<xmlNode>), const char *msg : itype(_Ptr<const char>), ...) LIBXSLT_ATTR_FORMAT(4,5);

XSLTPUBFUN int XSLTCALL
		xsltSetCtxtParseOptions		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), int options);
/*
 * Sorting.
 */

XSLTPUBFUN void XSLTCALL
		xsltDocumentSortFunction	(xmlNodeSetPtr list : itype(_Ptr<xmlNodeSet>));
XSLTPUBFUN void XSLTCALL
		xsltSetSortFunc			(xsltSortFunc handler : itype(_Ptr<void (xsltTransformContextPtr, xmlNodePtr *, int)>));
XSLTPUBFUN void XSLTCALL
		xsltSetCtxtSortFunc		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xsltSortFunc handler : itype(_Ptr<void (xsltTransformContextPtr, xmlNodePtr *, int)>));
XSLTPUBFUN void XSLTCALL
		xsltDefaultSortFunction		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr *sorts : itype(_Ptr<xmlNodePtr>), int nbsorts);
XSLTPUBFUN void XSLTCALL
		xsltDoSortFunction		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr *sorts : itype(_Ptr<xmlNodePtr>), int nbsorts);
XSLTPUBFUN xmlXPathObjectPtr *xsltComputeSortResult(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr sort : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlXPathObjectPtr>);

/*
 * QNames handling.
 */

XSLTPUBFUN const xmlChar *xsltSplitQName(xmlDictPtr dict : itype(_Ptr<xmlDict>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar **prefix : itype(_Ptr<_Ptr<const xmlChar>>)) : itype(_Ptr<const xmlChar>);
XSLTPUBFUN const xmlChar *xsltGetQNameURI(xmlNodePtr node : itype(_Ptr<xmlNode>), xmlChar **name : itype(_Ptr<_Ptr<xmlChar>>)) : itype(_Ptr<const xmlChar>);

XSLTPUBFUN const xmlChar *xsltGetQNameURI2(xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>), xmlNodePtr node : itype(_Ptr<xmlNode>), const xmlChar **name : itype(_Ptr<_Ptr<const xmlChar>>)) : itype(_Ptr<const xmlChar>);

/*
 * Output, reuse libxml I/O buffers.
 */
XSLTPUBFUN int XSLTCALL
		xsltSaveResultTo		(xmlOutputBufferPtr buf : itype(_Ptr<xmlOutputBuffer>), xmlDocPtr result : itype(_Ptr<xmlDoc>), xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>));
XSLTPUBFUN int XSLTCALL
		xsltSaveResultToFilename	(const char *URI : itype(_Ptr<const char>), xmlDocPtr result : itype(_Ptr<xmlDoc>), xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>), int compression);
XSLTPUBFUN int XSLTCALL
		xsltSaveResultToFile		(FILE *file : itype(_Ptr<FILE>), xmlDocPtr result : itype(_Ptr<xmlDoc>), xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>));
XSLTPUBFUN int XSLTCALL
		xsltSaveResultToFd		(int fd, xmlDocPtr result : itype(_Ptr<xmlDoc>), xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>));
XSLTPUBFUN int XSLTCALL
		xsltSaveResultToString          (xmlChar **doc_txt_ptr : itype(_Ptr<xmlChar *>), int *doc_txt_len : itype(_Ptr<int>), xmlDocPtr result : itype(_Ptr<xmlDoc>), xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>));

/*
 * XPath interface
 */
XSLTPUBFUN xmlXPathCompExprPtr xsltXPathCompile(xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>), const xmlChar *str : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlXPathCompExpr>);
XSLTPUBFUN xmlXPathCompExprPtr xsltXPathCompileFlags(xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>), const xmlChar *str : itype(_Ptr<const xmlChar>), int flags) : itype(_Ptr<xmlXPathCompExpr>);

/*
 * Profiling.
 */
XSLTPUBFUN void XSLTCALL
		xsltSaveProfiling		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), FILE *output : itype(_Ptr<FILE>));
XSLTPUBFUN xmlDocPtr xsltGetProfileInformation(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>)) : itype(_Ptr<xmlDoc>);

XSLTPUBFUN long XSLTCALL
		xsltTimestamp			(void);
XSLTPUBFUN void XSLTCALL
		xsltCalibrateAdjust		(long delta);

/**
 * XSLT_TIMESTAMP_TICS_PER_SEC:
 *
 * Sampling precision for profiling
 */
#define XSLT_TIMESTAMP_TICS_PER_SEC 100000l

/*
 * Hooks for the debugger.
 */

typedef enum {
    XSLT_DEBUG_NONE = 0, /* no debugging allowed */
    XSLT_DEBUG_INIT,
    XSLT_DEBUG_STEP,
    XSLT_DEBUG_STEPOUT,
    XSLT_DEBUG_NEXT,
    XSLT_DEBUG_STOP,
    XSLT_DEBUG_CONT,
    XSLT_DEBUG_RUN,
    XSLT_DEBUG_RUN_RESTART,
    XSLT_DEBUG_QUIT
} xsltDebugStatusCodes;

XSLTPUBVAR int xslDebugStatus;

typedef void (*xsltHandleDebuggerCallback) (xmlNodePtr cur, xmlNodePtr node,
			xsltTemplatePtr templ, xsltTransformContextPtr ctxt);
typedef int (*xsltAddCallCallback) (xsltTemplatePtr templ, xmlNodePtr source);
typedef void (*xsltDropCallCallback) (void);

XSLTPUBFUN void XSLTCALL
		xsltSetDebuggerStatus		(int value);
XSLTPUBFUN int XSLTCALL
		xsltGetDebuggerStatus		(void);
XSLTPUBFUN int XSLTCALL
		xsltSetDebuggerCallbacks	(int no, void *block);
XSLTPUBFUN int XSLTCALL
		xslAddCall			(xsltTemplatePtr templ : itype(_Ptr<xsltTemplate>), xmlNodePtr source : itype(_Ptr<xmlNode>));
XSLTPUBFUN void XSLTCALL
		xslDropCall			(void);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLTUTILS_H__ */


