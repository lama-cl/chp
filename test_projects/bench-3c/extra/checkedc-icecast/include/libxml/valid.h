/*
 * Summary: The DTD validation
 * Description: API for the DTD handling and the validity checking
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */


#ifndef __XML_VALID_H__
#define __XML_VALID_H__

#include <libxml/xmlversion.h>
#include <libxml/xmlerror.h>
#include <libxml/tree.h>
#include <libxml/list.h>
#include <libxml/xmlautomata.h>
#include <libxml/xmlregexp.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Validation state added for non-determinist content model.
 */
typedef struct _xmlValidState xmlValidState;
typedef xmlValidState *xmlValidStatePtr;

/**
 * xmlValidityErrorFunc:
 * @ctx:  usually an xmlValidCtxtPtr to a validity error context,
 *        but comes from ctxt->userData (which normally contains such
 *        a pointer); ctxt->userData can be changed by the user.
 * @msg:  the string to format *printf like vararg
 * @...:  remaining arguments to the format
 *
 * Callback called when a validity error is found. This is a message
 * oriented function similar to an *printf function.
 */
typedef void (XMLCDECL *xmlValidityErrorFunc) (void *ctx,
			     const char *msg,
			     ...) LIBXML_ATTR_FORMAT(2,3);

/**
 * xmlValidityWarningFunc:
 * @ctx:  usually an xmlValidCtxtPtr to a validity error context,
 *        but comes from ctxt->userData (which normally contains such
 *        a pointer); ctxt->userData can be changed by the user.
 * @msg:  the string to format *printf like vararg
 * @...:  remaining arguments to the format
 *
 * Callback called when a validity warning is found. This is a message
 * oriented function similar to an *printf function.
 */
typedef void (XMLCDECL *xmlValidityWarningFunc) (void *ctx,
			       const char *msg,
			       ...) LIBXML_ATTR_FORMAT(2,3);

#ifdef IN_LIBXML
/**
 * XML_CTXT_FINISH_DTD_0:
 *
 * Special value for finishDtd field when embedded in an xmlParserCtxt
 */
#define XML_CTXT_FINISH_DTD_0 0xabcd1234
/**
 * XML_CTXT_FINISH_DTD_1:
 *
 * Special value for finishDtd field when embedded in an xmlParserCtxt
 */
#define XML_CTXT_FINISH_DTD_1 0xabcd1235
#endif

/*
 * xmlValidCtxt:
 * An xmlValidCtxt is used for error reporting when validating.
 */
typedef struct _xmlValidCtxt xmlValidCtxt;
typedef xmlValidCtxt *xmlValidCtxtPtr;
struct _xmlValidCtxt {
    void *userData;			/* user specific data block */
    xmlValidityErrorFunc error;		/* the callback in case of errors */
    xmlValidityWarningFunc warning;	/* the callback in case of warning */

    /* Node analysis stack used when validating within entities */
    xmlNodePtr node : itype(_Ptr<xmlNode>);          /* Current parsed Node */
    int                nodeNr;        /* Depth of the parsing stack */
    int                nodeMax;       /* Max depth of the parsing stack */
    xmlNodePtr *nodeTab : itype(_Ptr<xmlNodePtr>);       /* array of nodes */

    unsigned int     finishDtd;       /* finished validating the Dtd ? */
    xmlDocPtr doc : itype(_Ptr<xmlDoc>);       /* the document */
    int                  valid;       /* temporary validity check result */

    /* state state used for non-determinist content validation */
    xmlValidState *vstate : itype(_Ptr<xmlValidState>);        /* current state */
    int                vstateNr;      /* Depth of the validation stack */
    int                vstateMax;     /* Max depth of the validation stack */
    xmlValidState *vstateTab : itype(_Ptr<xmlValidState>);     /* array of validation states */

#ifdef LIBXML_REGEXP_ENABLED
    xmlAutomataPtr am : itype(_Ptr<xmlAutomata>);     /* the automata */
    xmlAutomataStatePtr state : itype(_Ptr<xmlAutomataState>);     /* used to build the automata */
#else
    void                     *am;
    void                  *state;
#endif
};

/*
 * ALL notation declarations are stored in a table.
 * There is one table per DTD.
 */

typedef struct _xmlHashTable xmlNotationTable;
typedef xmlNotationTable *xmlNotationTablePtr;

/*
 * ALL element declarations are stored in a table.
 * There is one table per DTD.
 */

typedef struct _xmlHashTable xmlElementTable;
typedef xmlElementTable *xmlElementTablePtr;

/*
 * ALL attribute declarations are stored in a table.
 * There is one table per DTD.
 */

typedef struct _xmlHashTable xmlAttributeTable;
typedef xmlAttributeTable *xmlAttributeTablePtr;

/*
 * ALL IDs attributes are stored in a table.
 * There is one table per document.
 */

typedef struct _xmlHashTable xmlIDTable;
typedef xmlIDTable *xmlIDTablePtr;

/*
 * ALL Refs attributes are stored in a table.
 * There is one table per document.
 */

typedef struct _xmlHashTable xmlRefTable;
typedef xmlRefTable *xmlRefTablePtr;

/* Notation */
XMLPUBFUN xmlNotationPtr xmlAddNotationDecl(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDtdPtr dtd : itype(_Ptr<xmlDtd>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *PublicID : itype(_Ptr<const xmlChar>), const xmlChar *SystemID : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNotation>);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN xmlNotationTablePtr xmlCopyNotationTable(xmlNotationTablePtr table : itype(_Ptr<xmlNotationTable>)) : itype(_Ptr<xmlNotationTable>);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN void XMLCALL
		xmlFreeNotationTable	(xmlNotationTablePtr table : itype(_Ptr<xmlNotationTable>));
#ifdef LIBXML_OUTPUT_ENABLED
XMLPUBFUN void XMLCALL
		xmlDumpNotationDecl	(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), xmlNotationPtr nota : itype(_Ptr<xmlNotation>));
XMLPUBFUN void XMLCALL
		xmlDumpNotationTable	(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), xmlNotationTablePtr table : itype(_Ptr<xmlNotationTable>));
#endif /* LIBXML_OUTPUT_ENABLED */

/* Element Content */
/* the non Doc version are being deprecated */
XMLPUBFUN xmlElementContentPtr xmlNewElementContent(const xmlChar *name : itype(_Ptr<const xmlChar>), xmlElementContentType type) : itype(_Ptr<xmlElementContent>);
XMLPUBFUN xmlElementContentPtr xmlCopyElementContent(xmlElementContentPtr content : itype(_Ptr<xmlElementContent>)) : itype(_Ptr<xmlElementContent>);
XMLPUBFUN void XMLCALL
		xmlFreeElementContent	(xmlElementContentPtr cur : itype(_Ptr<xmlElementContent>));
/* the new versions with doc argument */
XMLPUBFUN xmlElementContentPtr xmlNewDocElementContent(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *name : itype(_Ptr<const xmlChar>), xmlElementContentType type) : itype(_Ptr<xmlElementContent>);
XMLPUBFUN xmlElementContentPtr xmlCopyDocElementContent(xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlElementContentPtr content : itype(_Ptr<xmlElementContent>)) : itype(_Ptr<xmlElementContent>);
XMLPUBFUN void XMLCALL
		xmlFreeDocElementContent(xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlElementContentPtr cur : itype(_Ptr<xmlElementContent>));
XMLPUBFUN void XMLCALL
		xmlSnprintfElementContent(char *buf : itype(_Ptr<char>), int size, xmlElementContentPtr content : itype(_Ptr<xmlElementContent>), int englob);
#ifdef LIBXML_OUTPUT_ENABLED
/* DEPRECATED */
XMLPUBFUN void XMLCALL
		xmlSprintfElementContent(char *buf : itype(_Ptr<char>), xmlElementContentPtr content : itype(_Ptr<xmlElementContent>), int englob);
#endif /* LIBXML_OUTPUT_ENABLED */
/* DEPRECATED */

/* Element */
XMLPUBFUN xmlElementPtr xmlAddElementDecl(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDtdPtr dtd : itype(_Ptr<xmlDtd>), const xmlChar *name : itype(_Ptr<const xmlChar>), xmlElementTypeVal type, xmlElementContentPtr content : itype(_Ptr<xmlElementContent>)) : itype(_Ptr<xmlElement>);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN xmlElementTablePtr xmlCopyElementTable(xmlElementTablePtr table : itype(_Ptr<xmlElementTable>)) : itype(_Ptr<xmlElementTable>);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN void XMLCALL
		xmlFreeElementTable	(xmlElementTablePtr table : itype(_Ptr<xmlElementTable>));
#ifdef LIBXML_OUTPUT_ENABLED
XMLPUBFUN void XMLCALL
		xmlDumpElementTable	(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), xmlElementTablePtr table : itype(_Ptr<xmlElementTable>));
XMLPUBFUN void XMLCALL
		xmlDumpElementDecl	(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), xmlElementPtr elem : itype(_Ptr<xmlElement>));
#endif /* LIBXML_OUTPUT_ENABLED */

/* Enumeration */
XMLPUBFUN xmlEnumerationPtr xmlCreateEnumeration(const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlEnumeration>);
XMLPUBFUN void XMLCALL
		xmlFreeEnumeration	(xmlEnumerationPtr cur : itype(_Ptr<xmlEnumeration>));
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN xmlEnumerationPtr xmlCopyEnumeration(xmlEnumerationPtr cur : itype(_Ptr<xmlEnumeration>)) : itype(_Ptr<xmlEnumeration>);
#endif /* LIBXML_TREE_ENABLED */

/* Attribute */
XMLPUBFUN xmlAttributePtr xmlAddAttributeDecl(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDtdPtr dtd : itype(_Ptr<xmlDtd>), const xmlChar *elem : itype(_Ptr<const xmlChar>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *ns : itype(_Ptr<const xmlChar>), xmlAttributeType type, xmlAttributeDefault def, const xmlChar *defaultValue : itype(_Ptr<const xmlChar>), xmlEnumerationPtr tree : itype(_Ptr<xmlEnumeration>)) : itype(_Ptr<xmlAttribute>);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN xmlAttributeTablePtr xmlCopyAttributeTable(xmlAttributeTablePtr table : itype(_Ptr<xmlAttributeTable>)) : itype(_Ptr<xmlAttributeTable>);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN void XMLCALL
		xmlFreeAttributeTable  (xmlAttributeTablePtr table : itype(_Ptr<xmlAttributeTable>));
#ifdef LIBXML_OUTPUT_ENABLED
XMLPUBFUN void XMLCALL
		xmlDumpAttributeTable  (xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), xmlAttributeTablePtr table : itype(_Ptr<xmlAttributeTable>));
XMLPUBFUN void XMLCALL
		xmlDumpAttributeDecl   (xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), xmlAttributePtr attr : itype(_Ptr<xmlAttribute>));
#endif /* LIBXML_OUTPUT_ENABLED */

/* IDs */
XMLPUBFUN xmlIDPtr xmlAddID(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *value : itype(_Ptr<const xmlChar>), xmlAttrPtr attr : itype(_Ptr<xmlAttr>)) : itype(_Ptr<xmlID>);
XMLPUBFUN void XMLCALL
		xmlFreeIDTable	       (xmlIDTablePtr table : itype(_Ptr<xmlIDTable>));
XMLPUBFUN xmlAttrPtr xmlGetID(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *ID : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlAttr>);
XMLPUBFUN int XMLCALL
		xmlIsID		       (xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr elem : itype(_Ptr<xmlNode>), xmlAttrPtr attr : itype(_Ptr<xmlAttr>));
XMLPUBFUN int XMLCALL
		xmlRemoveID	       (xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlAttrPtr attr : itype(_Ptr<xmlAttr>));

/* IDREFs */
XMLPUBFUN xmlRefPtr xmlAddRef(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *value : itype(_Ptr<const xmlChar>), xmlAttrPtr attr : itype(_Ptr<xmlAttr>)) : itype(_Ptr<xmlRef>);
XMLPUBFUN void XMLCALL
		xmlFreeRefTable	       (xmlRefTablePtr table : itype(_Ptr<xmlRefTable>));
XMLPUBFUN int XMLCALL
		xmlIsRef	       (xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr elem : itype(_Ptr<xmlNode>), xmlAttrPtr attr : itype(_Ptr<xmlAttr>));
XMLPUBFUN int XMLCALL
		xmlRemoveRef	       (xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlAttrPtr attr : itype(_Ptr<xmlAttr>));
XMLPUBFUN xmlListPtr xmlGetRefs(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *ID : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlList>);

/**
 * The public function calls related to validity checking.
 */
#ifdef LIBXML_VALID_ENABLED
/* Allocate/Release Validation Contexts */
XMLPUBFUN xmlValidCtxtPtr xmlNewValidCtxt(void) : itype(_Ptr<xmlValidCtxt>);
XMLPUBFUN void XMLCALL
		xmlFreeValidCtxt(xmlValidCtxtPtr : itype(_Ptr<xmlValidCtxt>));

XMLPUBFUN int XMLCALL
		xmlValidateRoot		(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>));
XMLPUBFUN int XMLCALL
		xmlValidateElementDecl	(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlElementPtr elem : itype(_Ptr<xmlElement>));
XMLPUBFUN xmlChar *xmlValidNormalizeAttributeValue(xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr elem : itype(_Ptr<xmlNode>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *value : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlValidCtxtNormalizeAttributeValue(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr elem : itype(_Ptr<xmlNode>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *value : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN int XMLCALL
		xmlValidateAttributeDecl(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlAttributePtr attr : itype(_Ptr<xmlAttribute>));
XMLPUBFUN int XMLCALL
		xmlValidateAttributeValue(xmlAttributeType type, const xmlChar *value : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
		xmlValidateNotationDecl	(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNotationPtr nota : itype(_Ptr<xmlNotation>));
XMLPUBFUN int XMLCALL
		xmlValidateDtd		(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlDtdPtr dtd : itype(_Ptr<xmlDtd>));
XMLPUBFUN int XMLCALL
		xmlValidateDtdFinal	(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>));
XMLPUBFUN int XMLCALL
		xmlValidateDocument	(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>));
XMLPUBFUN int XMLCALL
		xmlValidateElement	(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr elem : itype(_Ptr<xmlNode>));
XMLPUBFUN int XMLCALL
		xmlValidateOneElement	(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr elem : itype(_Ptr<xmlNode>));
XMLPUBFUN int XMLCALL
		xmlValidateOneAttribute	(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr elem : itype(_Ptr<xmlNode>), xmlAttrPtr attr : itype(_Ptr<xmlAttr>), const xmlChar *value : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
		xmlValidateOneNamespace	(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr elem : itype(_Ptr<xmlNode>), const xmlChar *prefix : itype(_Ptr<const xmlChar>), xmlNsPtr ns : itype(_Ptr<xmlNs>), const xmlChar *value : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
		xmlValidateDocumentFinal(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>));
#endif /* LIBXML_VALID_ENABLED */

#if defined(LIBXML_VALID_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN int XMLCALL
		xmlValidateNotationUse	(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *notationName : itype(_Ptr<const xmlChar>));
#endif /* LIBXML_VALID_ENABLED or LIBXML_SCHEMAS_ENABLED */

XMLPUBFUN int XMLCALL
		xmlIsMixedElement	(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *name : itype(_Ptr<const xmlChar>));
XMLPUBFUN xmlAttributePtr xmlGetDtdAttrDesc(xmlDtdPtr dtd : itype(_Ptr<xmlDtd>), const xmlChar *elem : itype(_Ptr<const xmlChar>), const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlAttribute>);
XMLPUBFUN xmlAttributePtr xmlGetDtdQAttrDesc(xmlDtdPtr dtd : itype(_Ptr<xmlDtd>), const xmlChar *elem : itype(_Ptr<const xmlChar>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *prefix : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlAttribute>);
XMLPUBFUN xmlNotationPtr xmlGetDtdNotationDesc(xmlDtdPtr dtd : itype(_Ptr<xmlDtd>), const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNotation>);
XMLPUBFUN xmlElementPtr xmlGetDtdQElementDesc(xmlDtdPtr dtd : itype(_Ptr<xmlDtd>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *prefix : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlElement>);
XMLPUBFUN xmlElementPtr xmlGetDtdElementDesc(xmlDtdPtr dtd : itype(_Ptr<xmlDtd>), const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlElement>);

#ifdef LIBXML_VALID_ENABLED

XMLPUBFUN int XMLCALL
		xmlValidGetPotentialChildren(xmlElementContent *ctree : itype(_Ptr<xmlElementContent>), const xmlChar **names : itype(_Ptr<_Ptr<const xmlChar>>), int *len : itype(_Ptr<int>), int max);

XMLPUBFUN int XMLCALL
		xmlValidGetValidElements(xmlNode *prev : itype(_Ptr<xmlNode>), xmlNode *next : itype(_Ptr<xmlNode>), const xmlChar **names : itype(_Ptr<_Ptr<const xmlChar>>), int max);
XMLPUBFUN int XMLCALL
		xmlValidateNameValue	(const xmlChar *value : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
		xmlValidateNamesValue	(const xmlChar *value : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
		xmlValidateNmtokenValue	(const xmlChar *value : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
		xmlValidateNmtokensValue(const xmlChar *value : itype(_Ptr<const xmlChar>));

#ifdef LIBXML_REGEXP_ENABLED
/*
 * Validation based on the regexp support
 */
XMLPUBFUN int XMLCALL
		xmlValidBuildContentModel(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlElementPtr elem : itype(_Ptr<xmlElement>));

XMLPUBFUN int XMLCALL
		xmlValidatePushElement	(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr elem : itype(_Ptr<xmlNode>), const xmlChar *qname : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
		xmlValidatePushCData	(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), const xmlChar *data : itype(_Ptr<const xmlChar>), int len);
XMLPUBFUN int XMLCALL
		xmlValidatePopElement	(xmlValidCtxtPtr ctxt : itype(_Ptr<xmlValidCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr elem : itype(_Ptr<xmlNode>), const xmlChar *qname : itype(_Ptr<const xmlChar>));
#endif /* LIBXML_REGEXP_ENABLED */
#endif /* LIBXML_VALID_ENABLED */
#ifdef __cplusplus
}
#endif
#endif /* __XML_VALID_H__ */
