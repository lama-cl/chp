/*
 * Summary: lists interfaces
 * Description: this module implement the list support used in
 * various place in the library.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Gary Pennington <Gary.Pennington@uk.sun.com>
 */

#ifndef __XML_LINK_INCLUDE__
#define __XML_LINK_INCLUDE__

#include <libxml/xmlversion.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _xmlLink xmlLink;
typedef xmlLink *xmlLinkPtr;

typedef struct _xmlList xmlList;
typedef xmlList *xmlListPtr;

/**
 * xmlListDeallocator:
 * @lk:  the data to deallocate
 *
 * Callback function used to free data from a list.
 */
typedef void (*xmlListDeallocator) (xmlLinkPtr lk);
/**
 * xmlListDataCompare:
 * @data0: the first data
 * @data1: the second data
 *
 * Callback function used to compare 2 data.
 *
 * Returns 0 is equality, -1 or 1 otherwise depending on the ordering.
 */
typedef int  (*xmlListDataCompare) (const void *data0, const void *data1);
/**
 * xmlListWalker:
 * @data: the data found in the list
 * @user: extra user provided data to the walker
 *
 * Callback function used when walking a list with xmlListWalk().
 *
 * Returns 0 to stop walking the list, 1 otherwise.
 */
typedef int (*xmlListWalker) (const void *data, void *user);

/* Creation/Deletion */
XMLPUBFUN xmlListPtr xmlListCreate(xmlListDeallocator deallocator : itype(_Ptr<void (xmlLinkPtr)>), xmlListDataCompare compare : itype(_Ptr<int (const void *, const void *)>)) : itype(_Ptr<xmlList>);
XMLPUBFUN void XMLCALL
		xmlListDelete		(xmlListPtr l : itype(_Ptr<xmlList>));

/* Basic Operators */
XMLPUBFUN void * XMLCALL
		xmlListSearch		(xmlListPtr l : itype(_Ptr<xmlList>), void *data);
XMLPUBFUN void * XMLCALL
		xmlListReverseSearch	(xmlListPtr l : itype(_Ptr<xmlList>), void *data);
XMLPUBFUN int XMLCALL
		xmlListInsert		(xmlListPtr l : itype(_Ptr<xmlList>), void *data) ;
XMLPUBFUN int XMLCALL
		xmlListAppend		(xmlListPtr l : itype(_Ptr<xmlList>), void *data) ;
XMLPUBFUN int XMLCALL
		xmlListRemoveFirst	(xmlListPtr l : itype(_Ptr<xmlList>), void *data);
XMLPUBFUN int XMLCALL
		xmlListRemoveLast	(xmlListPtr l : itype(_Ptr<xmlList>), void *data);
XMLPUBFUN int XMLCALL
		xmlListRemoveAll	(xmlListPtr l : itype(_Ptr<xmlList>), void *data);
XMLPUBFUN void XMLCALL
		xmlListClear		(xmlListPtr l : itype(_Ptr<xmlList>));
XMLPUBFUN int XMLCALL
		xmlListEmpty		(xmlListPtr l : itype(_Ptr<xmlList>));
XMLPUBFUN xmlLinkPtr xmlListFront(xmlListPtr l : itype(_Ptr<xmlList>)) : itype(_Ptr<xmlLink>);
XMLPUBFUN xmlLinkPtr xmlListEnd(xmlListPtr l : itype(_Ptr<xmlList>)) : itype(_Ptr<xmlLink>);
XMLPUBFUN int XMLCALL
		xmlListSize		(xmlListPtr l : itype(_Ptr<xmlList>));

XMLPUBFUN void XMLCALL
		xmlListPopFront		(xmlListPtr l : itype(_Ptr<xmlList>));
XMLPUBFUN void XMLCALL
		xmlListPopBack		(xmlListPtr l : itype(_Ptr<xmlList>));
XMLPUBFUN int XMLCALL
		xmlListPushFront	(xmlListPtr l : itype(_Ptr<xmlList>), void *data);
XMLPUBFUN int XMLCALL
		xmlListPushBack		(xmlListPtr l : itype(_Ptr<xmlList>), void *data);

/* Advanced Operators */
XMLPUBFUN void XMLCALL
		xmlListReverse		(xmlListPtr l : itype(_Ptr<xmlList>));
XMLPUBFUN void XMLCALL
		xmlListSort		(xmlListPtr l : itype(_Ptr<xmlList>));
XMLPUBFUN void XMLCALL
		xmlListWalk		(xmlListPtr l : itype(_Ptr<xmlList>), xmlListWalker walker : itype(_Ptr<int (const void *, void *)>), void *user);
XMLPUBFUN void XMLCALL
		xmlListReverseWalk	(xmlListPtr l : itype(_Ptr<xmlList>), xmlListWalker walker : itype(_Ptr<int (const void *, void *)>), void *user);
XMLPUBFUN void XMLCALL
		xmlListMerge		(xmlListPtr l1 : itype(_Ptr<xmlList>), xmlListPtr l2 : itype(_Ptr<xmlList>));
XMLPUBFUN xmlListPtr xmlListDup(const xmlListPtr old : itype(const _Ptr<xmlList>)) : itype(_Ptr<xmlList>);
XMLPUBFUN int XMLCALL
		xmlListCopy		(xmlListPtr cur : itype(_Ptr<xmlList>), const xmlListPtr old : itype(const _Ptr<xmlList>));
/* Link operators */
XMLPUBFUN void * XMLCALL
		xmlLinkGetData          (xmlLinkPtr lk : itype(_Ptr<xmlLink>));

/* xmlListUnique() */
/* xmlListSwap */

#ifdef __cplusplus
}
#endif

#endif /* __XML_LINK_INCLUDE__ */
