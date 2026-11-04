/*
 * Summary: API to build regexp automata
 * Description: the API to build regexp automata
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_AUTOMATA_H__
#define __XML_AUTOMATA_H__

#include <libxml/xmlversion.h>
#include <libxml/tree.h>

#ifdef LIBXML_REGEXP_ENABLED
#ifdef LIBXML_AUTOMATA_ENABLED
#include <libxml/xmlregexp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * xmlAutomataPtr:
 *
 * A libxml automata description, It can be compiled into a regexp
 */
typedef struct _xmlAutomata xmlAutomata;
typedef xmlAutomata *xmlAutomataPtr;

/**
 * xmlAutomataStatePtr:
 *
 * A state int the automata description,
 */
typedef struct _xmlAutomataState xmlAutomataState;
typedef xmlAutomataState *xmlAutomataStatePtr;

/*
 * Building API
 */
XMLPUBFUN xmlAutomataPtr xmlNewAutomata(void) : itype(_Ptr<xmlAutomata>);
XMLPUBFUN void XMLCALL
		    xmlFreeAutomata		(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>));

XMLPUBFUN xmlAutomataStatePtr xmlAutomataGetInitState(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>)) : itype(_Ptr<xmlAutomataState>);
XMLPUBFUN int XMLCALL
		    xmlAutomataSetFinalState	(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>), xmlAutomataStatePtr state : itype(_Ptr<xmlAutomataState>));
XMLPUBFUN xmlAutomataStatePtr xmlAutomataNewState(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>)) : itype(_Ptr<xmlAutomataState>);
XMLPUBFUN xmlAutomataStatePtr xmlAutomataNewTransition(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>), xmlAutomataStatePtr from : itype(_Ptr<xmlAutomataState>), xmlAutomataStatePtr to : itype(_Ptr<xmlAutomataState>), const xmlChar *token : itype(_Ptr<const xmlChar>), void *data) : itype(_Ptr<xmlAutomataState>);
XMLPUBFUN xmlAutomataStatePtr xmlAutomataNewTransition2(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>), xmlAutomataStatePtr from : itype(_Ptr<xmlAutomataState>), xmlAutomataStatePtr to : itype(_Ptr<xmlAutomataState>), const xmlChar *token : itype(_Ptr<const xmlChar>), const xmlChar *token2 : itype(_Ptr<const xmlChar>), void *data) : itype(_Ptr<xmlAutomataState>);
XMLPUBFUN xmlAutomataStatePtr xmlAutomataNewNegTrans(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>), xmlAutomataStatePtr from : itype(_Ptr<xmlAutomataState>), xmlAutomataStatePtr to : itype(_Ptr<xmlAutomataState>), const xmlChar *token : itype(_Ptr<const xmlChar>), const xmlChar *token2 : itype(_Ptr<const xmlChar>), void *data) : itype(_Ptr<xmlAutomataState>);

XMLPUBFUN xmlAutomataStatePtr xmlAutomataNewCountTrans(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>), xmlAutomataStatePtr from : itype(_Ptr<xmlAutomataState>), xmlAutomataStatePtr to : itype(_Ptr<xmlAutomataState>), const xmlChar *token : itype(_Ptr<const xmlChar>), int min, int max, void *data) : itype(_Ptr<xmlAutomataState>);
XMLPUBFUN xmlAutomataStatePtr xmlAutomataNewCountTrans2(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>), xmlAutomataStatePtr from : itype(_Ptr<xmlAutomataState>), xmlAutomataStatePtr to : itype(_Ptr<xmlAutomataState>), const xmlChar *token : itype(_Ptr<const xmlChar>), const xmlChar *token2 : itype(_Ptr<const xmlChar>), int min, int max, void *data) : itype(_Ptr<xmlAutomataState>);
XMLPUBFUN xmlAutomataStatePtr xmlAutomataNewOnceTrans(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>), xmlAutomataStatePtr from : itype(_Ptr<xmlAutomataState>), xmlAutomataStatePtr to : itype(_Ptr<xmlAutomataState>), const xmlChar *token : itype(_Ptr<const xmlChar>), int min, int max, void *data) : itype(_Ptr<xmlAutomataState>);
XMLPUBFUN xmlAutomataStatePtr xmlAutomataNewOnceTrans2(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>), xmlAutomataStatePtr from : itype(_Ptr<xmlAutomataState>), xmlAutomataStatePtr to : itype(_Ptr<xmlAutomataState>), const xmlChar *token : itype(_Ptr<const xmlChar>), const xmlChar *token2 : itype(_Ptr<const xmlChar>), int min, int max, void *data) : itype(_Ptr<xmlAutomataState>);
XMLPUBFUN xmlAutomataStatePtr xmlAutomataNewAllTrans(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>), xmlAutomataStatePtr from : itype(_Ptr<xmlAutomataState>), xmlAutomataStatePtr to : itype(_Ptr<xmlAutomataState>), int lax) : itype(_Ptr<xmlAutomataState>);
XMLPUBFUN xmlAutomataStatePtr xmlAutomataNewEpsilon(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>), xmlAutomataStatePtr from : itype(_Ptr<xmlAutomataState>), xmlAutomataStatePtr to : itype(_Ptr<xmlAutomataState>)) : itype(_Ptr<xmlAutomataState>);
XMLPUBFUN xmlAutomataStatePtr xmlAutomataNewCountedTrans(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>), xmlAutomataStatePtr from : itype(_Ptr<xmlAutomataState>), xmlAutomataStatePtr to : itype(_Ptr<xmlAutomataState>), int counter) : itype(_Ptr<xmlAutomataState>);
XMLPUBFUN xmlAutomataStatePtr xmlAutomataNewCounterTrans(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>), xmlAutomataStatePtr from : itype(_Ptr<xmlAutomataState>), xmlAutomataStatePtr to : itype(_Ptr<xmlAutomataState>), int counter) : itype(_Ptr<xmlAutomataState>);
XMLPUBFUN int XMLCALL
		    xmlAutomataNewCounter	(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>), int min, int max);

XMLPUBFUN xmlRegexpPtr xmlAutomataCompile(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>)) : itype(_Ptr<xmlRegexp>);
XMLPUBFUN int XMLCALL
		    xmlAutomataIsDeterminist	(xmlAutomataPtr am : itype(_Ptr<xmlAutomata>));

#ifdef __cplusplus
}
#endif

#endif /* LIBXML_AUTOMATA_ENABLED */
#endif /* LIBXML_REGEXP_ENABLED */

#endif /* __XML_AUTOMATA_H__ */
