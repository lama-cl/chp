/*
 * Summary: Chained hash tables
 * Description: This module implements the hash table support used in
 *		various places in the library.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Bjorn Reese <bjorn.reese@systematic.dk>
 */

#ifndef __XML_HASH_H__
#define __XML_HASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The hash table.
 */
typedef struct _xmlHashTable xmlHashTable;
typedef xmlHashTable *xmlHashTablePtr;

#ifdef __cplusplus
}
#endif

#include <libxml/xmlversion.h>
#include <libxml/parser.h>
#include <libxml/dict.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Recent version of gcc produce a warning when a function pointer is assigned
 * to an object pointer, or vice versa.  The following macro is a dirty hack
 * to allow suppression of the warning.  If your architecture has function
 * pointers which are a different size than a void pointer, there may be some
 * serious trouble within the library.
 */
/**
 * XML_CAST_FPTR:
 * @fptr:  pointer to a function
 *
 * Macro to do a casting from an object pointer to a
 * function pointer without encountering a warning from
 * gcc
 *
 * #define XML_CAST_FPTR(fptr) (*(void **)(&fptr))
 * This macro violated ISO C aliasing rules (gcc4 on s390 broke)
 * so it is disabled now
 */

#define XML_CAST_FPTR(fptr) fptr


/*
 * function types:
 */
/**
 * xmlHashDeallocator:
 * @payload:  the data in the hash
 * @name:  the name associated
 *
 * Callback to free data from a hash.
 */
typedef void (*xmlHashDeallocator)(void *payload, const xmlChar *name);
/**
 * xmlHashCopier:
 * @payload:  the data in the hash
 * @name:  the name associated
 *
 * Callback to copy data from a hash.
 *
 * Returns a copy of the data or NULL in case of error.
 */
typedef void *(*xmlHashCopier)(void *payload, const xmlChar *name);
/**
 * xmlHashScanner:
 * @payload:  the data in the hash
 * @data:  extra scanner data
 * @name:  the name associated
 *
 * Callback when scanning data in a hash with the simple scanner.
 */
typedef void (*xmlHashScanner)(void *payload, void *data, const xmlChar *name);
/**
 * xmlHashScannerFull:
 * @payload:  the data in the hash
 * @data:  extra scanner data
 * @name:  the name associated
 * @name2:  the second name associated
 * @name3:  the third name associated
 *
 * Callback when scanning data in a hash with the full scanner.
 */
typedef void (*xmlHashScannerFull)(void *payload, void *data,
				   const xmlChar *name, const xmlChar *name2,
				   const xmlChar *name3);

/*
 * Constructor and destructor.
 */
XMLPUBFUN xmlHashTablePtr xmlHashCreate(int size) : itype(_Ptr<xmlHashTable>);
XMLPUBFUN xmlHashTablePtr xmlHashCreateDict(int size, xmlDictPtr dict : itype(_Ptr<xmlDict>)) : itype(_Ptr<xmlHashTable>);
XMLPUBFUN void XMLCALL
			xmlHashFree	(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), xmlHashDeallocator f : itype(_Ptr<void (void *, const xmlChar *)>));
XMLPUBFUN void XMLCALL
			xmlHashDefaultDeallocator(void *entry, const xmlChar *name : itype(_Ptr<const xmlChar>));

/*
 * Add a new entry to the hash table.
 */
XMLPUBFUN int XMLCALL
			xmlHashAddEntry	(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), void *userdata);
XMLPUBFUN int XMLCALL
			xmlHashUpdateEntry(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), void *userdata, xmlHashDeallocator f : itype(_Ptr<void (void *, const xmlChar *)>));
XMLPUBFUN int XMLCALL
			xmlHashAddEntry2(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *name2 : itype(_Ptr<const xmlChar>), void *userdata);
XMLPUBFUN int XMLCALL
			xmlHashUpdateEntry2(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *name2 : itype(_Ptr<const xmlChar>), void *userdata, xmlHashDeallocator f : itype(_Ptr<void (void *, const xmlChar *)>));
XMLPUBFUN int XMLCALL
			xmlHashAddEntry3(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *name2 : itype(_Ptr<const xmlChar>), const xmlChar *name3 : itype(_Ptr<const xmlChar>), void *userdata);
XMLPUBFUN int XMLCALL
			xmlHashUpdateEntry3(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *name2 : itype(_Ptr<const xmlChar>), const xmlChar *name3 : itype(_Ptr<const xmlChar>), void *userdata, xmlHashDeallocator f : itype(_Ptr<void (void *, const xmlChar *)>));

/*
 * Remove an entry from the hash table.
 */
XMLPUBFUN int XMLCALL
			xmlHashRemoveEntry(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), xmlHashDeallocator f : itype(_Ptr<void (void *, const xmlChar *)>));
XMLPUBFUN int XMLCALL
			xmlHashRemoveEntry2(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *name2 : itype(_Ptr<const xmlChar>), xmlHashDeallocator f : itype(_Ptr<void (void *, const xmlChar *)>));
XMLPUBFUN int  XMLCALL
			xmlHashRemoveEntry3(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *name2 : itype(_Ptr<const xmlChar>), const xmlChar *name3 : itype(_Ptr<const xmlChar>), xmlHashDeallocator f : itype(_Ptr<void (void *, const xmlChar *)>));

/*
 * Retrieve the userdata.
 */
XMLPUBFUN void * XMLCALL
			xmlHashLookup	(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>));
XMLPUBFUN void * XMLCALL
			xmlHashLookup2	(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *name2 : itype(_Ptr<const xmlChar>));
XMLPUBFUN void * XMLCALL
			xmlHashLookup3	(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *name2 : itype(_Ptr<const xmlChar>), const xmlChar *name3 : itype(_Ptr<const xmlChar>));
XMLPUBFUN void * XMLCALL
			xmlHashQLookup	(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *prefix : itype(_Ptr<const xmlChar>));
XMLPUBFUN void * XMLCALL
			xmlHashQLookup2	(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *prefix : itype(_Ptr<const xmlChar>), const xmlChar *name2 : itype(_Ptr<const xmlChar>), const xmlChar *prefix2 : itype(_Ptr<const xmlChar>));
XMLPUBFUN void * XMLCALL
			xmlHashQLookup3	(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *prefix : itype(_Ptr<const xmlChar>), const xmlChar *name2 : itype(_Ptr<const xmlChar>), const xmlChar *prefix2 : itype(_Ptr<const xmlChar>), const xmlChar *name3 : itype(_Ptr<const xmlChar>), const xmlChar *prefix3 : itype(_Ptr<const xmlChar>));

/*
 * Helpers.
 */
XMLPUBFUN xmlHashTablePtr xmlHashCopy(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), xmlHashCopier f : itype(_Ptr<void *(void *, const xmlChar *)>)) : itype(_Ptr<xmlHashTable>);
XMLPUBFUN int XMLCALL
			xmlHashSize	(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>));
XMLPUBFUN void XMLCALL
			xmlHashScan	(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), xmlHashScanner f : itype(_Ptr<void (void *, void *, const xmlChar *)>), void *data);
XMLPUBFUN void XMLCALL
			xmlHashScan3	(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *name2 : itype(_Ptr<const xmlChar>), const xmlChar *name3 : itype(_Ptr<const xmlChar>), xmlHashScanner f : itype(_Ptr<void (void *, void *, const xmlChar *)>), void *data);
XMLPUBFUN void XMLCALL
			xmlHashScanFull	(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), xmlHashScannerFull f : itype(_Ptr<void (void *, void *, const xmlChar *, const xmlChar *, const xmlChar *)>), void *data);
XMLPUBFUN void XMLCALL
			xmlHashScanFull3(xmlHashTablePtr table : itype(_Ptr<xmlHashTable>), const xmlChar *name : itype(_Ptr<const xmlChar>), const xmlChar *name2 : itype(_Ptr<const xmlChar>), const xmlChar *name3 : itype(_Ptr<const xmlChar>), xmlHashScannerFull f : itype(_Ptr<void (void *, void *, const xmlChar *, const xmlChar *, const xmlChar *)>), void *data);
#ifdef __cplusplus
}
#endif
#endif /* ! __XML_HASH_H__ */
