/*
 * Summary: interfaces for tree manipulation
 * Description: this module describes the structures found in an tree resulting
 *              from an XML or HTML parsing, as well as the API provided for
 *              various processing on that tree
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_TREE_H__
#define __XML_TREE_H__

#include <stdio.h>
#include <limits.h>
#include <libxml/xmlversion.h>
#include <libxml/xmlstring.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Some of the basic types pointer to structures:
 */
/* xmlIO.h */
typedef struct _xmlParserInputBuffer xmlParserInputBuffer;
typedef _Ptr<xmlParserInputBuffer> xmlParserInputBufferPtr;

typedef struct _xmlOutputBuffer xmlOutputBuffer;
typedef _Ptr<xmlOutputBuffer> xmlOutputBufferPtr;

/* parser.h */
typedef struct _xmlParserInput xmlParserInput;
typedef _Ptr<xmlParserInput> xmlParserInputPtr;

typedef struct _xmlParserCtxt xmlParserCtxt;
typedef _Ptr<xmlParserCtxt> xmlParserCtxtPtr;

typedef struct _xmlSAXLocator xmlSAXLocator;
typedef _Ptr<xmlSAXLocator> xmlSAXLocatorPtr;

typedef struct _xmlSAXHandler xmlSAXHandler;
typedef _Ptr<xmlSAXHandler> xmlSAXHandlerPtr;

/* entities.h */
typedef struct _xmlEntity xmlEntity;
typedef _Ptr<xmlEntity> xmlEntityPtr;

/**
 * BASE_BUFFER_SIZE:
 *
 * default buffer size 4000.
 */
#define BASE_BUFFER_SIZE 4096

/**
 * LIBXML_NAMESPACE_DICT:
 *
 * Defines experimental behaviour:
 * 1) xmlNs gets an additional field @context (a xmlDoc)
 * 2) when creating a tree, xmlNs->href is stored in the dict of xmlDoc.
 */
/* #define LIBXML_NAMESPACE_DICT */

/**
 * xmlBufferAllocationScheme:
 *
 * A buffer allocation scheme can be defined to either match exactly the
 * need or double it's allocated size each time it is found too small.
 */

typedef enum {
    XML_BUFFER_ALLOC_DOUBLEIT,	/* double each time one need to grow */
    XML_BUFFER_ALLOC_EXACT,	/* grow only to the minimal size */
    XML_BUFFER_ALLOC_IMMUTABLE, /* immutable buffer */
    XML_BUFFER_ALLOC_IO,	/* special allocation scheme used for I/O */
    XML_BUFFER_ALLOC_HYBRID,	/* exact up to a threshold, and doubleit thereafter */
    XML_BUFFER_ALLOC_BOUNDED	/* limit the upper size of the buffer */
} xmlBufferAllocationScheme;

/**
 * xmlBuffer:
 *
 * A buffer structure, this old construct is limited to 2GB and
 * is being deprecated, use API with xmlBuf instead
 */
typedef struct _xmlBuffer xmlBuffer;
typedef xmlBuffer *xmlBufferPtr;
struct _xmlBuffer {
    xmlChar *content : itype(_Ptr<xmlChar>);		/* The buffer content UTF8 */
    unsigned int use;		/* The buffer size used */
    unsigned int size;		/* The buffer size */
    xmlBufferAllocationScheme alloc; /* The realloc method */
    xmlChar *contentIO : itype(_Ptr<xmlChar>);		/* in IO mode we may have a different base */
};

/**
 * xmlBuf:
 *
 * A buffer structure, new one, the actual structure internals are not public
 */

typedef struct _xmlBuf xmlBuf;

/**
 * xmlBufPtr:
 *
 * A pointer to a buffer structure, the actual structure internals are not
 * public
 */

typedef xmlBuf *xmlBufPtr;

/*
 * A few public routines for xmlBuf. As those are expected to be used
 * mostly internally the bulk of the routines are internal in buf.h
 */
XMLPUBFUN xmlChar *xmlBufContent(const xmlBuf *buf : itype(_Ptr<const xmlBuf>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlBufEnd(xmlBufPtr buf : itype(_Ptr<xmlBuf>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN size_t XMLCALL         xmlBufUse      (const xmlBufPtr buf : itype(const _Ptr<xmlBuf>));
XMLPUBFUN size_t XMLCALL         xmlBufShrink	(xmlBufPtr buf : itype(_Ptr<xmlBuf>), size_t len);

/*
 * LIBXML2_NEW_BUFFER:
 *
 * Macro used to express that the API use the new buffers for
 * xmlParserInputBuffer and xmlOutputBuffer. The change was
 * introduced in 2.9.0.
 */
#define LIBXML2_NEW_BUFFER

/**
 * XML_XML_NAMESPACE:
 *
 * This is the namespace for the special xml: prefix predefined in the
 * XML Namespace specification.
 */
#define XML_XML_NAMESPACE \
    (const xmlChar *) "http://www.w3.org/XML/1998/namespace"

/**
 * XML_XML_ID:
 *
 * This is the name for the special xml:id attribute
 */
#define XML_XML_ID (const xmlChar *) "xml:id"

/*
 * The different element types carried by an XML tree.
 *
 * NOTE: This is synchronized with DOM Level1 values
 *       See http://www.w3.org/TR/REC-DOM-Level-1/
 *
 * Actually this had diverged a bit, and now XML_DOCUMENT_TYPE_NODE should
 * be deprecated to use an XML_DTD_NODE.
 */
typedef enum {
    XML_ELEMENT_NODE=		1,
    XML_ATTRIBUTE_NODE=		2,
    XML_TEXT_NODE=		3,
    XML_CDATA_SECTION_NODE=	4,
    XML_ENTITY_REF_NODE=	5,
    XML_ENTITY_NODE=		6,
    XML_PI_NODE=		7,
    XML_COMMENT_NODE=		8,
    XML_DOCUMENT_NODE=		9,
    XML_DOCUMENT_TYPE_NODE=	10,
    XML_DOCUMENT_FRAG_NODE=	11,
    XML_NOTATION_NODE=		12,
    XML_HTML_DOCUMENT_NODE=	13,
    XML_DTD_NODE=		14,
    XML_ELEMENT_DECL=		15,
    XML_ATTRIBUTE_DECL=		16,
    XML_ENTITY_DECL=		17,
    XML_NAMESPACE_DECL=		18,
    XML_XINCLUDE_START=		19,
    XML_XINCLUDE_END=		20
#ifdef LIBXML_DOCB_ENABLED
   ,XML_DOCB_DOCUMENT_NODE=	21
#endif
} xmlElementType;


/**
 * xmlNotation:
 *
 * A DTD Notation definition.
 */

typedef struct _xmlNotation xmlNotation;
typedef xmlNotation *xmlNotationPtr;
struct _xmlNotation {
    const xmlChar *name : itype(_Nt_array_ptr<const xmlChar>);	        /* Notation name */
    const xmlChar *PublicID : itype(_Ptr<const xmlChar>);	/* Public identifier, if any */
    const xmlChar *SystemID : itype(_Ptr<const xmlChar>);	/* System identifier, if any */
};

/**
 * xmlAttributeType:
 *
 * A DTD Attribute type definition.
 */

typedef enum {
    XML_ATTRIBUTE_CDATA = 1,
    XML_ATTRIBUTE_ID,
    XML_ATTRIBUTE_IDREF	,
    XML_ATTRIBUTE_IDREFS,
    XML_ATTRIBUTE_ENTITY,
    XML_ATTRIBUTE_ENTITIES,
    XML_ATTRIBUTE_NMTOKEN,
    XML_ATTRIBUTE_NMTOKENS,
    XML_ATTRIBUTE_ENUMERATION,
    XML_ATTRIBUTE_NOTATION
} xmlAttributeType;

/**
 * xmlAttributeDefault:
 *
 * A DTD Attribute default definition.
 */

typedef enum {
    XML_ATTRIBUTE_NONE = 1,
    XML_ATTRIBUTE_REQUIRED,
    XML_ATTRIBUTE_IMPLIED,
    XML_ATTRIBUTE_FIXED
} xmlAttributeDefault;

/**
 * xmlEnumeration:
 *
 * List structure used when there is an enumeration in DTDs.
 */

typedef struct _xmlEnumeration xmlEnumeration;
typedef xmlEnumeration *xmlEnumerationPtr;
struct _xmlEnumeration {
    struct _xmlEnumeration *next : itype(_Ptr<struct _xmlEnumeration>);	/* next one */
    const xmlChar *name : itype(_Ptr<const xmlChar>);	/* Enumeration name */
};

/**
 * xmlAttribute:
 *
 * An Attribute declaration in a DTD.
 */

typedef struct _xmlAttribute xmlAttribute;
typedef xmlAttribute *xmlAttributePtr;
struct _xmlAttribute {
    void           *_private;	        /* application data */
    xmlElementType          type;       /* XML_ATTRIBUTE_DECL, must be second ! */
    const xmlChar *name : itype(_Ptr<const xmlChar>);	/* Attribute name */
    struct _xmlNode *children : itype(_Ptr<struct _xmlNode>);	/* NULL */
    struct _xmlNode *last : itype(_Ptr<struct _xmlNode>);	/* NULL */
    struct _xmlDtd *parent : itype(_Ptr<struct _xmlDtd>);	/* -> DTD */
    struct _xmlNode *next : itype(_Ptr<struct _xmlNode>);	/* next sibling link  */
    struct _xmlNode *prev : itype(_Ptr<struct _xmlNode>);	/* previous sibling link  */
    struct _xmlDoc *doc : itype(_Ptr<struct _xmlDoc>);       /* the containing document */

    struct _xmlAttribute *nexth : itype(_Ptr<struct _xmlAttribute>);	/* next in hash table */
    xmlAttributeType       atype;	/* The attribute type */
    xmlAttributeDefault      def;	/* the default */
    const xmlChar *defaultValue : itype(_Ptr<const xmlChar>);	/* or the default value */
    xmlEnumerationPtr tree : itype(_Ptr<xmlEnumeration>);       /* or the enumeration tree if any */
    const xmlChar *prefix : itype(_Ptr<const xmlChar>);	/* the namespace prefix if any */
    const xmlChar *elem : itype(_Ptr<const xmlChar>);	/* Element holding the attribute */
};

/**
 * xmlElementContentType:
 *
 * Possible definitions of element content types.
 */
typedef enum {
    XML_ELEMENT_CONTENT_PCDATA = 1,
    XML_ELEMENT_CONTENT_ELEMENT,
    XML_ELEMENT_CONTENT_SEQ,
    XML_ELEMENT_CONTENT_OR
} xmlElementContentType;

/**
 * xmlElementContentOccur:
 *
 * Possible definitions of element content occurrences.
 */
typedef enum {
    XML_ELEMENT_CONTENT_ONCE = 1,
    XML_ELEMENT_CONTENT_OPT,
    XML_ELEMENT_CONTENT_MULT,
    XML_ELEMENT_CONTENT_PLUS
} xmlElementContentOccur;

/**
 * xmlElementContent:
 *
 * An XML Element content as stored after parsing an element definition
 * in a DTD.
 */

typedef struct _xmlElementContent xmlElementContent;
typedef xmlElementContent *xmlElementContentPtr;
struct _xmlElementContent {
    xmlElementContentType     type;	/* PCDATA, ELEMENT, SEQ or OR */
    xmlElementContentOccur    ocur;	/* ONCE, OPT, MULT or PLUS */
    const xmlChar *name : itype(_Ptr<const xmlChar>);	/* Element name */
    struct _xmlElementContent *c1 : itype(_Ptr<struct _xmlElementContent>);	/* first child */
    struct _xmlElementContent *c2 : itype(_Ptr<struct _xmlElementContent>);	/* second child */
    struct _xmlElementContent *parent : itype(_Ptr<struct _xmlElementContent>);	/* parent */
    const xmlChar *prefix : itype(_Ptr<const xmlChar>);	/* Namespace prefix */
};

/**
 * xmlElementTypeVal:
 *
 * The different possibilities for an element content type.
 */

typedef enum {
    XML_ELEMENT_TYPE_UNDEFINED = 0,
    XML_ELEMENT_TYPE_EMPTY = 1,
    XML_ELEMENT_TYPE_ANY,
    XML_ELEMENT_TYPE_MIXED,
    XML_ELEMENT_TYPE_ELEMENT
} xmlElementTypeVal;

#ifdef __cplusplus
}
#endif
#include <libxml/xmlregexp.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * xmlElement:
 *
 * An XML Element declaration from a DTD.
 */

typedef struct _xmlElement xmlElement;
typedef xmlElement *xmlElementPtr;
struct _xmlElement {
    void           *_private;	        /* application data */
    xmlElementType          type;       /* XML_ELEMENT_DECL, must be second ! */
    const xmlChar *name : itype(_Ptr<const xmlChar>);	/* Element name */
    struct _xmlNode *children : itype(_Ptr<struct _xmlNode>);	/* NULL */
    struct _xmlNode *last : itype(_Ptr<struct _xmlNode>);	/* NULL */
    struct _xmlDtd *parent : itype(_Ptr<struct _xmlDtd>);	/* -> DTD */
    struct _xmlNode *next : itype(_Ptr<struct _xmlNode>);	/* next sibling link  */
    struct _xmlNode *prev : itype(_Ptr<struct _xmlNode>);	/* previous sibling link  */
    struct _xmlDoc *doc : itype(_Ptr<struct _xmlDoc>);       /* the containing document */

    xmlElementTypeVal      etype;	/* The type */
    xmlElementContentPtr content : itype(_Ptr<xmlElementContent>);	/* the allowed element content */
    xmlAttributePtr attributes : itype(_Ptr<xmlAttribute>);	/* List of the declared attributes */
    const xmlChar *prefix : itype(_Ptr<const xmlChar>);	/* the namespace prefix if any */
#ifdef LIBXML_REGEXP_ENABLED
    xmlRegexpPtr contModel : itype(_Ptr<xmlRegexp>);	/* the validating regexp */
#else
    void	      *contModel;
#endif
};


/**
 * XML_LOCAL_NAMESPACE:
 *
 * A namespace declaration node.
 */
#define XML_LOCAL_NAMESPACE XML_NAMESPACE_DECL
typedef xmlElementType xmlNsType;

/**
 * xmlNs:
 *
 * An XML namespace.
 * Note that prefix == NULL is valid, it defines the default namespace
 * within the subtree (until overridden).
 *
 * xmlNsType is unified with xmlElementType.
 */

typedef struct _xmlNs xmlNs;
typedef xmlNs *xmlNsPtr;
struct _xmlNs {
    struct _xmlNs *next : itype(_Ptr<struct _xmlNs>);	/* next Ns link for this node  */
    xmlNsType      type;	/* global or local */
    const xmlChar *href : itype(_Ptr<const xmlChar>);	/* URL for the namespace */
    const xmlChar *prefix : itype(_Ptr<const xmlChar>);	/* prefix for the namespace */
    void           *_private;   /* application data */
    struct _xmlDoc *context : itype(_Ptr<struct _xmlDoc>);		/* normally an xmlDoc */
};

/**
 * xmlDtd:
 *
 * An XML DTD, as defined by <!DOCTYPE ... There is actually one for
 * the internal subset and for the external subset.
 */
typedef struct _xmlDtd xmlDtd;
typedef xmlDtd *xmlDtdPtr;
struct _xmlDtd {
    void           *_private;	/* application data */
    xmlElementType  type;       /* XML_DTD_NODE, must be second ! */
    const xmlChar *name : itype(_Ptr<const xmlChar>);	/* Name of the DTD */
    struct _xmlNode *children : itype(_Ptr<struct _xmlNode>);	/* the value of the property link */
    struct _xmlNode *last : itype(_Ptr<struct _xmlNode>);	/* last child link */
    struct _xmlDoc *parent : itype(_Ptr<struct _xmlDoc>);	/* child->parent link */
    struct _xmlNode *next : itype(_Ptr<struct _xmlNode>);	/* next sibling link  */
    struct _xmlNode *prev : itype(_Ptr<struct _xmlNode>);	/* previous sibling link  */
    struct _xmlDoc *doc : itype(_Ptr<struct _xmlDoc>);	/* the containing document */

    /* End of common part */
    void          *notations;   /* Hash table for notations if any */
    void          *elements;    /* Hash table for elements if any */
    void          *attributes;  /* Hash table for attributes if any */
    void          *entities;    /* Hash table for entities if any */
    const xmlChar *ExternalID : itype(_Ptr<const xmlChar>);	/* External identifier for PUBLIC DTD */
    const xmlChar *SystemID : itype(_Ptr<const xmlChar>);	/* URI for a SYSTEM or PUBLIC DTD */
    void          *pentities;   /* Hash table for param entities if any */
};

/**
 * xmlAttr:
 *
 * An attribute on an XML node.
 */
typedef struct _xmlAttr xmlAttr;
typedef xmlAttr *xmlAttrPtr;
struct _xmlAttr {
    void           *_private;	/* application data */
    xmlElementType   type;      /* XML_ATTRIBUTE_NODE, must be second ! */
    const xmlChar *name : itype(_Ptr<const xmlChar>);      /* the name of the property */
    struct _xmlNode *children : itype(_Ptr<struct _xmlNode>);	/* the value of the property */
    struct _xmlNode *last : itype(_Ptr<struct _xmlNode>);	/* NULL */
    struct _xmlNode *parent : itype(_Ptr<struct _xmlNode>);	/* child->parent link */
    struct _xmlAttr *next : itype(_Ptr<struct _xmlAttr>);	/* next sibling link  */
    struct _xmlAttr *prev : itype(_Ptr<struct _xmlAttr>);	/* previous sibling link  */
    struct _xmlDoc *doc : itype(_Ptr<struct _xmlDoc>);	/* the containing document */
    xmlNs *ns : itype(_Ptr<xmlNs>);        /* pointer to the associated namespace */
    xmlAttributeType atype;     /* the attribute type if validating */
    void            *psvi;	/* for type/PSVI informations */
};

/**
 * xmlID:
 *
 * An XML ID instance.
 */

typedef struct _xmlID xmlID;
typedef xmlID *xmlIDPtr;
struct _xmlID {
    struct _xmlID *next : itype(_Ptr<struct _xmlID>);	/* next ID */
    const xmlChar *value : itype(_Ptr<const xmlChar>);	/* The ID name */
    xmlAttrPtr attr : itype(_Ptr<xmlAttr>);	/* The attribute holding it */
    const xmlChar *name : itype(_Ptr<const xmlChar>);	/* The attribute if attr is not available */
    int               lineno;	/* The line number if attr is not available */
    struct _xmlDoc *doc : itype(_Ptr<struct _xmlDoc>);	/* The document holding the ID */
};

/**
 * xmlRef:
 *
 * An XML IDREF instance.
 */

typedef struct _xmlRef xmlRef;
typedef xmlRef *xmlRefPtr;
struct _xmlRef {
    struct _xmlRef *next : itype(_Ptr<struct _xmlRef>);	/* next Ref */
    const xmlChar *value : itype(_Ptr<const xmlChar>);	/* The Ref name */
    xmlAttrPtr attr : itype(_Ptr<xmlAttr>);	/* The attribute holding it */
    const xmlChar *name : itype(_Ptr<const xmlChar>);	/* The attribute if attr is not available */
    int               lineno;	/* The line number if attr is not available */
};

/**
 * xmlNode:
 *
 * A node in an XML tree.
 */
typedef struct _xmlNode xmlNode;
typedef _Ptr<xmlNode> xmlNodePtr;
struct _xmlNode {
    void           *_private;	/* application data */
    xmlElementType   type;	/* type number, must be second ! */
    const xmlChar *name : itype(_Nt_array_ptr<const xmlChar>);      /* the name of the node, or the entity */
    struct _xmlNode *children : itype(_Ptr<struct _xmlNode>);	/* parent->childs link */
    struct _xmlNode *last : itype(_Ptr<struct _xmlNode>);	/* last child link */
    struct _xmlNode *parent : itype(_Ptr<struct _xmlNode>);	/* child->parent link */
    struct _xmlNode *next : itype(_Ptr<struct _xmlNode>);	/* next sibling link  */
    struct _xmlNode *prev : itype(_Ptr<struct _xmlNode>);	/* previous sibling link  */
    struct _xmlDoc *doc : itype(_Ptr<struct _xmlDoc>);	/* the containing document */

    /* End of common part */
    xmlNs *ns : itype(_Ptr<xmlNs>);        /* pointer to the associated namespace */
    xmlChar *content : itype(_Ptr<xmlChar>);   /* the content */
    struct _xmlAttr *properties : itype(_Ptr<struct _xmlAttr>);/* properties list */
    xmlNs *nsDef : itype(_Ptr<xmlNs>);     /* namespace definitions on this node */
    void            *psvi;	/* for type/PSVI informations */
    unsigned short   line;	/* line number */
    unsigned short   extra;	/* extra data for XPath/XSLT */
};

/**
 * XML_GET_CONTENT:
 *
 * Macro to extract the content pointer of a node.
 */
#define XML_GET_CONTENT(n)					\
    ((n)->type == XML_ELEMENT_NODE ? NULL : (n)->content)

/**
 * XML_GET_LINE:
 *
 * Macro to extract the line number of an element node.
 */
#define XML_GET_LINE(n)						\
    (xmlGetLineNo(n))

/**
 * xmlDocProperty
 *
 * Set of properties of the document as found by the parser
 * Some of them are linked to similarly named xmlParserOption
 */
typedef enum {
    XML_DOC_WELLFORMED		= 1<<0, /* document is XML well formed */
    XML_DOC_NSVALID		= 1<<1, /* document is Namespace valid */
    XML_DOC_OLD10		= 1<<2, /* parsed with old XML-1.0 parser */
    XML_DOC_DTDVALID		= 1<<3, /* DTD validation was successful */
    XML_DOC_XINCLUDE		= 1<<4, /* XInclude substitution was done */
    XML_DOC_USERBUILT		= 1<<5, /* Document was built using the API
                                           and not by parsing an instance */
    XML_DOC_INTERNAL		= 1<<6, /* built for internal processing */
    XML_DOC_HTML		= 1<<7  /* parsed or built HTML document */
} xmlDocProperties;

/**
 * xmlDoc:
 *
 * An XML document.
 */
typedef struct _xmlDoc xmlDoc;
typedef _Ptr<xmlDoc> xmlDocPtr;
struct _xmlDoc {
    void           *_private;	/* application data */
    xmlElementType  type;       /* XML_DOCUMENT_NODE, must be second ! */
    char *name : itype(_Ptr<char>);	/* name/filename/URI of the document */
    struct _xmlNode *children : itype(_Ptr<struct _xmlNode>);	/* the document tree */
    struct _xmlNode *last : itype(_Ptr<struct _xmlNode>);	/* last child link */
    struct _xmlNode *parent : itype(_Ptr<struct _xmlNode>);	/* child->parent link */
    struct _xmlNode *next : itype(_Ptr<struct _xmlNode>);	/* next sibling link  */
    struct _xmlNode *prev : itype(_Ptr<struct _xmlNode>);	/* previous sibling link  */
    struct _xmlDoc *doc : itype(_Ptr<struct _xmlDoc>);	/* autoreference to itself */

    /* End of common part */
    int             compression;/* level of zlib compression */
    int             standalone; /* standalone document (no external refs)
				     1 if standalone="yes"
				     0 if standalone="no"
				    -1 if there is no XML declaration
				    -2 if there is an XML declaration, but no
					standalone attribute was specified */
    struct _xmlDtd *intSubset : itype(_Ptr<struct _xmlDtd>);	/* the document internal subset */
    struct _xmlDtd *extSubset : itype(_Ptr<struct _xmlDtd>);	/* the document external subset */
    struct _xmlNs *oldNs : itype(_Ptr<struct _xmlNs>);	/* Global namespace, the old way */
    const xmlChar *version : itype(_Ptr<const xmlChar>);	/* the XML version string */
    const xmlChar *encoding : itype(_Ptr<const xmlChar>);   /* external initial encoding, if any */
    void           *ids;        /* Hash table for ID attributes if any */
    void           *refs;       /* Hash table for IDREFs attributes if any */
    const xmlChar *URL : itype(_Ptr<const xmlChar>);	/* The URI for that document */
    int             charset;    /* Internal flag for charset handling,
				   actually an xmlCharEncoding */
    struct _xmlDict *dict : itype(_Ptr<struct _xmlDict>);      /* dict used to allocate names or NULL */
    void           *psvi;	/* for type/PSVI informations */
    int             parseFlags;	/* set of xmlParserOption used to parse the
				   document */
    int             properties;	/* set of xmlDocProperties for this document
				   set at the end of parsing */
};


typedef struct _xmlDOMWrapCtxt xmlDOMWrapCtxt;
typedef xmlDOMWrapCtxt *xmlDOMWrapCtxtPtr;

/**
 * xmlDOMWrapAcquireNsFunction:
 * @ctxt:  a DOM wrapper context
 * @node:  the context node (element or attribute)
 * @nsName:  the requested namespace name
 * @nsPrefix:  the requested namespace prefix
 *
 * A function called to acquire namespaces (xmlNs) from the wrapper.
 *
 * Returns an xmlNsPtr or NULL in case of an error.
 */
typedef xmlNsPtr (*xmlDOMWrapAcquireNsFunction) (xmlDOMWrapCtxtPtr ctxt,
						 xmlNodePtr node,
						 const xmlChar *nsName,
						 const xmlChar *nsPrefix);

/**
 * xmlDOMWrapCtxt:
 *
 * Context for DOM wrapper-operations.
 */
struct _xmlDOMWrapCtxt {
    void * _private;
    /*
    * The type of this context, just in case we need specialized
    * contexts in the future.
    */
    int type;
    /*
    * Internal namespace map used for various operations.
    */
    void * namespaceMap;
    /*
    * Use this one to acquire an xmlNsPtr intended for node->ns.
    * (Note that this is not intended for elem->nsDef).
    */
    xmlDOMWrapAcquireNsFunction getNsForNodeFunc : itype(_Ptr<_Ptr<xmlNs> (_Ptr<xmlDOMWrapCtxt>, _Ptr<xmlNode>, _Ptr<const xmlChar>, _Ptr<const xmlChar>)>);
};

/**
 * xmlChildrenNode:
 *
 * Macro for compatibility naming layer with libxml1. Maps
 * to "children."
 */
#ifndef xmlChildrenNode
#define xmlChildrenNode children
#endif

/**
 * xmlRootNode:
 *
 * Macro for compatibility naming layer with libxml1. Maps
 * to "children".
 */
#ifndef xmlRootNode
#define xmlRootNode children
#endif

/*
 * Variables.
 */

/*
 * Some helper functions
 */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XPATH_ENABLED) || \
    defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_DEBUG_ENABLED) || \
    defined (LIBXML_HTML_ENABLED) || defined(LIBXML_SAX1_ENABLED) || \
    defined(LIBXML_HTML_ENABLED) || defined(LIBXML_WRITER_ENABLED) || \
    defined(LIBXML_DOCB_ENABLED) || defined(LIBXML_LEGACY_ENABLED)
XMLPUBFUN int XMLCALL
		xmlValidateNCName	(const xmlChar *value : itype(_Ptr<const xmlChar>), int space);
#endif

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN int XMLCALL
		xmlValidateQName	(const xmlChar *value : itype(_Ptr<const xmlChar>), int space);
XMLPUBFUN int XMLCALL
		xmlValidateName		(const xmlChar *value : itype(_Ptr<const xmlChar>), int space);
XMLPUBFUN int XMLCALL
		xmlValidateNMToken	(const xmlChar *value : itype(_Ptr<const xmlChar>), int space);
#endif

XMLPUBFUN xmlChar *xmlBuildQName(const xmlChar *ncname : itype(_Ptr<const xmlChar>), const xmlChar *prefix : itype(_Ptr<const xmlChar>), xmlChar *memory : itype(_Ptr<xmlChar>), int len) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlSplitQName2(const xmlChar *name : itype(_Ptr<const xmlChar>), xmlChar **prefix : itype(_Ptr<_Ptr<xmlChar>>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN const xmlChar *xmlSplitQName3(const xmlChar *name : itype(_Ptr<const xmlChar>), int *len : itype(_Ptr<int>)) : itype(_Ptr<const xmlChar>);

/*
 * Handling Buffers, the old ones see @xmlBuf for the new ones.
 */

XMLPUBFUN void XMLCALL
		xmlSetBufferAllocationScheme(xmlBufferAllocationScheme scheme);
XMLPUBFUN xmlBufferAllocationScheme XMLCALL
		xmlGetBufferAllocationScheme(void);

XMLPUBFUN xmlBufferPtr xmlBufferCreate(void) : itype(_Ptr<xmlBuffer>);
XMLPUBFUN xmlBufferPtr xmlBufferCreateSize(size_t size) : itype(_Ptr<xmlBuffer>);
XMLPUBFUN xmlBufferPtr xmlBufferCreateStatic(void *mem, size_t size) : itype(_Ptr<xmlBuffer>);
XMLPUBFUN int XMLCALL
		xmlBufferResize		(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), unsigned int size);
XMLPUBFUN void XMLCALL
		xmlBufferFree		(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>));
XMLPUBFUN int XMLCALL
		xmlBufferDump		(FILE *file : itype(_Ptr<FILE>), xmlBufferPtr buf : itype(_Ptr<xmlBuffer>));
XMLPUBFUN int XMLCALL
		xmlBufferAdd		(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), const xmlChar *str : itype(_Ptr<const xmlChar>), int len);
XMLPUBFUN int XMLCALL
		xmlBufferAddHead	(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), const xmlChar *str : itype(_Ptr<const xmlChar>), int len);
XMLPUBFUN int XMLCALL
		xmlBufferCat		(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), const xmlChar *str : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
		xmlBufferCCat		(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), const char *str : itype(_Ptr<const char>));
XMLPUBFUN int XMLCALL
		xmlBufferShrink		(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), unsigned int len);
XMLPUBFUN int XMLCALL
		xmlBufferGrow		(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), unsigned int len);
XMLPUBFUN void XMLCALL
		xmlBufferEmpty		(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>));
XMLPUBFUN const xmlChar *xmlBufferContent(const xmlBuffer *buf : itype(_Ptr<const xmlBuffer>)) : itype(_Ptr<const xmlChar>);
XMLPUBFUN xmlChar *xmlBufferDetach(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN void XMLCALL
		xmlBufferSetAllocationScheme(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), xmlBufferAllocationScheme scheme);
XMLPUBFUN int XMLCALL
		xmlBufferLength		(const xmlBuffer *buf : itype(_Ptr<const xmlBuffer>));

/*
 * Creating/freeing new structures.
 */
XMLPUBFUN xmlDtdPtr xmlCreateIntSubset(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *ExternalID : itype(_Ptr<const xmlChar>), const xmlChar *SystemID : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlDtd>);
XMLPUBFUN xmlDtdPtr xmlNewDtd(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *ExternalID : itype(_Ptr<const xmlChar>), const xmlChar *SystemID : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlDtd>);
XMLPUBFUN xmlDtdPtr xmlGetIntSubset(const xmlDoc *doc : itype(_Ptr<const xmlDoc>)) : itype(_Ptr<xmlDtd>);
XMLPUBFUN void XMLCALL
		xmlFreeDtd		(xmlDtdPtr cur : itype(_Ptr<xmlDtd>));
#ifdef LIBXML_LEGACY_ENABLED
XMLPUBFUN xmlNsPtr xmlNewGlobalNs(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *href : itype(_Ptr<const xmlChar>), const xmlChar *prefix : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNs>);
#endif /* LIBXML_LEGACY_ENABLED */
XMLPUBFUN xmlNsPtr xmlNewNs(xmlNodePtr node : itype(_Ptr<xmlNode>), const xmlChar *href : itype(_Ptr<const xmlChar>), const xmlChar *prefix : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNs>);
XMLPUBFUN void XMLCALL
		xmlFreeNs		(xmlNsPtr cur : itype(_Ptr<xmlNs>));
XMLPUBFUN void XMLCALL
		xmlFreeNsList		(xmlNsPtr cur : itype(_Ptr<xmlNs>));
XMLPUBFUN xmlDocPtr xmlNewDoc(const xmlChar *version : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlDoc>);
XMLPUBFUN void XMLCALL
		xmlFreeDoc		(xmlDocPtr cur : itype(_Ptr<xmlDoc>));
XMLPUBFUN xmlAttrPtr xmlNewDocProp(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *value : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlAttr>);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_HTML_ENABLED) || \
    defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN xmlAttrPtr xmlNewProp(xmlNodePtr node : itype(_Ptr<xmlNode>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *value : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlAttr>);
#endif
XMLPUBFUN xmlAttrPtr xmlNewNsProp(xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNsPtr ns : itype(_Ptr<xmlNs>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *value : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlAttr>);
XMLPUBFUN xmlAttrPtr xmlNewNsPropEatName(xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNsPtr ns : itype(_Ptr<xmlNs>), xmlChar *name : itype(_Ptr<xmlChar>), const xmlChar *value : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlAttr>);
XMLPUBFUN void XMLCALL
		xmlFreePropList		(xmlAttrPtr cur : itype(_Ptr<xmlAttr>));
XMLPUBFUN void XMLCALL
		xmlFreeProp		(xmlAttrPtr cur : itype(_Ptr<xmlAttr>));
XMLPUBFUN xmlAttrPtr xmlCopyProp(xmlNodePtr target : itype(_Ptr<xmlNode>), xmlAttrPtr cur : itype(_Ptr<xmlAttr>)) : itype(_Ptr<xmlAttr>);
XMLPUBFUN xmlAttrPtr xmlCopyPropList(xmlNodePtr target : itype(_Ptr<xmlNode>), xmlAttrPtr cur : itype(_Ptr<xmlAttr>)) : itype(_Ptr<xmlAttr>);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN xmlDtdPtr xmlCopyDtd(xmlDtdPtr dtd : itype(_Ptr<xmlDtd>)) : itype(_Ptr<xmlDtd>);
#endif /* LIBXML_TREE_ENABLED */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN xmlDocPtr xmlCopyDoc(xmlDocPtr doc : itype(_Ptr<xmlDoc>), int recursive) : itype(_Ptr<xmlDoc>);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
/*
 * Creating new nodes.
 */
XMLPUBFUN xmlNodePtr xmlNewDocNode(xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNsPtr ns : itype(_Ptr<xmlNs>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *content : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlNewDocNodeEatName(xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNsPtr ns : itype(_Ptr<xmlNs>), xmlChar *name : itype(_Ptr<xmlChar>), const xmlChar *content : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlNewNode(xmlNsPtr ns : itype(_Ptr<xmlNs>), const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlNewNodeEatName(xmlNsPtr ns : itype(_Ptr<xmlNs>), xmlChar *name : itype(_Ptr<xmlChar>)) : itype(_Ptr<xmlNode>);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN xmlNodePtr xmlNewChild(xmlNodePtr parent : itype(_Ptr<xmlNode>), xmlNsPtr ns : itype(_Ptr<xmlNs>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *content : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);
#endif
XMLPUBFUN xmlNodePtr xmlNewDocText(const xmlDoc *doc : itype(_Ptr<const xmlDoc>), const xmlChar *content : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlNewText(const xmlChar *content : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlNewDocPI(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *content : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlNewPI(const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *content : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlNewDocTextLen(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *content : itype(_Ptr<const xmlChar>), int len) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlNewTextLen(const xmlChar *content : itype(_Ptr<const xmlChar>), int len) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlNewDocComment(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *content : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlNewComment(const xmlChar *content : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlNewCDataBlock(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *content : itype(_Ptr<const xmlChar>), int len) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlNewCharRef(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlNewReference(const xmlDoc *doc : itype(_Ptr<const xmlDoc>), const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlCopyNode(xmlNodePtr node : itype(_Ptr<xmlNode>), int recursive) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlDocCopyNode(xmlNodePtr node : itype(_Ptr<xmlNode>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), int recursive) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlDocCopyNodeList(xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr node : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlCopyNodeList(xmlNodePtr node : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNode>);
#ifdef LIBXML_TREE_ENABLED

XMLPUBFUN xmlNodePtr xmlNewTextChild(xmlNodePtr parent : itype(_Ptr<xmlNode>), 
    xmlNsPtr ns : itype(_Ptr<xmlNs>), 
    const xmlChar *name : itype(_Nt_array_ptr<const xmlChar>), 
    const xmlChar *content : itype(_Nt_array_ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);


XMLPUBFUN xmlNodePtr xmlNewDocRawNode(xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNsPtr ns : itype(_Ptr<xmlNs>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *content : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlNewDocFragment(xmlDocPtr doc : itype(_Ptr<xmlDoc>)) : itype(_Ptr<xmlNode>);
#endif /* LIBXML_TREE_ENABLED */

/*
 * Navigating.
 */
XMLPUBFUN long XMLCALL
		xmlGetLineNo		(const xmlNode *node : itype(_Ptr<const xmlNode>));
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_DEBUG_ENABLED)
XMLPUBFUN xmlChar *xmlGetNodePath(const xmlNode *node : itype(_Ptr<const xmlNode>)) : itype(_Ptr<xmlChar>);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_DEBUG_ENABLED) */
XMLPUBFUN xmlNodePtr xmlDocGetRootElement(const xmlDoc *doc : itype(_Ptr<const xmlDoc>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlGetLastChild(const xmlNode *parent : itype(_Ptr<const xmlNode>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN int XMLCALL
		xmlNodeIsText		(const xmlNode *node : itype(_Ptr<const xmlNode>));
XMLPUBFUN int XMLCALL
		xmlIsBlankNode		(const xmlNode *node : itype(_Ptr<const xmlNode>));

/*
 * Changing the structure.
 */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED)
XMLPUBFUN xmlNodePtr xmlDocSetRootElement(xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr root : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNode>);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED) */
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN void XMLCALL
		xmlNodeSetName		(xmlNodePtr cur : itype(_Ptr<xmlNode>), const xmlChar *name : itype(_Ptr<const xmlChar>));
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN xmlNodePtr xmlAddChild(xmlNodePtr parent : itype(_Ptr<xmlNode>), xmlNodePtr cur : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlAddChildList(xmlNodePtr parent : itype(_Ptr<xmlNode>), xmlNodePtr cur : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNode>);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED)
XMLPUBFUN xmlNodePtr xmlReplaceNode(xmlNodePtr old : itype(_Ptr<xmlNode>), xmlNodePtr cur : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNode>);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED) */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_HTML_ENABLED) || \
    defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED)
XMLPUBFUN xmlNodePtr xmlAddPrevSibling(xmlNodePtr cur : itype(_Ptr<xmlNode>), xmlNodePtr elem : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNode>);
#endif /* LIBXML_TREE_ENABLED || LIBXML_HTML_ENABLED || LIBXML_SCHEMAS_ENABLED */
XMLPUBFUN xmlNodePtr xmlAddSibling(xmlNodePtr cur : itype(_Ptr<xmlNode>), xmlNodePtr elem : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlAddNextSibling(xmlNodePtr cur : itype(_Ptr<xmlNode>), xmlNodePtr elem : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN void XMLCALL
		xmlUnlinkNode		(xmlNodePtr cur : itype(_Ptr<xmlNode>));
XMLPUBFUN xmlNodePtr xmlTextMerge(xmlNodePtr first : itype(_Ptr<xmlNode>), xmlNodePtr second : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN int XMLCALL
		xmlTextConcat		(xmlNodePtr node : itype(_Ptr<xmlNode>), const xmlChar *content : itype(_Ptr<const xmlChar>), int len);
XMLPUBFUN void XMLCALL
		xmlFreeNodeList		(xmlNodePtr cur : itype(_Ptr<xmlNode>));
XMLPUBFUN void XMLCALL
		xmlFreeNode		(xmlNodePtr cur : itype(_Ptr<xmlNode>));
XMLPUBFUN void XMLCALL
		xmlSetTreeDoc		(xmlNodePtr tree : itype(_Ptr<xmlNode>), xmlDocPtr doc : itype(_Ptr<xmlDoc>));
XMLPUBFUN void XMLCALL
		xmlSetListDoc		(xmlNodePtr list : itype(_Ptr<xmlNode>), xmlDocPtr doc : itype(_Ptr<xmlDoc>));
/*
 * Namespaces.
 */
XMLPUBFUN xmlNsPtr xmlSearchNs(xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr node : itype(_Ptr<xmlNode>), const xmlChar *nameSpace : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNs>);
XMLPUBFUN xmlNsPtr xmlSearchNsByHref(xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr node : itype(_Ptr<xmlNode>), const xmlChar *href : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNs>);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XPATH_ENABLED) || \
    defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN xmlNsPtr *xmlGetNsList(const xmlDoc *doc : itype(_Ptr<const xmlDoc>), const xmlNode *node : itype(_Ptr<const xmlNode>)) : itype(_Ptr<xmlNsPtr>);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XPATH_ENABLED) */

XMLPUBFUN void XMLCALL
		xmlSetNs		(xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNsPtr ns : itype(_Ptr<xmlNs>));
XMLPUBFUN xmlNsPtr xmlCopyNamespace(xmlNsPtr cur : itype(_Ptr<xmlNs>)) : itype(_Ptr<xmlNs>);
XMLPUBFUN xmlNsPtr xmlCopyNamespaceList(xmlNsPtr cur : itype(_Ptr<xmlNs>)) : itype(_Ptr<xmlNs>);

/*
 * Changing the content.
 */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED) || \
    defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_HTML_ENABLED)
XMLPUBFUN xmlAttrPtr xmlSetProp(xmlNodePtr node : itype(_Ptr<xmlNode>), 
    const xmlChar *name : itype(_Nt_array_ptr<const xmlChar>), 
    const xmlChar *value : itype(_Nt_array_ptr<const xmlChar>)) : itype(_Ptr<xmlAttr>);
XMLPUBFUN xmlAttrPtr xmlSetNsProp(xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNsPtr ns : itype(_Ptr<xmlNs>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *value : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlAttr>);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED) || \
	  defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_HTML_ENABLED) */
XMLPUBFUN xmlChar *xmlGetNoNsProp(const xmlNode *node : itype(_Ptr<const xmlNode>), const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlGetProp(const xmlNode *node : itype(_Ptr<const xmlNode>), const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Nt_array_ptr<xmlChar>);
XMLPUBFUN xmlAttrPtr xmlHasProp(const xmlNode *node : itype(_Ptr<const xmlNode>), const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlAttr>);
XMLPUBFUN xmlAttrPtr xmlHasNsProp(const xmlNode *node : itype(_Ptr<const xmlNode>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *nameSpace : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlAttr>);
XMLPUBFUN xmlChar *xmlGetNsProp(const xmlNode *node : itype(_Ptr<const xmlNode>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *nameSpace : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlNodePtr xmlStringGetNodeList(const xmlDoc *doc : itype(_Ptr<const xmlDoc>), const xmlChar *value : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlStringLenGetNodeList(const xmlDoc *doc : itype(_Ptr<const xmlDoc>), const xmlChar *value : itype(_Ptr<const xmlChar>), int len) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlChar *xmlNodeListGetString(xmlDocPtr doc : itype(_Ptr<xmlDoc>), const xmlNode *list : itype(_Ptr<const xmlNode>), int inLine) : itype(_Nt_array_ptr<xmlChar>);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN xmlChar *xmlNodeListGetRawString(const xmlDoc *doc : itype(_Ptr<const xmlDoc>), const xmlNode *list : itype(_Ptr<const xmlNode>), int inLine) : itype(_Ptr<xmlChar>);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN void XMLCALL
		xmlNodeSetContent	(xmlNodePtr cur : itype(_Ptr<xmlNode>), const xmlChar *content : itype(_Ptr<const xmlChar>));
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN void XMLCALL
		xmlNodeSetContentLen	(xmlNodePtr cur : itype(_Ptr<xmlNode>), const xmlChar *content : itype(_Ptr<const xmlChar>), int len);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN void XMLCALL
		xmlNodeAddContent	(xmlNodePtr cur : itype(_Ptr<xmlNode>), const xmlChar *content : itype(_Ptr<const xmlChar>));
XMLPUBFUN void XMLCALL
		xmlNodeAddContentLen	(xmlNodePtr cur : itype(_Ptr<xmlNode>), const xmlChar *content : itype(_Ptr<const xmlChar>), int len);
XMLPUBFUN xmlChar *xmlNodeGetContent(const xmlNode *cur : itype(_Ptr<const xmlNode>)) : itype(_Ptr<xmlChar>);

XMLPUBFUN int XMLCALL
		xmlNodeBufGetContent	(xmlBufferPtr buffer : itype(_Ptr<xmlBuffer>), const xmlNode *cur : itype(_Ptr<const xmlNode>));
XMLPUBFUN int XMLCALL
		xmlBufGetNodeContent	(xmlBufPtr buf : itype(_Ptr<xmlBuf>), const xmlNode *cur : itype(_Ptr<const xmlNode>));

XMLPUBFUN xmlChar *xmlNodeGetLang(const xmlNode *cur : itype(_Ptr<const xmlNode>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN int XMLCALL
		xmlNodeGetSpacePreserve	(const xmlNode *cur : itype(_Ptr<const xmlNode>));
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN void XMLCALL
		xmlNodeSetLang		(xmlNodePtr cur : itype(_Ptr<xmlNode>), const xmlChar *lang : itype(_Ptr<const xmlChar>));
XMLPUBFUN void XMLCALL
		xmlNodeSetSpacePreserve (xmlNodePtr cur : itype(_Ptr<xmlNode>), int val);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN xmlChar *xmlNodeGetBase(const xmlDoc *doc : itype(_Ptr<const xmlDoc>), const xmlNode *cur : itype(_Ptr<const xmlNode>)) : itype(_Ptr<xmlChar>);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED)
XMLPUBFUN void XMLCALL
		xmlNodeSetBase		(xmlNodePtr cur : itype(_Ptr<xmlNode>), const xmlChar *uri : itype(_Ptr<const xmlChar>));
#endif

/*
 * Removing content.
 */
XMLPUBFUN int XMLCALL
		xmlRemoveProp		(xmlAttrPtr cur : itype(_Ptr<xmlAttr>));
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN int XMLCALL
		xmlUnsetNsProp		(xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNsPtr ns : itype(_Ptr<xmlNs>), const xmlChar *name : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
		xmlUnsetProp		(xmlNodePtr node : itype(_Ptr<xmlNode>), const xmlChar *name : itype(_Ptr<const xmlChar>));
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */

/*
 * Internal, don't use.
 */
XMLPUBFUN void XMLCALL
		xmlBufferWriteCHAR	(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), const xmlChar *string : itype(_Ptr<const xmlChar>));
XMLPUBFUN void XMLCALL
		xmlBufferWriteChar	(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), const char *string : itype(_Ptr<const char>));
XMLPUBFUN void XMLCALL
		xmlBufferWriteQuotedString(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), const xmlChar *string : itype(_Ptr<const xmlChar>));

#ifdef LIBXML_OUTPUT_ENABLED
XMLPUBFUN void xmlAttrSerializeTxtContent(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlAttrPtr attr : itype(_Ptr<xmlAttr>), const xmlChar *string : itype(_Ptr<const xmlChar>));
#endif /* LIBXML_OUTPUT_ENABLED */

#ifdef LIBXML_TREE_ENABLED
/*
 * Namespace handling.
 */
XMLPUBFUN int XMLCALL
		xmlReconciliateNs	(xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr tree : itype(_Ptr<xmlNode>));
#endif

#ifdef LIBXML_OUTPUT_ENABLED
/*
 * Saving.
 */
XMLPUBFUN void XMLCALL
		xmlDocDumpFormatMemory	(xmlDocPtr cur : itype(_Ptr<xmlDoc>), xmlChar **mem : itype(_Ptr<_Ptr<xmlChar>>), int *size : itype(_Ptr<int>), int format);
XMLPUBFUN void XMLCALL
		xmlDocDumpMemory	(xmlDocPtr cur : itype(_Ptr<xmlDoc>), xmlChar **mem : itype(_Ptr<_Nt_array_ptr<xmlChar>>), int *size : itype(_Ptr<int>));
XMLPUBFUN void XMLCALL
		xmlDocDumpMemoryEnc	(xmlDocPtr out_doc : itype(_Ptr<xmlDoc>), xmlChar **doc_txt_ptr : itype(_Ptr<_Ptr<xmlChar>>), int *doc_txt_len : itype(_Ptr<int>), const char *txt_encoding : itype(_Ptr<const char>));
XMLPUBFUN void XMLCALL
		xmlDocDumpFormatMemoryEnc(xmlDocPtr out_doc : itype(_Ptr<xmlDoc>), xmlChar **doc_txt_ptr : itype(_Ptr<_Ptr<xmlChar>>), int *doc_txt_len : itype(_Ptr<int>), const char *txt_encoding : itype(_Ptr<const char>), int format);
XMLPUBFUN int XMLCALL
		xmlDocFormatDump	(FILE *f : itype(_Ptr<FILE>), xmlDocPtr cur : itype(_Ptr<xmlDoc>), int format);
XMLPUBFUN int XMLCALL
		xmlDocDump		(FILE *f : itype(_Ptr<FILE>), xmlDocPtr cur : itype(_Ptr<xmlDoc>));
XMLPUBFUN void XMLCALL
		xmlElemDump		(FILE *f : itype(_Ptr<FILE>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr cur : itype(_Ptr<xmlNode>));
XMLPUBFUN int XMLCALL
		xmlSaveFile		(const char *filename : itype(_Ptr<const char>), xmlDocPtr cur : itype(_Ptr<xmlDoc>));
XMLPUBFUN int XMLCALL
		xmlSaveFormatFile	(const char *filename : itype(_Ptr<const char>), xmlDocPtr cur : itype(_Ptr<xmlDoc>), int format);
XMLPUBFUN size_t XMLCALL
		xmlBufNodeDump		(xmlBufPtr buf : itype(_Ptr<xmlBuf>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr cur : itype(_Ptr<xmlNode>), int level, int format);
XMLPUBFUN int XMLCALL
		xmlNodeDump		(xmlBufferPtr buf : itype(_Ptr<xmlBuffer>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr cur : itype(_Ptr<xmlNode>), int level, int format);

XMLPUBFUN int XMLCALL
		xmlSaveFileTo		(xmlOutputBufferPtr buf : itype(_Ptr<xmlOutputBuffer>), xmlDocPtr cur : itype(_Ptr<xmlDoc>), const char *encoding : itype(_Ptr<const char>));
XMLPUBFUN int XMLCALL
		xmlSaveFormatFileTo     (xmlOutputBufferPtr buf : itype(_Ptr<xmlOutputBuffer>), xmlDocPtr cur : itype(_Ptr<xmlDoc>), const char *encoding : itype(_Ptr<const char>), int format);
XMLPUBFUN void XMLCALL
		xmlNodeDumpOutput	(xmlOutputBufferPtr buf : itype(_Ptr<xmlOutputBuffer>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr cur : itype(_Ptr<xmlNode>), int level, int format, const char *encoding : itype(_Ptr<const char>));

XMLPUBFUN int XMLCALL
		xmlSaveFormatFileEnc    (const char *filename : itype(_Ptr<const char>), xmlDocPtr cur : itype(_Ptr<xmlDoc>), const char *encoding : itype(_Ptr<const char>), int format);

XMLPUBFUN int XMLCALL
		xmlSaveFileEnc		(const char *filename : itype(_Ptr<const char>), xmlDocPtr cur : itype(_Ptr<xmlDoc>), const char *encoding : itype(_Ptr<const char>));

#endif /* LIBXML_OUTPUT_ENABLED */
/*
 * XHTML
 */
XMLPUBFUN int XMLCALL
		xmlIsXHTML		(const xmlChar *systemID : itype(_Ptr<const xmlChar>), const xmlChar *publicID : itype(_Ptr<const xmlChar>));

/*
 * Compression.
 */
XMLPUBFUN int XMLCALL
		xmlGetDocCompressMode	(const xmlDoc *doc : itype(_Ptr<const xmlDoc>));
XMLPUBFUN void XMLCALL
		xmlSetDocCompressMode	(xmlDocPtr doc : itype(_Ptr<xmlDoc>), int mode);
XMLPUBFUN int XMLCALL
		xmlGetCompressMode	(void);
XMLPUBFUN void XMLCALL
		xmlSetCompressMode	(int mode);

/*
* DOM-wrapper helper functions.
*/
XMLPUBFUN xmlDOMWrapCtxtPtr xmlDOMWrapNewCtxt(void) : itype(_Ptr<xmlDOMWrapCtxt>);
XMLPUBFUN void XMLCALL
		xmlDOMWrapFreeCtxt	(xmlDOMWrapCtxtPtr ctxt : itype(_Ptr<xmlDOMWrapCtxt>));
XMLPUBFUN int XMLCALL
	    xmlDOMWrapReconcileNamespaces(xmlDOMWrapCtxtPtr ctxt : itype(_Ptr<xmlDOMWrapCtxt>), xmlNodePtr elem : itype(_Ptr<xmlNode>), int options);
XMLPUBFUN int XMLCALL
	    xmlDOMWrapAdoptNode		(xmlDOMWrapCtxtPtr ctxt : itype(_Ptr<xmlDOMWrapCtxt>), xmlDocPtr sourceDoc : itype(_Ptr<xmlDoc>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlDocPtr destDoc : itype(_Ptr<xmlDoc>), xmlNodePtr destParent : itype(_Ptr<xmlNode>), int options);
XMLPUBFUN int XMLCALL
	    xmlDOMWrapRemoveNode	(xmlDOMWrapCtxtPtr ctxt : itype(_Ptr<xmlDOMWrapCtxt>), xmlDocPtr doc : itype(_Ptr<xmlDoc>), xmlNodePtr node : itype(_Ptr<xmlNode>), int options);
XMLPUBFUN int XMLCALL
	    xmlDOMWrapCloneNode		(xmlDOMWrapCtxtPtr ctxt : itype(_Ptr<xmlDOMWrapCtxt>), xmlDocPtr sourceDoc : itype(_Ptr<xmlDoc>), xmlNodePtr node : itype(_Ptr<xmlNode>), xmlNodePtr *clonedNode : itype(_Ptr<xmlNodePtr>), xmlDocPtr destDoc : itype(_Ptr<xmlDoc>), xmlNodePtr destParent : itype(_Ptr<xmlNode>), int deep, int options);

#ifdef LIBXML_TREE_ENABLED
/*
 * 5 interfaces from DOM ElementTraversal, but different in entities
 * traversal.
 */
XMLPUBFUN unsigned long XMLCALL
            xmlChildElementCount        (xmlNodePtr parent : itype(_Ptr<xmlNode>));
XMLPUBFUN xmlNodePtr xmlNextElementSibling(xmlNodePtr node : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlFirstElementChild(xmlNodePtr parent : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlLastElementChild(xmlNodePtr parent : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNode>);
XMLPUBFUN xmlNodePtr xmlPreviousElementSibling(xmlNodePtr node : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNode>);
#endif
#ifdef __cplusplus
}
#endif
#ifndef __XML_PARSER_H__
#include <libxml/xmlmemory.h>
#endif

#endif /* __XML_TREE_H__ */

