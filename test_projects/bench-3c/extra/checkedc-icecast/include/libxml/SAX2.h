/*
 * Summary: SAX2 parser interface used to build the DOM tree
 * Description: those are the default SAX2 interfaces used by
 *              the library when building DOM tree.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */


#ifndef __XML_SAX2_H__
#define __XML_SAX2_H__

#include <stdio.h>
#include <stdlib.h>
#include <libxml/xmlversion.h>
#include <libxml/parser.h>
#include <libxml/xlink.h>

#ifdef __cplusplus
extern "C" {
#endif
XMLPUBFUN const xmlChar *xmlSAX2GetPublicId(void *ctx) : itype(_Ptr<const xmlChar>);
XMLPUBFUN const xmlChar *xmlSAX2GetSystemId(void *ctx) : itype(_Ptr<const xmlChar>);
XMLPUBFUN void XMLCALL
		xmlSAX2SetDocumentLocator	(void *ctx, xmlSAXLocatorPtr loc : itype(_Ptr<xmlSAXLocator>));

XMLPUBFUN int XMLCALL
		xmlSAX2GetLineNumber		(void *ctx);
XMLPUBFUN int XMLCALL
		xmlSAX2GetColumnNumber		(void *ctx);

XMLPUBFUN int XMLCALL
		xmlSAX2IsStandalone		(void *ctx);
XMLPUBFUN int XMLCALL
		xmlSAX2HasInternalSubset	(void *ctx);
XMLPUBFUN int XMLCALL
		xmlSAX2HasExternalSubset	(void *ctx);

XMLPUBFUN void XMLCALL
		xmlSAX2InternalSubset		(void *ctx, const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *ExternalID : itype(_Ptr<const xmlChar>), const xmlChar *SystemID : itype(_Ptr<const xmlChar>));
XMLPUBFUN void XMLCALL
		xmlSAX2ExternalSubset		(void *ctx, const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *ExternalID : itype(_Ptr<const xmlChar>), const xmlChar *SystemID : itype(_Ptr<const xmlChar>));
XMLPUBFUN xmlEntityPtr xmlSAX2GetEntity(void *ctx, const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlEntity>);
XMLPUBFUN xmlEntityPtr xmlSAX2GetParameterEntity(void *ctx, const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlEntity>);
XMLPUBFUN xmlParserInputPtr xmlSAX2ResolveEntity(void *ctx, const xmlChar *publicId : itype(_Ptr<const xmlChar>), const xmlChar *systemId : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlParserInput>);

XMLPUBFUN void XMLCALL
		xmlSAX2EntityDecl		(void *ctx, const xmlChar *name : itype(_Ptr<const xmlChar>), int type, const xmlChar *publicId : itype(_Ptr<const xmlChar>), const xmlChar *systemId : itype(_Ptr<const xmlChar>), xmlChar *content : itype(_Ptr<xmlChar>));
XMLPUBFUN void XMLCALL
		xmlSAX2AttributeDecl		(void *ctx, const xmlChar *elem : itype(_Ptr<const xmlChar>), const xmlChar *fullname : itype(_Ptr<const xmlChar>), int type, int def, const xmlChar *defaultValue : itype(_Ptr<const xmlChar>), xmlEnumerationPtr tree : itype(_Ptr<xmlEnumeration>));
XMLPUBFUN void XMLCALL
		xmlSAX2ElementDecl		(void *ctx, const xmlChar *name : itype(_Ptr<const xmlChar>), int type, xmlElementContentPtr content : itype(_Ptr<xmlElementContent>));
XMLPUBFUN void XMLCALL
		xmlSAX2NotationDecl		(void *ctx, const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *publicId : itype(_Ptr<const xmlChar>), const xmlChar *systemId : itype(_Ptr<const xmlChar>));
XMLPUBFUN void XMLCALL
		xmlSAX2UnparsedEntityDecl	(void *ctx, const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *publicId : itype(_Ptr<const xmlChar>), const xmlChar *systemId : itype(_Ptr<const xmlChar>), const xmlChar *notationName : itype(_Ptr<const xmlChar>));

XMLPUBFUN void XMLCALL
		xmlSAX2StartDocument		(void *ctx);
XMLPUBFUN void XMLCALL
		xmlSAX2EndDocument		(void *ctx);
#if defined(LIBXML_SAX1_ENABLED) || defined(LIBXML_HTML_ENABLED) || \
    defined(LIBXML_WRITER_ENABLED) || defined(LIBXML_DOCB_ENABLED) || \
    defined(LIBXML_LEGACY_ENABLED)
XMLPUBFUN void XMLCALL
		xmlSAX2StartElement		(void *ctx, const xmlChar *fullname : itype(_Ptr<const xmlChar>), const xmlChar **atts : itype(_Ptr<_Ptr<const xmlChar>>));
XMLPUBFUN void XMLCALL
		xmlSAX2EndElement		(void *ctx, const xmlChar *name : itype(_Ptr<const xmlChar>));
#endif /* LIBXML_SAX1_ENABLED or LIBXML_HTML_ENABLED or LIBXML_LEGACY_ENABLED */
XMLPUBFUN void XMLCALL
		xmlSAX2StartElementNs		(void *ctx, const xmlChar *localname : itype(_Ptr<const xmlChar>), const xmlChar *prefix : itype(_Ptr<const xmlChar>), const xmlChar *URI : itype(_Ptr<const xmlChar>), int nb_namespaces, const xmlChar **namespaces : itype(_Ptr<_Ptr<const xmlChar>>), int nb_attributes, int nb_defaulted, const xmlChar **attributes : itype(_Ptr<_Ptr<const xmlChar>>));
XMLPUBFUN void XMLCALL
		xmlSAX2EndElementNs		(void *ctx, const xmlChar *localname : itype(_Ptr<const xmlChar>), const xmlChar *prefix : itype(_Ptr<const xmlChar>), const xmlChar *URI : itype(_Ptr<const xmlChar>));
XMLPUBFUN void XMLCALL
		xmlSAX2Reference		(void *ctx, const xmlChar *name : itype(_Ptr<const xmlChar>));
XMLPUBFUN void XMLCALL
		xmlSAX2Characters		(void *ctx, const xmlChar *ch : itype(_Ptr<const xmlChar>), int len);
XMLPUBFUN void XMLCALL
		xmlSAX2IgnorableWhitespace	(void *ctx, const xmlChar *ch : itype(_Ptr<const xmlChar>), int len);
XMLPUBFUN void XMLCALL
		xmlSAX2ProcessingInstruction	(void *ctx, const xmlChar *target : itype(_Ptr<const xmlChar>), const xmlChar *data : itype(_Ptr<const xmlChar>));
XMLPUBFUN void XMLCALL
		xmlSAX2Comment			(void *ctx, const xmlChar *value : itype(_Ptr<const xmlChar>));
XMLPUBFUN void XMLCALL
		xmlSAX2CDataBlock		(void *ctx, const xmlChar *value : itype(_Ptr<const xmlChar>), int len);

#ifdef LIBXML_SAX1_ENABLED
XMLPUBFUN int XMLCALL
		xmlSAXDefaultVersion		(int version);
#endif /* LIBXML_SAX1_ENABLED */

XMLPUBFUN int XMLCALL
		xmlSAXVersion			(xmlSAXHandler *hdlr : itype(_Ptr<xmlSAXHandler>), int version);
XMLPUBFUN void XMLCALL
		xmlSAX2InitDefaultSAXHandler    (xmlSAXHandler *hdlr : itype(_Ptr<xmlSAXHandler>), int warning);
#ifdef LIBXML_HTML_ENABLED
XMLPUBFUN void XMLCALL
		xmlSAX2InitHtmlDefaultSAXHandler(xmlSAXHandler *hdlr : itype(_Ptr<xmlSAXHandler>));
XMLPUBFUN void XMLCALL
		htmlDefaultSAXHandlerInit	(void);
#endif
#ifdef LIBXML_DOCB_ENABLED
XMLPUBFUN void XMLCALL
		xmlSAX2InitDocbDefaultSAXHandler(xmlSAXHandler *hdlr : itype(_Ptr<xmlSAXHandler>));
XMLPUBFUN void XMLCALL
		docbDefaultSAXHandlerInit	(void);
#endif
XMLPUBFUN void XMLCALL
		xmlDefaultSAXHandlerInit	(void);
#ifdef __cplusplus
}
#endif
#endif /* __XML_SAX2_H__ */
