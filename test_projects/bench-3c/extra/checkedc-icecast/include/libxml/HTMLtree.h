/*
 * Summary: specific APIs to process HTML tree, especially serialization
 * Description: this module implements a few function needed to process
 *              tree in an HTML specific way.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __HTML_TREE_H__
#define __HTML_TREE_H__

#include <stdio.h>
#include <libxml/xmlversion.h>
#include <libxml/tree.h>
#include <libxml/HTMLparser.h>

#ifdef LIBXML_HTML_ENABLED

#ifdef __cplusplus
extern "C" {
#endif


/**
 * HTML_TEXT_NODE:
 *
 * Macro. A text node in a HTML document is really implemented
 * the same way as a text node in an XML document.
 */
#define HTML_TEXT_NODE		XML_TEXT_NODE
/**
 * HTML_ENTITY_REF_NODE:
 *
 * Macro. An entity reference in a HTML document is really implemented
 * the same way as an entity reference in an XML document.
 */
#define HTML_ENTITY_REF_NODE	XML_ENTITY_REF_NODE
/**
 * HTML_COMMENT_NODE:
 *
 * Macro. A comment in a HTML document is really implemented
 * the same way as a comment in an XML document.
 */
#define HTML_COMMENT_NODE	XML_COMMENT_NODE
/**
 * HTML_PRESERVE_NODE:
 *
 * Macro. A preserved node in a HTML document is really implemented
 * the same way as a CDATA section in an XML document.
 */
#define HTML_PRESERVE_NODE	XML_CDATA_SECTION_NODE
/**
 * HTML_PI_NODE:
 *
 * Macro. A processing instruction in a HTML document is really implemented
 * the same way as a processing instruction in an XML document.
 */
#define HTML_PI_NODE		XML_PI_NODE

XMLPUBFUN htmlDocPtr htmlNewDoc(const xmlChar *URI : itype(_Ptr<const xmlChar>), const xmlChar *ExternalID : itype(_Ptr<const xmlChar>)) : itype(xmlDocPtr);
XMLPUBFUN htmlDocPtr htmlNewDocNoDtD(const xmlChar *URI : itype(_Ptr<const xmlChar>), const xmlChar *ExternalID : itype(_Ptr<const xmlChar>)) : itype(xmlDocPtr);
XMLPUBFUN const xmlChar *htmlGetMetaEncoding(htmlDocPtr doc : itype(xmlDocPtr)) : itype(_Ptr<const xmlChar>);
XMLPUBFUN int XMLCALL
		htmlSetMetaEncoding	(htmlDocPtr doc : itype(xmlDocPtr), const xmlChar *encoding : itype(_Ptr<const xmlChar>));
#ifdef LIBXML_OUTPUT_ENABLED
XMLPUBFUN void XMLCALL
		htmlDocDumpMemory	(xmlDocPtr cur : itype(_Ptr<xmlDoc>), xmlChar **mem : itype(_Ptr<_Ptr<xmlChar>>), int *size : itype(_Ptr<int>));
XMLPUBFUN void XMLCALL
		htmlDocDumpMemoryFormat	(xmlDocPtr cur : itype(_Ptr<xmlDoc>), xmlChar **mem : itype(_Ptr<_Ptr<xmlChar>>), int *size : itype(_Ptr<int>), int format);
XMLPUBFUN int XMLCALL
		htmlDocDump		(FILE *f : itype(_Ptr<FILE>), xmlDocPtr cur : itype(_Ptr<xmlDoc>));
XMLPUBFUN int XMLCALL
		htmlSaveFile		(const char *filename : itype(_Ptr<const char>), xmlDocPtr cur : itype(_Ptr<xmlDoc>));
XMLPUBFUN int XMLCALL
		htmlNodeDump		(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr cur : itype(_Ptr<xmlNode>));
XMLPUBFUN void XMLCALL
		htmlNodeDumpFile	(FILE *out : itype(_Ptr<FILE>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr cur : itype(_Ptr<xmlNode>));
XMLPUBFUN int XMLCALL
		htmlNodeDumpFileFormat	(FILE *out : itype(_Ptr<FILE>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr cur : itype(_Ptr<xmlNode>), const char *encoding : itype(_Ptr<const char>), int format);
XMLPUBFUN int XMLCALL
		htmlSaveFileEnc		(const char *filename : itype(_Ptr<const char>), xmlDocPtr cur : itype(_Ptr<xmlDoc>), const char *encoding : itype(_Ptr<const char>));
XMLPUBFUN int XMLCALL
		htmlSaveFileFormat	(const char *filename : itype(_Ptr<const char>), xmlDocPtr cur : itype(_Ptr<xmlDoc>), const char *encoding : itype(_Ptr<const char>), int format);

XMLPUBFUN void XMLCALL
		htmlNodeDumpFormatOutput(xmlOutputBufferPtr buf : itype(_Ptr<xmlOutputBuffer>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr cur : itype(_Ptr<xmlNode>), const char *encoding : itype(_Ptr<const char>), int format);
XMLPUBFUN void XMLCALL
		htmlDocContentDumpOutput(xmlOutputBufferPtr buf : itype(_Ptr<xmlOutputBuffer>), xmlDocPtr cur : itype(_Ptr<xmlDoc>), const char *encoding : itype(_Ptr<const char>));
XMLPUBFUN void XMLCALL
		htmlDocContentDumpFormatOutput(xmlOutputBufferPtr buf : itype(_Ptr<xmlOutputBuffer>), xmlDocPtr cur : itype(_Ptr<xmlDoc>), const char *encoding : itype(_Ptr<const char>), int format);
XMLPUBFUN void XMLCALL
		htmlNodeDumpOutput	(xmlOutputBufferPtr buf : itype(_Ptr<xmlOutputBuffer>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr cur : itype(_Ptr<xmlNode>), const char *encoding : itype(_Ptr<const char>));

#endif /* LIBXML_OUTPUT_ENABLED */

XMLPUBFUN int XMLCALL
		htmlIsBooleanAttr	(const xmlChar *name : itype(_Ptr<const xmlChar>));


#ifdef __cplusplus
}
#endif

#endif /* LIBXML_HTML_ENABLED */

#endif /* __HTML_TREE_H__ */

