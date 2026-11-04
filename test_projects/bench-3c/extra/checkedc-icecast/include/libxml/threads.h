/**
 * Summary: interfaces for thread handling
 * Description: set of generic threading related routines
 *              should work with pthreads, Windows native or TLS threads
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_THREADS_H__
#define __XML_THREADS_H__

#include <libxml/xmlversion.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * xmlMutex are a simple mutual exception locks.
 */
typedef struct _xmlMutex xmlMutex;
typedef xmlMutex *xmlMutexPtr;

/*
 * xmlRMutex are reentrant mutual exception locks.
 */
typedef struct _xmlRMutex xmlRMutex;
typedef xmlRMutex *xmlRMutexPtr;

#ifdef __cplusplus
}
#endif
#include <libxml/globals.h>
#ifdef __cplusplus
extern "C" {
#endif
XMLPUBFUN xmlMutexPtr xmlNewMutex(void) : itype(_Ptr<xmlMutex>);
XMLPUBFUN void XMLCALL
			xmlMutexLock	(xmlMutexPtr tok : itype(_Ptr<xmlMutex>));
XMLPUBFUN void XMLCALL
			xmlMutexUnlock	(xmlMutexPtr tok : itype(_Ptr<xmlMutex>));
XMLPUBFUN void XMLCALL
			xmlFreeMutex	(xmlMutexPtr tok : itype(_Ptr<xmlMutex>));

XMLPUBFUN xmlRMutexPtr xmlNewRMutex(void) : itype(_Ptr<xmlRMutex>);
XMLPUBFUN void XMLCALL
			xmlRMutexLock	(xmlRMutexPtr tok : itype(_Ptr<xmlRMutex>));
XMLPUBFUN void XMLCALL
			xmlRMutexUnlock	(xmlRMutexPtr tok : itype(_Ptr<xmlRMutex>));
XMLPUBFUN void XMLCALL
			xmlFreeRMutex	(xmlRMutexPtr tok : itype(_Ptr<xmlRMutex>));

/*
 * Library wide APIs.
 */
XMLPUBFUN void XMLCALL
			xmlInitThreads	(void);
XMLPUBFUN void XMLCALL
			xmlLockLibrary	(void);
XMLPUBFUN void XMLCALL
			xmlUnlockLibrary(void);
XMLPUBFUN int XMLCALL
			xmlGetThreadId	(void);
XMLPUBFUN int XMLCALL
			xmlIsMainThread	(void);
XMLPUBFUN void XMLCALL
			xmlCleanupThreads(void);
XMLPUBFUN xmlGlobalStatePtr xmlGetGlobalState(void) : itype(_Ptr<xmlGlobalState>);

#ifdef HAVE_PTHREAD_H
#elif defined(HAVE_WIN32_THREADS) && !defined(HAVE_COMPILER_TLS) && (!defined(LIBXML_STATIC) || defined(LIBXML_STATIC_FOR_DLL))
#if defined(LIBXML_STATIC_FOR_DLL)
int XMLCALL
xmlDllMain(void *hinstDLL, unsigned long fdwReason,
           void *lpvReserved);
#endif
#endif

#ifdef __cplusplus
}
#endif


#endif /* __XML_THREADS_H__ */
