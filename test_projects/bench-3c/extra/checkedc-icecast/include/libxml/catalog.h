/**
 * Summary: interfaces to the Catalog handling system
 * Description: the catalog module implements the support for
 * XML Catalogs and SGML catalogs
 *
 * SGML Open Technical Resolution TR9401:1997.
 * http://www.jclark.com/sp/catalog.htm
 *
 * XML Catalogs Working Draft 06 August 2001
 * http://www.oasis-open.org/committees/entity/spec-2001-08-06.html
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_CATALOG_H__
#define __XML_CATALOG_H__

#include <stdio.h>

#include <libxml/xmlversion.h>
#include <libxml/xmlstring.h>
#include <libxml/tree.h>

#ifdef LIBXML_CATALOG_ENABLED

#ifdef __cplusplus
extern "C" {
#endif

/**
 * XML_CATALOGS_NAMESPACE:
 *
 * The namespace for the XML Catalogs elements.
 */
#define XML_CATALOGS_NAMESPACE					\
    (const xmlChar *) "urn:oasis:names:tc:entity:xmlns:xml:catalog"
/**
 * XML_CATALOG_PI:
 *
 * The specific XML Catalog Processing Instruction name.
 */
#define XML_CATALOG_PI						\
    (const xmlChar *) "oasis-xml-catalog"

/*
 * The API is voluntarily limited to general cataloging.
 */
typedef enum {
    XML_CATA_PREFER_NONE = 0,
    XML_CATA_PREFER_PUBLIC = 1,
    XML_CATA_PREFER_SYSTEM
} xmlCatalogPrefer;

typedef enum {
    XML_CATA_ALLOW_NONE = 0,
    XML_CATA_ALLOW_GLOBAL = 1,
    XML_CATA_ALLOW_DOCUMENT = 2,
    XML_CATA_ALLOW_ALL = 3
} xmlCatalogAllow;

typedef struct _xmlCatalog xmlCatalog;
typedef xmlCatalog *xmlCatalogPtr;

/*
 * Operations on a given catalog.
 */
XMLPUBFUN xmlCatalogPtr xmlNewCatalog(int sgml) : itype(_Ptr<xmlCatalog>);
XMLPUBFUN xmlCatalogPtr xmlLoadACatalog(const char *filename : itype(_Ptr<const char>)) : itype(_Ptr<xmlCatalog>);
XMLPUBFUN xmlCatalogPtr xmlLoadSGMLSuperCatalog(const char *filename : itype(_Ptr<const char>)) : itype(_Ptr<xmlCatalog>);
XMLPUBFUN int XMLCALL
		xmlConvertSGMLCatalog	(xmlCatalogPtr catal : itype(_Ptr<xmlCatalog>));
XMLPUBFUN int XMLCALL
		xmlACatalogAdd		(xmlCatalogPtr catal : itype(_Ptr<xmlCatalog>), const xmlChar *type : itype(_Ptr<const xmlChar>), const xmlChar *orig : itype(_Ptr<const xmlChar>), const xmlChar *replace : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
		xmlACatalogRemove	(xmlCatalogPtr catal : itype(_Ptr<xmlCatalog>), const xmlChar *value : itype(_Ptr<const xmlChar>));
XMLPUBFUN xmlChar *xmlACatalogResolve(xmlCatalogPtr catal : itype(_Ptr<xmlCatalog>), const xmlChar *pubID : itype(_Ptr<const xmlChar>), const xmlChar *sysID : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlACatalogResolveSystem(xmlCatalogPtr catal : itype(_Ptr<xmlCatalog>), const xmlChar *sysID : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlACatalogResolvePublic(xmlCatalogPtr catal : itype(_Ptr<xmlCatalog>), const xmlChar *pubID : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlACatalogResolveURI(xmlCatalogPtr catal : itype(_Ptr<xmlCatalog>), const xmlChar *URI : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
#ifdef LIBXML_OUTPUT_ENABLED
XMLPUBFUN void XMLCALL
		xmlACatalogDump		(xmlCatalogPtr catal : itype(_Ptr<xmlCatalog>), FILE *out : itype(_Ptr<FILE>));
#endif /* LIBXML_OUTPUT_ENABLED */
XMLPUBFUN void XMLCALL
		xmlFreeCatalog		(xmlCatalogPtr catal : itype(_Ptr<xmlCatalog>));
XMLPUBFUN int XMLCALL
		xmlCatalogIsEmpty	(xmlCatalogPtr catal : itype(_Ptr<xmlCatalog>));

/*
 * Global operations.
 */
XMLPUBFUN void XMLCALL
		xmlInitializeCatalog	(void);
XMLPUBFUN int XMLCALL
		xmlLoadCatalog		(const char *filename : itype(_Ptr<const char>));
XMLPUBFUN void XMLCALL
		xmlLoadCatalogs		(const char *paths : itype(_Ptr<const char>));
XMLPUBFUN void XMLCALL
		xmlCatalogCleanup	(void);
#ifdef LIBXML_OUTPUT_ENABLED
XMLPUBFUN void XMLCALL
		xmlCatalogDump		(FILE *out : itype(_Ptr<FILE>));
#endif /* LIBXML_OUTPUT_ENABLED */
XMLPUBFUN xmlChar *xmlCatalogResolve(const xmlChar *pubID : itype(_Ptr<const xmlChar>), const xmlChar *sysID : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlCatalogResolveSystem(const xmlChar *sysID : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlCatalogResolvePublic(const xmlChar *pubID : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlCatalogResolveURI(const xmlChar *URI : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN int XMLCALL
		xmlCatalogAdd		(const xmlChar *type : itype(_Ptr<const xmlChar>), const xmlChar *orig : itype(_Ptr<const xmlChar>), const xmlChar *replace : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
		xmlCatalogRemove	(const xmlChar *value : itype(_Ptr<const xmlChar>));
XMLPUBFUN xmlDocPtr xmlParseCatalogFile(const char *filename : itype(_Ptr<const char>)) : itype(_Ptr<xmlDoc>);
XMLPUBFUN int XMLCALL
		xmlCatalogConvert	(void);

/*
 * Strictly minimal interfaces for per-document catalogs used
 * by the parser.
 */
XMLPUBFUN void XMLCALL
		xmlCatalogFreeLocal	(void *catalogs);
XMLPUBFUN void * XMLCALL
		xmlCatalogAddLocal	(void *catalogs, const xmlChar *URL : itype(_Ptr<const xmlChar>));
XMLPUBFUN xmlChar *xmlCatalogLocalResolve(void *catalogs, const xmlChar *pubID : itype(_Ptr<const xmlChar>), const xmlChar *sysID : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlCatalogLocalResolveURI(void *catalogs, const xmlChar *URI : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
/*
 * Preference settings.
 */
XMLPUBFUN int XMLCALL
		xmlCatalogSetDebug	(int level);
XMLPUBFUN xmlCatalogPrefer XMLCALL
		xmlCatalogSetDefaultPrefer(xmlCatalogPrefer prefer);
XMLPUBFUN void XMLCALL
		xmlCatalogSetDefaults	(xmlCatalogAllow allow);
XMLPUBFUN xmlCatalogAllow XMLCALL
		xmlCatalogGetDefaults	(void);


/* DEPRECATED interfaces */
XMLPUBFUN const xmlChar *xmlCatalogGetSystem(const xmlChar *sysID : itype(_Ptr<const xmlChar>)) : itype(_Ptr<const xmlChar>);
XMLPUBFUN const xmlChar *xmlCatalogGetPublic(const xmlChar *pubID : itype(_Ptr<const xmlChar>)) : itype(_Ptr<const xmlChar>);

#ifdef __cplusplus
}
#endif
#endif /* LIBXML_CATALOG_ENABLED */
#endif /* __XML_CATALOG_H__ */
