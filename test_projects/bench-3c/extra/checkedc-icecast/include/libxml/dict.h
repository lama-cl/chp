/*
 * Summary: string dictionary
 * Description: dictionary of reusable strings, just used to avoid allocation
 *         and freeing operations.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_DICT_H__
#define __XML_DICT_H__

#include <stddef.h>
#include <libxml/xmlversion.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The dictionary.
 */
typedef struct _xmlDict xmlDict;
typedef xmlDict *xmlDictPtr;

/*
 * Initializer
 */
XMLPUBFUN int XMLCALL  xmlInitializeDict(void);

/*
 * Constructor and destructor.
 */
XMLPUBFUN xmlDictPtr xmlDictCreate(void) : itype(_Ptr<xmlDict>);
XMLPUBFUN size_t XMLCALL
			xmlDictSetLimit	(xmlDictPtr dict : itype(_Ptr<xmlDict>), size_t limit);
XMLPUBFUN size_t XMLCALL
			xmlDictGetUsage (xmlDictPtr dict : itype(_Ptr<xmlDict>));
XMLPUBFUN xmlDictPtr xmlDictCreateSub(xmlDictPtr sub : itype(_Ptr<xmlDict>)) : itype(_Ptr<xmlDict>);
XMLPUBFUN int XMLCALL
			xmlDictReference(xmlDictPtr dict : itype(_Ptr<xmlDict>));
XMLPUBFUN void XMLCALL
			xmlDictFree	(xmlDictPtr dict : itype(_Ptr<xmlDict>));

/*
 * Lookup of entry in the dictionary.
 */
XMLPUBFUN const xmlChar *xmlDictLookup(xmlDictPtr dict : itype(_Ptr<xmlDict>), const xmlChar *name : itype(_Ptr<const xmlChar>), int len) : itype(_Ptr<const xmlChar>);
XMLPUBFUN const xmlChar *xmlDictExists(xmlDictPtr dict : itype(_Ptr<xmlDict>), const xmlChar *name : itype(_Ptr<const xmlChar>), int len) : itype(_Ptr<const xmlChar>);
XMLPUBFUN const xmlChar *xmlDictQLookup(xmlDictPtr dict : itype(_Ptr<xmlDict>), const xmlChar *prefix : itype(_Ptr<const xmlChar>), const xmlChar *name : itype(_Ptr<const xmlChar>)) : itype(_Ptr<const xmlChar>);
XMLPUBFUN int XMLCALL
			xmlDictOwns	(xmlDictPtr dict : itype(_Ptr<xmlDict>), const xmlChar *str : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
			xmlDictSize	(xmlDictPtr dict : itype(_Ptr<xmlDict>));

/*
 * Cleanup function
 */
XMLPUBFUN void XMLCALL
                        xmlDictCleanup  (void);

#ifdef __cplusplus
}
#endif
#endif /* ! __XML_DICT_H__ */
