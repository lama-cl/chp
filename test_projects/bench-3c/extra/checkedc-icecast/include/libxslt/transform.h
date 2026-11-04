/*
 * Summary: the XSLT engine transformation part.
 * Description: This module implements the bulk of the actual
 *              transformation processing. Most of the xsl: element
 *              constructs are implemented in this module.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_XSLT_TRANSFORM_H__
#define __XML_XSLT_TRANSFORM_H__

#include <libxml/parser.h>
#include <libxml/xmlIO.h>
#include "xsltexports.h"
#include <libxslt/xsltInternals.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * XInclude default processing.
 */
XSLTPUBFUN void XSLTCALL
		xsltSetXIncludeDefault	(int xinclude);
XSLTPUBFUN int XSLTCALL
		xsltGetXIncludeDefault	(void);

/**
 * Export context to users.
 */
XSLTPUBFUN xsltTransformContextPtr xsltNewTransformContext(xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>), xmlDocPtr doc : itype(_Ptr<xmlDoc>)) : itype(_Ptr<xsltTransformContext>);

XSLTPUBFUN void XSLTCALL
		xsltFreeTransformContext(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>));

XSLTPUBFUN xmlDocPtr xsltApplyStylesheetUser(xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), const char **params : itype(_Ptr<_Ptr<const char>>), const char *output : itype(_Ptr<const char>), FILE *profile : itype(_Ptr<FILE>), xsltTransformContextPtr userCtxt : itype(_Ptr<xsltTransformContext>)) : itype(_Ptr<xmlDoc>);
XSLTPUBFUN void XSLTCALL
                xsltProcessOneNode      (xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xsltStackElemPtr params : itype(_Ptr<xsltStackElem>));
/**
 * Private Interfaces.
 */
XSLTPUBFUN void XSLTCALL
		xsltApplyStripSpaces	(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>));
XSLTPUBFUN xmlDocPtr xsltApplyStylesheet(xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), const char **params : itype(_Ptr<_Ptr<const char>>)) : itype(_Ptr<xmlDoc>);
XSLTPUBFUN xmlDocPtr xsltProfileStylesheet(xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), const char **params : itype(_Ptr<_Ptr<const char>>), FILE *output : itype(_Ptr<FILE>)) : itype(_Ptr<xmlDoc>);
XSLTPUBFUN int XSLTCALL
		xsltRunStylesheet	(xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), const char **params : itype(_Ptr<_Ptr<const char>>), const char *output : itype(_Ptr<const char>), xmlSAXHandlerPtr SAX : itype(_Ptr<xmlSAXHandler>), xmlOutputBufferPtr IObuf : itype(_Ptr<xmlOutputBuffer>));
XSLTPUBFUN int XSLTCALL
		xsltRunStylesheetUser	(xsltStylesheetPtr style : itype(_Ptr<xsltStylesheet>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), const char **params : itype(_Ptr<_Ptr<const char>>), const char *output : itype(_Ptr<const char>), xmlSAXHandlerPtr SAX : itype(_Ptr<xmlSAXHandler>), xmlOutputBufferPtr IObuf : itype(_Ptr<xmlOutputBuffer>), FILE *profile : itype(_Ptr<FILE>), xsltTransformContextPtr userCtxt : itype(_Ptr<xsltTransformContext>));
XSLTPUBFUN void XSLTCALL
		xsltApplyOneTemplate	(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr list : itype(_Ptr<xmlNode>), xsltTemplatePtr templ : itype(_Ptr<xsltTemplate>), xsltStackElemPtr params : itype(_Ptr<xsltStackElem>));
XSLTPUBFUN void XSLTCALL
		xsltDocumentElem	(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltSort		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltCopy		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltText		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltElement		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltComment		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltAttribute		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltProcessingInstruction(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltCopyOf		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltValueOf		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltNumber		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltApplyImports	(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltCallTemplate	(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltApplyTemplates	(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltChoose		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltIf			(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltForEach		(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr inst : itype(_Ptr<xmlNode>), xsltElemPreCompPtr comp : itype(_Ptr<xsltElemPreComp>));
XSLTPUBFUN void XSLTCALL
		xsltRegisterAllElement	(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>));

XSLTPUBFUN xmlNodePtr xsltCopyTextString(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xmlNodePtr target : itype(_Ptr<xmlNode>), const xmlChar *string : itype(_Ptr<const xmlChar>), int noescape) : itype(_Ptr<xmlNode>);

/* Following 2 functions needed for libexslt/functions.c */
XSLTPUBFUN void XSLTCALL
		xsltLocalVariablePop	(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), int limitNr, int level);
XSLTPUBFUN int XSLTCALL
		xsltLocalVariablePush	(xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>), xsltStackElemPtr variable : itype(_Ptr<xsltStackElem>), int level);
/*
 * Hook for the debugger if activated.
 */
XSLTPUBFUN void XSLTCALL
		xslHandleDebugger	(xmlNodePtr cur : itype(_Ptr<xmlNode>), xmlNodePtr node : itype(_Ptr<xmlNode>), xsltTemplatePtr templ : itype(_Ptr<xsltTemplate>), xsltTransformContextPtr ctxt : itype(_Ptr<xsltTransformContext>));

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_TRANSFORM_H__ */

