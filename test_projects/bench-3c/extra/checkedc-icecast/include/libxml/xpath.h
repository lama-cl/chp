/*
 * Summary: XML Path Language implementation
 * Description: API for the XML Path Language implementation
 *
 * XML Path Language implementation
 * XPath is a language for addressing parts of an XML document,
 * designed to be used by both XSLT and XPointer
 *     http://www.w3.org/TR/xpath
 *
 * Implements
 * W3C Recommendation 16 November 1999
 *     http://www.w3.org/TR/1999/REC-xpath-19991116
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_XPATH_H__
#define __XML_XPATH_H__

#include <libxml/xmlversion.h>

#ifdef LIBXML_XPATH_ENABLED

#include <libxml/xmlerror.h>
#include <libxml/tree.h>
#include <libxml/hash.h>
#endif /* LIBXML_XPATH_ENABLED */

#if defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
#ifdef __cplusplus
extern "C" {
#endif
#endif /* LIBXML_XPATH_ENABLED or LIBXML_SCHEMAS_ENABLED */

#ifdef LIBXML_XPATH_ENABLED

typedef struct _xmlXPathContext xmlXPathContext;
typedef xmlXPathContext *xmlXPathContextPtr;
typedef struct _xmlXPathParserContext xmlXPathParserContext;
typedef xmlXPathParserContext *xmlXPathParserContextPtr;

/**
 * The set of XPath error codes.
 */

typedef enum {
    XPATH_EXPRESSION_OK = 0,
    XPATH_NUMBER_ERROR,
    XPATH_UNFINISHED_LITERAL_ERROR,
    XPATH_START_LITERAL_ERROR,
    XPATH_VARIABLE_REF_ERROR,
    XPATH_UNDEF_VARIABLE_ERROR,
    XPATH_INVALID_PREDICATE_ERROR,
    XPATH_EXPR_ERROR,
    XPATH_UNCLOSED_ERROR,
    XPATH_UNKNOWN_FUNC_ERROR,
    XPATH_INVALID_OPERAND,
    XPATH_INVALID_TYPE,
    XPATH_INVALID_ARITY,
    XPATH_INVALID_CTXT_SIZE,
    XPATH_INVALID_CTXT_POSITION,
    XPATH_MEMORY_ERROR,
    XPTR_SYNTAX_ERROR,
    XPTR_RESOURCE_ERROR,
    XPTR_SUB_RESOURCE_ERROR,
    XPATH_UNDEF_PREFIX_ERROR,
    XPATH_ENCODING_ERROR,
    XPATH_INVALID_CHAR_ERROR,
    XPATH_INVALID_CTXT,
    XPATH_STACK_ERROR,
    XPATH_FORBID_VARIABLE_ERROR,
    XPATH_OP_LIMIT_EXCEEDED,
    XPATH_RECURSION_LIMIT_EXCEEDED
} xmlXPathError;

/*
 * A node-set (an unordered collection of nodes without duplicates).
 */
typedef struct _xmlNodeSet xmlNodeSet;
typedef xmlNodeSet *xmlNodeSetPtr;
struct _xmlNodeSet {
    int nodeNr;			/* number of nodes in the set */
    int nodeMax;		/* size of the array as allocated */
    xmlNodePtr *nodeTab : itype(_Ptr<xmlNodePtr>);	/* array of nodes in no particular order */
    /* @@ with_ns to check whether namespace nodes should be looked at @@ */
};

/*
 * An expression is evaluated to yield an object, which
 * has one of the following four basic types:
 *   - node-set
 *   - boolean
 *   - number
 *   - string
 *
 * @@ XPointer will add more types !
 */

typedef enum {
    XPATH_UNDEFINED = 0,
    XPATH_NODESET = 1,
    XPATH_BOOLEAN = 2,
    XPATH_NUMBER = 3,
    XPATH_STRING = 4,
    XPATH_POINT = 5,
    XPATH_RANGE = 6,
    XPATH_LOCATIONSET = 7,
    XPATH_USERS = 8,
    XPATH_XSLT_TREE = 9  /* An XSLT value tree, non modifiable */
} xmlXPathObjectType;

typedef struct _xmlXPathObject xmlXPathObject;
typedef xmlXPathObject *xmlXPathObjectPtr;
struct _xmlXPathObject {
    xmlXPathObjectType type;
    xmlNodeSetPtr nodesetval : itype(_Ptr<xmlNodeSet>);
    int boolval;
    double floatval;
    xmlChar *stringval : itype(_Ptr<xmlChar>);
    void *user;
    int index;
    void *user2;
    int index2;
};

/**
 * xmlXPathConvertFunc:
 * @obj:  an XPath object
 * @type:  the number of the target type
 *
 * A conversion function is associated to a type and used to cast
 * the new type to primitive values.
 *
 * Returns -1 in case of error, 0 otherwise
 */
typedef int (*xmlXPathConvertFunc) (xmlXPathObjectPtr obj, int type);

/*
 * Extra type: a name and a conversion function.
 */

typedef struct _xmlXPathType xmlXPathType;
typedef xmlXPathType *xmlXPathTypePtr;
struct _xmlXPathType {
    const xmlChar *name : itype(_Ptr<const xmlChar>);		/* the type name */
    xmlXPathConvertFunc func : itype(_Ptr<int (_Ptr<xmlXPathObject>, int)>);		/* the conversion function */
};

/*
 * Extra variable: a name and a value.
 */

typedef struct _xmlXPathVariable xmlXPathVariable;
typedef xmlXPathVariable *xmlXPathVariablePtr;
struct _xmlXPathVariable {
    const xmlChar *name : itype(_Ptr<const xmlChar>);		/* the variable name */
    xmlXPathObjectPtr value : itype(_Ptr<xmlXPathObject>);		/* the value */
};

/**
 * xmlXPathEvalFunc:
 * @ctxt: an XPath parser context
 * @nargs: the number of arguments passed to the function
 *
 * An XPath evaluation function, the parameters are on the XPath context stack.
 */

typedef void (*xmlXPathEvalFunc)(xmlXPathParserContextPtr ctxt,
	                         int nargs);

/*
 * Extra function: a name and a evaluation function.
 */

typedef struct _xmlXPathFunct xmlXPathFunct;
typedef xmlXPathFunct *xmlXPathFuncPtr;
struct _xmlXPathFunct {
    const xmlChar *name : itype(_Ptr<const xmlChar>);		/* the function name */
    xmlXPathEvalFunc func : itype(_Ptr<void (_Ptr<xmlXPathParserContext>, int)>);		/* the evaluation function */
};

/**
 * xmlXPathAxisFunc:
 * @ctxt:  the XPath interpreter context
 * @cur:  the previous node being explored on that axis
 *
 * An axis traversal function. To traverse an axis, the engine calls
 * the first time with cur == NULL and repeat until the function returns
 * NULL indicating the end of the axis traversal.
 *
 * Returns the next node in that axis or NULL if at the end of the axis.
 */

typedef xmlXPathObjectPtr (*xmlXPathAxisFunc) (xmlXPathParserContextPtr ctxt,
				 xmlXPathObjectPtr cur);

/*
 * Extra axis: a name and an axis function.
 */

typedef struct _xmlXPathAxis xmlXPathAxis;
typedef xmlXPathAxis *xmlXPathAxisPtr;
struct _xmlXPathAxis {
    const xmlChar *name : itype(_Ptr<const xmlChar>);		/* the axis name */
    xmlXPathAxisFunc func : itype(_Ptr<_Ptr<xmlXPathObject> (_Ptr<xmlXPathParserContext>, _Ptr<xmlXPathObject>)>);		/* the search function */
};

/**
 * xmlXPathFunction:
 * @ctxt:  the XPath interprestation context
 * @nargs:  the number of arguments
 *
 * An XPath function.
 * The arguments (if any) are popped out from the context stack
 * and the result is pushed on the stack.
 */

typedef void (*xmlXPathFunction) (xmlXPathParserContextPtr ctxt, int nargs);

/*
 * Function and Variable Lookup.
 */

/**
 * xmlXPathVariableLookupFunc:
 * @ctxt:  an XPath context
 * @name:  name of the variable
 * @ns_uri:  the namespace name hosting this variable
 *
 * Prototype for callbacks used to plug variable lookup in the XPath
 * engine.
 *
 * Returns the XPath object value or NULL if not found.
 */
typedef xmlXPathObjectPtr (*xmlXPathVariableLookupFunc) (void *ctxt,
                                         const xmlChar *name,
                                         const xmlChar *ns_uri);

/**
 * xmlXPathFuncLookupFunc:
 * @ctxt:  an XPath context
 * @name:  name of the function
 * @ns_uri:  the namespace name hosting this function
 *
 * Prototype for callbacks used to plug function lookup in the XPath
 * engine.
 *
 * Returns the XPath function or NULL if not found.
 */
typedef xmlXPathFunction (*xmlXPathFuncLookupFunc) (void *ctxt,
					 const xmlChar *name,
					 const xmlChar *ns_uri);

/**
 * xmlXPathFlags:
 * Flags for XPath engine compilation and runtime
 */
/**
 * XML_XPATH_CHECKNS:
 *
 * check namespaces at compilation
 */
#define XML_XPATH_CHECKNS (1<<0)
/**
 * XML_XPATH_NOVAR:
 *
 * forbid variables in expression
 */
#define XML_XPATH_NOVAR	  (1<<1)

/**
 * xmlXPathContext:
 *
 * Expression evaluation occurs with respect to a context.
 * he context consists of:
 *    - a node (the context node)
 *    - a node list (the context node list)
 *    - a set of variable bindings
 *    - a function library
 *    - the set of namespace declarations in scope for the expression
 * Following the switch to hash tables, this need to be trimmed up at
 * the next binary incompatible release.
 * The node may be modified when the context is passed to libxml2
 * for an XPath evaluation so you may need to initialize it again
 * before the next call.
 */

struct _xmlXPathContext {
    xmlDocPtr doc : itype(_Ptr<xmlDoc>);			/* The current document */
    xmlNodePtr node : itype(_Ptr<xmlNode>);			/* The current node */

    int nb_variables_unused;		/* unused (hash table) */
    int max_variables_unused;		/* unused (hash table) */
    xmlHashTablePtr varHash : itype(_Ptr<xmlHashTable>);		/* Hash table of defined variables */

    int nb_types;			/* number of defined types */
    int max_types;			/* max number of types */
    xmlXPathTypePtr types : itype(_Ptr<xmlXPathType>);		/* Array of defined types */

    int nb_funcs_unused;		/* unused (hash table) */
    int max_funcs_unused;		/* unused (hash table) */
    xmlHashTablePtr funcHash : itype(_Ptr<xmlHashTable>);		/* Hash table of defined funcs */

    int nb_axis;			/* number of defined axis */
    int max_axis;			/* max number of axis */
    xmlXPathAxisPtr axis : itype(_Ptr<xmlXPathAxis>);		/* Array of defined axis */

    /* the namespace nodes of the context node */
    xmlNsPtr *namespaces : itype(_Ptr<xmlNsPtr>);		/* Array of namespaces */
    int nsNr;				/* number of namespace in scope */
    void *user;				/* function to free */

    /* extra variables */
    int contextSize;			/* the context size */
    int proximityPosition;		/* the proximity position */

    /* extra stuff for XPointer */
    int xptr;				/* is this an XPointer context? */
    xmlNodePtr here : itype(_Ptr<xmlNode>);			/* for here() */
    xmlNodePtr origin : itype(_Ptr<xmlNode>);			/* for origin() */

    /* the set of namespace declarations in scope for the expression */
    xmlHashTablePtr nsHash : itype(_Ptr<xmlHashTable>);		/* The namespaces hash table */
    xmlXPathVariableLookupFunc varLookupFunc : itype(_Ptr<_Ptr<xmlXPathObject> (void *, _Ptr<const xmlChar>, _Ptr<const xmlChar>)>);/* variable lookup func */
    void *varLookupData;		/* variable lookup data */

    /* Possibility to link in an extra item */
    void *extra;                        /* needed for XSLT */

    /* The function name and URI when calling a function */
    const xmlChar *function : itype(_Ptr<const xmlChar>);
    const xmlChar *functionURI : itype(_Ptr<const xmlChar>);

    /* function lookup function and data */
    xmlXPathFuncLookupFunc funcLookupFunc : itype(_Ptr<_Ptr<void (_Ptr<xmlXPathParserContext>, int)> (void *, _Ptr<const xmlChar>, _Ptr<const xmlChar>)>);/* function lookup func */
    void *funcLookupData;		/* function lookup data */

    /* temporary namespace lists kept for walking the namespace axis */
    xmlNsPtr *tmpNsList : itype(_Ptr<xmlNsPtr>);		/* Array of namespaces */
    int tmpNsNr;			/* number of namespaces in scope */

    /* error reporting mechanism */
    void *userData;                     /* user specific data block */
    xmlStructuredErrorFunc error : itype(_Ptr<void (void *, xmlErrorPtr)>);       /* the callback in case of errors */
    xmlError lastError;			/* the last error */
    xmlNodePtr debugNode : itype(_Ptr<xmlNode>);		/* the source node XSLT */

    /* dictionary */
    xmlDictPtr dict : itype(_Ptr<xmlDict>);			/* dictionary if any */

    int flags;				/* flags to control compilation */

    /* Cache for reusal of XPath objects */
    void *cache;

    /* Resource limits */
    unsigned long opLimit;
    unsigned long opCount;
    int depth;
    int maxDepth;
    int maxParserDepth;
};

/*
 * The structure of a compiled expression form is not public.
 */

typedef struct _xmlXPathCompExpr xmlXPathCompExpr;
typedef xmlXPathCompExpr *xmlXPathCompExprPtr;

/**
 * xmlXPathParserContext:
 *
 * An XPath parser context. It contains pure parsing informations,
 * an xmlXPathContext, and the stack of objects.
 */
struct _xmlXPathParserContext {
    const xmlChar *cur : itype(_Ptr<const xmlChar>);			/* the current char being parsed */
    const xmlChar *base : itype(_Ptr<const xmlChar>);			/* the full expression */

    int error;				/* error code */

    xmlXPathContextPtr context : itype(_Ptr<xmlXPathContext>);	/* the evaluation context */
    xmlXPathObjectPtr value : itype(_Ptr<xmlXPathObject>);	/* the current value */
    int                 valueNr;	/* number of values stacked */
    int                valueMax;	/* max number of values stacked */
    xmlXPathObjectPtr *valueTab : itype(_Ptr<xmlXPathObjectPtr>);	/* stack of values */

    xmlXPathCompExprPtr comp : itype(_Ptr<xmlXPathCompExpr>);		/* the precompiled expression */
    int xptr;				/* it this an XPointer expression */
    xmlNodePtr ancestor : itype(_Ptr<xmlNode>);	/* used for walking preceding axis */

    int              valueFrame;        /* used to limit Pop on the stack */
};

/************************************************************************
 *									*
 *			Public API					*
 *									*
 ************************************************************************/

/**
 * Objects and Nodesets handling
 */

XMLPUBVAR double xmlXPathNAN;
XMLPUBVAR double xmlXPathPINF;
XMLPUBVAR double xmlXPathNINF;

/* These macros may later turn into functions */
/**
 * xmlXPathNodeSetGetLength:
 * @ns:  a node-set
 *
 * Implement a functionality similar to the DOM NodeList.length.
 *
 * Returns the number of nodes in the node-set.
 */
#define xmlXPathNodeSetGetLength(ns) ((ns) ? (ns)->nodeNr : 0)
/**
 * xmlXPathNodeSetItem:
 * @ns:  a node-set
 * @index:  index of a node in the set
 *
 * Implements a functionality similar to the DOM NodeList.item().
 *
 * Returns the xmlNodePtr at the given @index in @ns or NULL if
 *         @index is out of range (0 to length-1)
 */
#define xmlXPathNodeSetItem(ns, index)				\
		((((ns) != NULL) &&				\
		  ((index) >= 0) && ((index) < (ns)->nodeNr)) ?	\
		 (ns)->nodeTab[(index)]				\
		 : NULL)
/**
 * xmlXPathNodeSetIsEmpty:
 * @ns: a node-set
 *
 * Checks whether @ns is empty or not.
 *
 * Returns %TRUE if @ns is an empty node-set.
 */
#define xmlXPathNodeSetIsEmpty(ns)                                      \
    (((ns) == NULL) || ((ns)->nodeNr == 0) || ((ns)->nodeTab == NULL))


XMLPUBFUN void XMLCALL
		    xmlXPathFreeObject		(xmlXPathObjectPtr obj : itype(_Ptr<xmlXPathObject>));
XMLPUBFUN xmlNodeSetPtr xmlXPathNodeSetCreate(xmlNodePtr val : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlNodeSet>);
XMLPUBFUN void XMLCALL
		    xmlXPathFreeNodeSetList	(xmlXPathObjectPtr obj : itype(_Ptr<xmlXPathObject>));
XMLPUBFUN void XMLCALL
		    xmlXPathFreeNodeSet		(xmlNodeSetPtr obj : itype(_Ptr<xmlNodeSet>));
XMLPUBFUN xmlXPathObjectPtr xmlXPathObjectCopy(xmlXPathObjectPtr val : itype(_Ptr<xmlXPathObject>)) : itype(_Ptr<xmlXPathObject>);
XMLPUBFUN int XMLCALL
		    xmlXPathCmpNodes		(xmlNodePtr node1 : itype(_Ptr<xmlNode>), xmlNodePtr node2 : itype(_Ptr<xmlNode>));
/**
 * Conversion functions to basic types.
 */
XMLPUBFUN int XMLCALL
		    xmlXPathCastNumberToBoolean	(double val);
XMLPUBFUN int XMLCALL
		    xmlXPathCastStringToBoolean	(const xmlChar *val : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
		    xmlXPathCastNodeSetToBoolean(xmlNodeSetPtr ns : itype(_Ptr<xmlNodeSet>));
XMLPUBFUN int XMLCALL
		    xmlXPathCastToBoolean	(xmlXPathObjectPtr val : itype(_Ptr<xmlXPathObject>));

XMLPUBFUN double XMLCALL
		    xmlXPathCastBooleanToNumber	(int val);
XMLPUBFUN double XMLCALL
		    xmlXPathCastStringToNumber	(const xmlChar *val : itype(_Ptr<const xmlChar>));
XMLPUBFUN double XMLCALL
		    xmlXPathCastNodeToNumber	(xmlNodePtr node : itype(_Ptr<xmlNode>));
XMLPUBFUN double XMLCALL
		    xmlXPathCastNodeSetToNumber	(xmlNodeSetPtr ns : itype(_Ptr<xmlNodeSet>));
XMLPUBFUN double XMLCALL
		    xmlXPathCastToNumber	(xmlXPathObjectPtr val : itype(_Ptr<xmlXPathObject>));

XMLPUBFUN xmlChar *xmlXPathCastBooleanToString(int val) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlXPathCastNumberToString(double val) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlXPathCastNodeToString(xmlNodePtr node : itype(_Ptr<xmlNode>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlXPathCastNodeSetToString(xmlNodeSetPtr ns : itype(_Ptr<xmlNodeSet>)) : itype(_Ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlXPathCastToString(xmlXPathObjectPtr val : itype(_Ptr<xmlXPathObject>)) : itype(_Ptr<xmlChar>);

XMLPUBFUN xmlXPathObjectPtr xmlXPathConvertBoolean(xmlXPathObjectPtr val : itype(_Ptr<xmlXPathObject>)) : itype(_Ptr<xmlXPathObject>);
XMLPUBFUN xmlXPathObjectPtr xmlXPathConvertNumber(xmlXPathObjectPtr val : itype(_Ptr<xmlXPathObject>)) : itype(_Ptr<xmlXPathObject>);
XMLPUBFUN xmlXPathObjectPtr xmlXPathConvertString(xmlXPathObjectPtr val : itype(_Ptr<xmlXPathObject>)) : itype(_Ptr<xmlXPathObject>);

/**
 * Context handling.
 */
XMLPUBFUN xmlXPathContextPtr xmlXPathNewContext(xmlDocPtr doc : itype(_Ptr<xmlDoc>)) : itype(_Ptr<xmlXPathContext>);
XMLPUBFUN void XMLCALL
		    xmlXPathFreeContext		(xmlXPathContextPtr ctxt : itype(_Ptr<xmlXPathContext>));
XMLPUBFUN int XMLCALL
		    xmlXPathContextSetCache(xmlXPathContextPtr ctxt : itype(_Ptr<xmlXPathContext>), int active, int value, int options);
/**
 * Evaluation functions.
 */
XMLPUBFUN long XMLCALL
		    xmlXPathOrderDocElems	(xmlDocPtr doc : itype(_Ptr<xmlDoc>));
XMLPUBFUN int XMLCALL
		    xmlXPathSetContextNode	(xmlNodePtr node : itype(_Ptr<xmlNode>), xmlXPathContextPtr ctx : itype(_Ptr<xmlXPathContext>));
XMLPUBFUN xmlXPathObjectPtr xmlXPathNodeEval(xmlNodePtr node : itype(_Ptr<xmlNode>), const xmlChar *str : itype(_Ptr<const xmlChar>), xmlXPathContextPtr ctx : itype(_Ptr<xmlXPathContext>)) : itype(_Ptr<xmlXPathObject>);
XMLPUBFUN xmlXPathObjectPtr xmlXPathEval(const xmlChar *str : itype(_Ptr<const xmlChar>), xmlXPathContextPtr ctx : itype(_Ptr<xmlXPathContext>)) : itype(_Ptr<xmlXPathObject>);
XMLPUBFUN xmlXPathObjectPtr xmlXPathEvalExpression(const xmlChar *str : itype(_Ptr<const xmlChar>), xmlXPathContextPtr ctxt : itype(_Ptr<xmlXPathContext>)) : itype(_Ptr<xmlXPathObject>);
XMLPUBFUN int XMLCALL
		    xmlXPathEvalPredicate	(xmlXPathContextPtr ctxt : itype(_Ptr<xmlXPathContext>), xmlXPathObjectPtr res : itype(_Ptr<xmlXPathObject>));
/**
 * Separate compilation/evaluation entry points.
 */
XMLPUBFUN xmlXPathCompExprPtr xmlXPathCompile(const xmlChar *str : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlXPathCompExpr>);
XMLPUBFUN xmlXPathCompExprPtr xmlXPathCtxtCompile(xmlXPathContextPtr ctxt : itype(_Ptr<xmlXPathContext>), const xmlChar *str : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlXPathCompExpr>);
XMLPUBFUN xmlXPathObjectPtr xmlXPathCompiledEval(xmlXPathCompExprPtr comp : itype(_Ptr<xmlXPathCompExpr>), xmlXPathContextPtr ctx : itype(_Ptr<xmlXPathContext>)) : itype(_Ptr<xmlXPathObject>);
XMLPUBFUN int XMLCALL
		    xmlXPathCompiledEvalToBoolean(xmlXPathCompExprPtr comp : itype(_Ptr<xmlXPathCompExpr>), xmlXPathContextPtr ctxt : itype(_Ptr<xmlXPathContext>));
XMLPUBFUN void XMLCALL
		    xmlXPathFreeCompExpr	(xmlXPathCompExprPtr comp : itype(_Ptr<xmlXPathCompExpr>));
#endif /* LIBXML_XPATH_ENABLED */
#if defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN void XMLCALL
		    xmlXPathInit		(void);
XMLPUBFUN int XMLCALL
		xmlXPathIsNaN	(double val);
XMLPUBFUN int XMLCALL
		xmlXPathIsInf	(double val);

#ifdef __cplusplus
}
#endif

#endif /* LIBXML_XPATH_ENABLED or LIBXML_SCHEMAS_ENABLED*/
#endif /* ! __XML_XPATH_H__ */
