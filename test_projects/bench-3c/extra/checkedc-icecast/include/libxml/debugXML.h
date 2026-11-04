/*
 * Summary: Tree debugging APIs
 * Description: Interfaces to a set of routines used for debugging the tree
 *              produced by the XML parser.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __DEBUG_XML__
#define __DEBUG_XML__
#include <stdio.h>
#include <libxml/xmlversion.h>
#include <libxml/tree.h>

#ifdef LIBXML_DEBUG_ENABLED

#include <libxml/xpath.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The standard Dump routines.
 */
XMLPUBFUN void XMLCALL
	xmlDebugDumpString	(FILE *output : itype(_Ptr<FILE>), const xmlChar *str : itype(_Ptr<const xmlChar>));
XMLPUBFUN void XMLCALL
	xmlDebugDumpAttr	(FILE *output : itype(_Ptr<FILE>), xmlAttrPtr attr : itype(_Ptr<xmlAttr>), int depth);
XMLPUBFUN void XMLCALL
	xmlDebugDumpAttrList	(FILE *output : itype(_Ptr<FILE>), xmlAttrPtr attr : itype(_Ptr<xmlAttr>), int depth);
XMLPUBFUN void XMLCALL
	xmlDebugDumpOneNode	(FILE *output : itype(_Ptr<FILE>), xmlNodePtr node : itype(_Ptr<xmlNode>), int depth);
XMLPUBFUN void XMLCALL
	xmlDebugDumpNode	(FILE *output : itype(_Ptr<FILE>), xmlNodePtr node : itype(_Ptr<xmlNode>), int depth);
XMLPUBFUN void XMLCALL
	xmlDebugDumpNodeList	(FILE *output : itype(_Ptr<FILE>), xmlNodePtr node : itype(_Ptr<xmlNode>), int depth);
XMLPUBFUN void XMLCALL
	xmlDebugDumpDocumentHead(FILE *output : itype(_Ptr<FILE>), xmlDocPtr doc : itype(_Ptr<xmlDoc>));
XMLPUBFUN void XMLCALL
	xmlDebugDumpDocument	(FILE *output : itype(_Ptr<FILE>), xmlDocPtr doc : itype(_Ptr<xmlDoc>));
XMLPUBFUN void XMLCALL
	xmlDebugDumpDTD		(FILE *output : itype(_Ptr<FILE>), xmlDtdPtr dtd : itype(_Ptr<xmlDtd>));
XMLPUBFUN void XMLCALL
	xmlDebugDumpEntities	(FILE *output : itype(_Ptr<FILE>), xmlDocPtr doc : itype(_Ptr<xmlDoc>));

/****************************************************************
 *								*
 *			Checking routines			*
 *								*
 ****************************************************************/

XMLPUBFUN int XMLCALL
	xmlDebugCheckDocument	(FILE *output : itype(_Ptr<FILE>), xmlDocPtr doc : itype(_Ptr<xmlDoc>));

/****************************************************************
 *								*
 *			XML shell helpers			*
 *								*
 ****************************************************************/

XMLPUBFUN void XMLCALL
	xmlLsOneNode		(FILE *output : itype(_Ptr<FILE>), xmlNodePtr node : itype(_Ptr<xmlNode>));
XMLPUBFUN int XMLCALL
	xmlLsCountNode		(xmlNodePtr node : itype(_Ptr<xmlNode>));

XMLPUBFUN const char *xmlBoolToText(int boolval) : itype(_Ptr<const char>);

/****************************************************************
 *								*
 *	 The XML shell related structures and functions		*
 *								*
 ****************************************************************/

#ifdef LIBXML_XPATH_ENABLED
/**
 * xmlShellReadlineFunc:
 * @prompt:  a string prompt
 *
 * This is a generic signature for the XML shell input function.
 *
 * Returns a string which will be freed by the Shell.
 */
typedef char * (* xmlShellReadlineFunc)(char *prompt);

/**
 * xmlShellCtxt:
 *
 * A debugging shell context.
 * TODO: add the defined function tables.
 */
typedef struct _xmlShellCtxt xmlShellCtxt;
typedef xmlShellCtxt *xmlShellCtxtPtr;
struct _xmlShellCtxt {
    char *filename : itype(_Ptr<char>);
    xmlDocPtr doc : itype(_Ptr<xmlDoc>);
    xmlNodePtr node : itype(_Ptr<xmlNode>);
    xmlXPathContextPtr pctxt : itype(_Ptr<xmlXPathContext>);
    int loaded;
    FILE *output : itype(_Ptr<FILE>);
    xmlShellReadlineFunc input : itype(_Ptr<char *(char *)>);
};

/**
 * xmlShellCmd:
 * @ctxt:  a shell context
 * @arg:  a string argument
 * @node:  a first node
 * @node2:  a second node
 *
 * This is a generic signature for the XML shell functions.
 *
 * Returns an int, negative returns indicating errors.
 */
typedef int (* xmlShellCmd) (xmlShellCtxtPtr ctxt,
                             char *arg,
			     xmlNodePtr node,
			     xmlNodePtr node2);

XMLPUBFUN void XMLCALL
	xmlShellPrintXPathError	(int errorType, const char *arg : itype(_Ptr<const char>));
XMLPUBFUN void XMLCALL
	xmlShellPrintXPathResult(xmlXPathObjectPtr list : itype(_Ptr<xmlXPathObject>));
XMLPUBFUN int XMLCALL
	xmlShellList		(xmlShellCtxtPtr ctxt : itype(_Ptr<xmlShellCtxt>), char *arg : itype(_Ptr<char>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr node2 : itype(_Ptr<xmlNode>));
XMLPUBFUN int XMLCALL
	xmlShellBase		(xmlShellCtxtPtr ctxt : itype(_Ptr<xmlShellCtxt>), char *arg : itype(_Ptr<char>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr node2 : itype(_Ptr<xmlNode>));
XMLPUBFUN int XMLCALL
	xmlShellDir		(xmlShellCtxtPtr ctxt : itype(_Ptr<xmlShellCtxt>), char *arg : itype(_Ptr<char>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr node2 : itype(_Ptr<xmlNode>));
XMLPUBFUN int XMLCALL
	xmlShellLoad		(xmlShellCtxtPtr ctxt : itype(_Ptr<xmlShellCtxt>), char *filename : itype(_Ptr<char>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr node2 : itype(_Ptr<xmlNode>));
#ifdef LIBXML_OUTPUT_ENABLED
XMLPUBFUN void XMLCALL
	xmlShellPrintNode	(xmlNodePtr node : itype(_Ptr<xmlNode>));
XMLPUBFUN int XMLCALL
	xmlShellCat		(xmlShellCtxtPtr ctxt : itype(_Ptr<xmlShellCtxt>), char *arg : itype(_Ptr<char>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr node2 : itype(_Ptr<xmlNode>));
XMLPUBFUN int XMLCALL
	xmlShellWrite		(xmlShellCtxtPtr ctxt : itype(_Ptr<xmlShellCtxt>), char *filename : itype(_Ptr<char>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr node2 : itype(_Ptr<xmlNode>));
XMLPUBFUN int XMLCALL
	xmlShellSave		(xmlShellCtxtPtr ctxt : itype(_Ptr<xmlShellCtxt>), char *filename : itype(_Ptr<char>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr node2 : itype(_Ptr<xmlNode>));
#endif /* LIBXML_OUTPUT_ENABLED */
#ifdef LIBXML_VALID_ENABLED
XMLPUBFUN int XMLCALL
	xmlShellValidate	(xmlShellCtxtPtr ctxt : itype(_Ptr<xmlShellCtxt>), char *dtd : itype(_Ptr<char>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr node2 : itype(_Ptr<xmlNode>));
#endif /* LIBXML_VALID_ENABLED */
XMLPUBFUN int XMLCALL
	xmlShellDu		(xmlShellCtxtPtr ctxt : itype(_Ptr<xmlShellCtxt>), char *arg : itype(_Ptr<char>), xmlNodePtr tree : itype(_Ptr<xmlNode>), xmlNodePtr node2 : itype(_Ptr<xmlNode>));
XMLPUBFUN int XMLCALL
	xmlShellPwd		(xmlShellCtxtPtr ctxt : itype(_Ptr<xmlShellCtxt>), char *buffer : itype(_Ptr<char>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr node2 : itype(_Ptr<xmlNode>));

/*
 * The Shell interface.
 */
XMLPUBFUN void XMLCALL
	xmlShell		(xmlDocPtr doc : itype(_Ptr<xmlDoc>), char *filename : itype(_Ptr<char>), xmlShellReadlineFunc input : itype(_Ptr<char *(char *)>), FILE *output : itype(_Ptr<FILE>));

#endif /* LIBXML_XPATH_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* LIBXML_DEBUG_ENABLED */
#endif /* __DEBUG_XML__ */
