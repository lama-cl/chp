/*
 * Summary: interface for the XML entities handling
 * Description: this module provides some of the entity API needed
 *              for the parser and applications.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_ENTITIES_H__
#define __XML_ENTITIES_H__

#include <libxml/xmlversion.h>
#include <libxml/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The different valid entity types.
 */
typedef enum {
    XML_INTERNAL_GENERAL_ENTITY = 1,
    XML_EXTERNAL_GENERAL_PARSED_ENTITY = 2,
    XML_EXTERNAL_GENERAL_UNPARSED_ENTITY = 3,
    XML_INTERNAL_PARAMETER_ENTITY = 4,
    XML_EXTERNAL_PARAMETER_ENTITY = 5,
    XML_INTERNAL_PREDEFINED_ENTITY = 6
} xmlEntityType;

/*
 * An unit of storage for an entity, contains the string, the value
 * and the linkind data needed for the linking in the hash table.
 */

struct _xmlEntity {
    void           *_private;	        /* application data */
    xmlElementType          type;       /* XML_ENTITY_DECL, must be second ! */
    const xmlChar *name : itype(_Ptr<const xmlChar>);	/* Entity name */
    struct _xmlNode *children : itype(_Ptr<struct _xmlNode>);	/* First child link */
    struct _xmlNode *last : itype(_Ptr<struct _xmlNode>);	/* Last child link */
    struct _xmlDtd *parent : itype(_Ptr<struct _xmlDtd>);	/* -> DTD */
    struct _xmlNode *next : itype(_Ptr<struct _xmlNode>);	/* next sibling link  */
    struct _xmlNode *prev : itype(_Ptr<struct _xmlNode>);	/* previous sibling link  */
    struct _xmlDoc *doc : itype(_Ptr<struct _xmlDoc>);       /* the containing document */

    xmlChar *orig : itype(_Ptr<xmlChar>);	/* content without ref substitution */
    xmlChar *content : itype(_Ptr<xmlChar>);	/* content or ndata if unparsed */
    int                   length;	/* the content length */
    xmlEntityType          etype;	/* The entity type */
    const xmlChar *ExternalID : itype(_Ptr<const xmlChar>);	/* External identifier for PUBLIC */
    const xmlChar *SystemID : itype(_Ptr<const xmlChar>);	/* URI for a SYSTEM or PUBLIC Entity */

    struct _xmlEntity *nexte : itype(_Ptr<struct _xmlEntity>);	/* unused */
    const xmlChar *URI : itype(_Ptr<const xmlChar>);	/* the full URI as computed */
    int                    owner;	/* does the entity own the childrens */
    int			 checked;	/* was the entity content checked */
					/* this is also used to count entities
					 * references done from that entity
					 * and if it contains '<' */
};

/*
 * All entities are stored in an hash table.
 * There is 2 separate hash tables for global and parameter entities.
 */

typedef struct _xmlHashTable xmlEntitiesTable;
typedef xmlEntitiesTable *xmlEntitiesTablePtr;

/*
 * External functions:
 */

#ifdef LIBXML_LEGACY_ENABLED
XMLPUBFUN void XMLCALL
		xmlInitializePredefinedEntities	(void);
#endif /* LIBXML_LEGACY_ENABLED */

XMLPUBFUN xmlEntityPtr xmlNewEntity(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *name : itype(_Ptr<const xmlChar>), int type, const xmlChar *ExternalID : itype(_Ptr<const xmlChar>), const xmlChar *SystemID : itype(_Ptr<const xmlChar>), const xmlChar *content : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlEntity>);
XMLPUBFUN xmlEntityPtr xmlAddDocEntity(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *name : itype(_Ptr<const xmlChar>), int type, const xmlChar *ExternalID : itype(_Ptr<const xmlChar>), const xmlChar *SystemID : itype(_Ptr<const xmlChar>), const xmlChar *content : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlEntity>);
XMLPUBFUN xmlEntityPtr xmlAddDtdEntity(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *name : itype(_Ptr<const xmlChar>), int type, const xmlChar *ExternalID : itype(_Ptr<const xmlChar>), const xmlChar *SystemID : itype(_Ptr<const xmlChar>), const xmlChar *content : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlEntity>);
XMLPUBFUN xmlEntityPtr xmlGetPredefinedEntity(const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlEntity>);
XMLPUBFUN xmlEntityPtr xmlGetDocEntity(const xmlDoc *doc : itype(_Ptr<const xmlDoc>), const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlEntity>);
XMLPUBFUN xmlEntityPtr xmlGetDtdEntity(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlEntity>);
XMLPUBFUN xmlEntityPtr xmlGetParameterEntity(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlEntity>);
#ifdef LIBXML_LEGACY_ENABLED
XMLPUBFUN const xmlChar *xmlEncodeEntities(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *input : itype(_Ptr<const xmlChar>)) : itype(_Ptr<const xmlChar>);
#endif /* LIBXML_LEGACY_ENABLED */
XMLPUBFUN xmlChar *xmlEncodeEntitiesReentrant(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *input : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlEncodeSpecialChars(const xmlDoc *doc : itype(_Ptr<const xmlDoc>), const xmlChar *input : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlEntitiesTablePtr xmlCreateEntitiesTable(void) : itype(_Ptr<xmlEntitiesTable>);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN xmlEntitiesTablePtr xmlCopyEntitiesTable(xmlEntitiesTablePtr table : itype(_Ptr<xmlEntitiesTable>)) : itype(_Ptr<xmlEntitiesTable>);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN void XMLCALL
			xmlFreeEntitiesTable	(xmlEntitiesTablePtr table : itype(_Ptr<xmlEntitiesTable>));
#ifdef LIBXML_OUTPUT_ENABLED
XMLPUBFUN void XMLCALL
			xmlDumpEntitiesTable	(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), xmlEntitiesTablePtr table : itype(_Ptr<xmlEntitiesTable>));
XMLPUBFUN void XMLCALL
			xmlDumpEntityDecl	(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), xmlEntityPtr ent : itype(_Ptr<xmlEntity>));
#endif /* LIBXML_OUTPUT_ENABLED */
#ifdef LIBXML_LEGACY_ENABLED
XMLPUBFUN void XMLCALL
			xmlCleanupPredefinedEntities(void);
#endif /* LIBXML_LEGACY_ENABLED */


#ifdef __cplusplus
}
#endif

# endif /* __XML_ENTITIES_H__ */
