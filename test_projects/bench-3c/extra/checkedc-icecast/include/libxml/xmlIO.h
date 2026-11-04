/*
 * Summary: interface for the I/O interfaces used by the parser
 * Description: interface for the I/O interfaces used by the parser
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_IO_H__
#define __XML_IO_H__

#include <stdio.h>
#include <libxml/xmlversion.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Those are the functions and datatypes for the parser input
 * I/O structures.
 */

/**
 * xmlInputMatchCallback:
 * @filename: the filename or URI
 *
 * Callback used in the I/O Input API to detect if the current handler
 * can provide input functionality for this resource.
 *
 * Returns 1 if yes and 0 if another Input module should be used
 */
typedef int (XMLCALL *xmlInputMatchCallback) (char const *filename);
/**
 * xmlInputOpenCallback:
 * @filename: the filename or URI
 *
 * Callback used in the I/O Input API to open the resource
 *
 * Returns an Input context or NULL in case or error
 */
typedef void * (XMLCALL *xmlInputOpenCallback) (char const *filename);
/**
 * xmlInputReadCallback:
 * @context:  an Input context
 * @buffer:  the buffer to store data read
 * @len:  the length of the buffer in bytes
 *
 * Callback used in the I/O Input API to read the resource
 *
 * Returns the number of bytes read or -1 in case of error
 */
typedef int (XMLCALL *xmlInputReadCallback) (void * context, char * buffer, int len);
/**
 * xmlInputCloseCallback:
 * @context:  an Input context
 *
 * Callback used in the I/O Input API to close the resource
 *
 * Returns 0 or -1 in case of error
 */
typedef int (XMLCALL *xmlInputCloseCallback) (void * context);

#ifdef LIBXML_OUTPUT_ENABLED
/*
 * Those are the functions and datatypes for the library output
 * I/O structures.
 */

/**
 * xmlOutputMatchCallback:
 * @filename: the filename or URI
 *
 * Callback used in the I/O Output API to detect if the current handler
 * can provide output functionality for this resource.
 *
 * Returns 1 if yes and 0 if another Output module should be used
 */
typedef int (XMLCALL *xmlOutputMatchCallback) (char const *filename);
/**
 * xmlOutputOpenCallback:
 * @filename: the filename or URI
 *
 * Callback used in the I/O Output API to open the resource
 *
 * Returns an Output context or NULL in case or error
 */
typedef void * (XMLCALL *xmlOutputOpenCallback) (char const *filename);
/**
 * xmlOutputWriteCallback:
 * @context:  an Output context
 * @buffer:  the buffer of data to write
 * @len:  the length of the buffer in bytes
 *
 * Callback used in the I/O Output API to write to the resource
 *
 * Returns the number of bytes written or -1 in case of error
 */
typedef int (XMLCALL *xmlOutputWriteCallback) (void * context, const char * buffer,
                                       int len);
/**
 * xmlOutputCloseCallback:
 * @context:  an Output context
 *
 * Callback used in the I/O Output API to close the resource
 *
 * Returns 0 or -1 in case of error
 */
typedef int (XMLCALL *xmlOutputCloseCallback) (void * context);
#endif /* LIBXML_OUTPUT_ENABLED */

#ifdef __cplusplus
}
#endif

#include <libxml/globals.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/encoding.h>

#ifdef __cplusplus
extern "C" {
#endif
struct _xmlParserInputBuffer {
    void*                  context;
    xmlInputReadCallback readcallback : itype(_Ptr<int (void *, char *, int)>);
    xmlInputCloseCallback closecallback : itype(_Ptr<int (void *)>);

    xmlCharEncodingHandlerPtr encoder : itype(_Ptr<xmlCharEncodingHandler>); /* I18N conversions to UTF-8 */

    xmlBufPtr buffer : itype(_Ptr<xmlBuf>);    /* Local buffer encoded in UTF-8 */
    xmlBufPtr raw : itype(_Ptr<xmlBuf>);       /* if encoder != NULL buffer for raw input */
    int	compressed;	    /* -1=unknown, 0=not compressed, 1=compressed */
    int error;
    unsigned long rawconsumed;/* amount consumed from raw */
};


#ifdef LIBXML_OUTPUT_ENABLED
struct _xmlOutputBuffer {
    void*                   context;
    xmlOutputWriteCallback writecallback : itype(_Ptr<int (void *, const char *, int)>);
    xmlOutputCloseCallback closecallback : itype(_Ptr<int (void *)>);

    xmlCharEncodingHandlerPtr encoder : itype(_Ptr<xmlCharEncodingHandler>); /* I18N conversions to UTF-8 */

    xmlBufPtr buffer : itype(_Ptr<xmlBuf>);    /* Local buffer encoded in UTF-8 or ISOLatin */
    xmlBufPtr conv : itype(_Ptr<xmlBuf>);      /* if encoder != NULL buffer for output */
    int written;            /* total number of byte written */
    int error;
};
#endif /* LIBXML_OUTPUT_ENABLED */

/*
 * Interfaces for input
 */
XMLPUBFUN void XMLCALL
	xmlCleanupInputCallbacks		(void);

XMLPUBFUN int XMLCALL
	xmlPopInputCallbacks			(void);

XMLPUBFUN void XMLCALL
	xmlRegisterDefaultInputCallbacks	(void);
XMLPUBFUN xmlParserInputBufferPtr xmlAllocParserInputBuffer(xmlCharEncoding enc) : itype(_Ptr<xmlParserInputBuffer>);

XMLPUBFUN xmlParserInputBufferPtr xmlParserInputBufferCreateFilename(const char *URI : itype(_Ptr<const char>), xmlCharEncoding enc) : itype(_Ptr<xmlParserInputBuffer>);
XMLPUBFUN xmlParserInputBufferPtr xmlParserInputBufferCreateFile(FILE *file : itype(_Ptr<FILE>), xmlCharEncoding enc) : itype(_Ptr<xmlParserInputBuffer>);
XMLPUBFUN xmlParserInputBufferPtr xmlParserInputBufferCreateFd(int fd, xmlCharEncoding enc) : itype(_Ptr<xmlParserInputBuffer>);
XMLPUBFUN xmlParserInputBufferPtr xmlParserInputBufferCreateMem(const char *mem : itype(_Ptr<const char>), int size, xmlCharEncoding enc) : itype(_Ptr<xmlParserInputBuffer>);
XMLPUBFUN xmlParserInputBufferPtr xmlParserInputBufferCreateStatic(const char *mem : itype(_Ptr<const char>), int size, xmlCharEncoding enc) : itype(_Ptr<xmlParserInputBuffer>);
XMLPUBFUN xmlParserInputBufferPtr xmlParserInputBufferCreateIO(xmlInputReadCallback ioread : itype(_Ptr<int (void *, char *, int)>), xmlInputCloseCallback ioclose : itype(_Ptr<int (void *)>), void *ioctx, xmlCharEncoding enc) : itype(_Ptr<xmlParserInputBuffer>);
XMLPUBFUN int XMLCALL
	xmlParserInputBufferRead		(xmlParserInputBufferPtr in : itype(_Ptr<xmlParserInputBuffer>), int len);
XMLPUBFUN int XMLCALL
	xmlParserInputBufferGrow		(xmlParserInputBufferPtr in : itype(_Ptr<xmlParserInputBuffer>), int len);
XMLPUBFUN int XMLCALL
	xmlParserInputBufferPush		(xmlParserInputBufferPtr in : itype(_Ptr<xmlParserInputBuffer>), int len, const char *buf : itype(_Ptr<const char>));
XMLPUBFUN void XMLCALL
	xmlFreeParserInputBuffer		(xmlParserInputBufferPtr in : itype(_Ptr<xmlParserInputBuffer>));
XMLPUBFUN char *xmlParserGetDirectory(const char *filename : itype(_Ptr<const char>)) : itype(_Ptr<char>);

XMLPUBFUN int XMLCALL
	xmlRegisterInputCallbacks		(xmlInputMatchCallback matchFunc : itype(_Ptr<int (const char *)>), xmlInputOpenCallback openFunc : itype(_Ptr<void *(const char *)>), xmlInputReadCallback readFunc : itype(_Ptr<int (void *, char *, int)>), xmlInputCloseCallback closeFunc : itype(_Ptr<int (void *)>));

xmlParserInputBufferPtr __xmlParserInputBufferCreateFilename(const char *URI : itype(_Ptr<const char>), xmlCharEncoding enc) : itype(_Ptr<xmlParserInputBuffer>);

#ifdef LIBXML_OUTPUT_ENABLED
/*
 * Interfaces for output
 */
XMLPUBFUN void XMLCALL
	xmlCleanupOutputCallbacks		(void);
XMLPUBFUN void XMLCALL
	xmlRegisterDefaultOutputCallbacks(void);
XMLPUBFUN xmlOutputBufferPtr xmlAllocOutputBuffer(xmlCharEncodingHandlerPtr encoder : itype(_Ptr<xmlCharEncodingHandler>)) : itype(_Ptr<xmlOutputBuffer>);

XMLPUBFUN xmlOutputBufferPtr xmlOutputBufferCreateFilename(const char *URI : itype(_Ptr<const char>), xmlCharEncodingHandlerPtr encoder : itype(_Ptr<xmlCharEncodingHandler>), int compression) : itype(_Ptr<xmlOutputBuffer>);

XMLPUBFUN xmlOutputBufferPtr xmlOutputBufferCreateFile(FILE *file : itype(_Ptr<FILE>), xmlCharEncodingHandlerPtr encoder : itype(_Ptr<xmlCharEncodingHandler>)) : itype(_Ptr<xmlOutputBuffer>);

XMLPUBFUN xmlOutputBufferPtr xmlOutputBufferCreateBuffer(xmlBufferPtr buffer : itype(_Ptr<xmlBuffer>), xmlCharEncodingHandlerPtr encoder : itype(_Ptr<xmlCharEncodingHandler>)) : itype(_Ptr<xmlOutputBuffer>);

XMLPUBFUN xmlOutputBufferPtr xmlOutputBufferCreateFd(int fd, xmlCharEncodingHandlerPtr encoder : itype(_Ptr<xmlCharEncodingHandler>)) : itype(_Ptr<xmlOutputBuffer>);

XMLPUBFUN xmlOutputBufferPtr xmlOutputBufferCreateIO(xmlOutputWriteCallback iowrite : itype(_Ptr<int (void *, const char *, int)>), xmlOutputCloseCallback ioclose : itype(_Ptr<int (void *)>), void *ioctx, xmlCharEncodingHandlerPtr encoder : itype(_Ptr<xmlCharEncodingHandler>)) : itype(_Ptr<xmlOutputBuffer>);

/* Couple of APIs to get the output without digging into the buffers */
XMLPUBFUN const xmlChar *xmlOutputBufferGetContent(xmlOutputBufferPtr out : itype(_Ptr<xmlOutputBuffer>)) : itype(_Ptr<const xmlChar>);
XMLPUBFUN size_t XMLCALL
        xmlOutputBufferGetSize          (xmlOutputBufferPtr out : itype(_Ptr<xmlOutputBuffer>));

XMLPUBFUN int XMLCALL
	xmlOutputBufferWrite		(xmlOutputBufferPtr out : itype(_Ptr<xmlOutputBuffer>), int len, const char *buf : itype(_Ptr<const char>));
XMLPUBFUN int XMLCALL
	xmlOutputBufferWriteString	(xmlOutputBufferPtr out : itype(_Ptr<xmlOutputBuffer>), const char *str : itype(_Ptr<const char>));
XMLPUBFUN int XMLCALL
	xmlOutputBufferWriteEscape	(xmlOutputBufferPtr out : itype(_Ptr<xmlOutputBuffer>), const xmlChar *str : itype(_Ptr<const xmlChar>), xmlCharEncodingOutputFunc escaping : itype(_Ptr<int (unsigned char *, int *, const unsigned char *, int *)>));

XMLPUBFUN int XMLCALL
	xmlOutputBufferFlush		(xmlOutputBufferPtr out : itype(_Ptr<xmlOutputBuffer>));
XMLPUBFUN int XMLCALL
	xmlOutputBufferClose		(xmlOutputBufferPtr out : itype(_Ptr<xmlOutputBuffer>));

XMLPUBFUN int XMLCALL
	xmlRegisterOutputCallbacks	(xmlOutputMatchCallback matchFunc : itype(_Ptr<int (const char *)>), xmlOutputOpenCallback openFunc : itype(_Ptr<void *(const char *)>), xmlOutputWriteCallback writeFunc : itype(_Ptr<int (void *, const char *, int)>), xmlOutputCloseCallback closeFunc : itype(_Ptr<int (void *)>));

xmlOutputBufferPtr __xmlOutputBufferCreateFilename(const char *URI : itype(_Ptr<const char>), xmlCharEncodingHandlerPtr encoder : itype(_Ptr<xmlCharEncodingHandler>), int compression) : itype(_Ptr<xmlOutputBuffer>);

#ifdef LIBXML_HTTP_ENABLED
/*  This function only exists if HTTP support built into the library  */
XMLPUBFUN void XMLCALL
	xmlRegisterHTTPPostCallbacks	(void );
#endif /* LIBXML_HTTP_ENABLED */

#endif /* LIBXML_OUTPUT_ENABLED */

XMLPUBFUN xmlParserInputPtr xmlCheckHTTPInput(xmlParserCtxtPtr ctxt : itype(_Ptr<xmlParserCtxt>), xmlParserInputPtr ret : itype(_Ptr<xmlParserInput>)) : itype(_Ptr<xmlParserInput>);

/*
 * A predefined entity loader disabling network accesses
 */
XMLPUBFUN xmlParserInputPtr xmlNoNetExternalEntityLoader(const char *URL : itype(_Ptr<const char>), const char *ID : itype(_Ptr<const char>), xmlParserCtxtPtr ctxt : itype(_Ptr<xmlParserCtxt>)) : itype(_Ptr<xmlParserInput>);

/*
 * xmlNormalizeWindowsPath is obsolete, don't use it.
 * Check xmlCanonicPath in uri.h for a better alternative.
 */
XMLPUBFUN xmlChar *xmlNormalizeWindowsPath(const xmlChar *path : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xmlChar>);

XMLPUBFUN int XMLCALL
	xmlCheckFilename		(const char *path : itype(_Ptr<const char>));
/**
 * Default 'file://' protocol callbacks
 */
XMLPUBFUN int XMLCALL
	xmlFileMatch			(const char *filename : itype(_Ptr<const char>));
XMLPUBFUN void * XMLCALL
	xmlFileOpen			(const char *filename : itype(_Ptr<const char>));
XMLPUBFUN int XMLCALL
	xmlFileRead			(void * context, char *buffer : itype(_Ptr<char>), int len);
XMLPUBFUN int XMLCALL
	xmlFileClose			(void * context);

/**
 * Default 'http://' protocol callbacks
 */
#ifdef LIBXML_HTTP_ENABLED
XMLPUBFUN int XMLCALL
	xmlIOHTTPMatch			(const char *filename : itype(_Ptr<const char>));
XMLPUBFUN void * XMLCALL
	xmlIOHTTPOpen			(const char *filename : itype(_Ptr<const char>));
#ifdef LIBXML_OUTPUT_ENABLED
XMLPUBFUN void * XMLCALL
	xmlIOHTTPOpenW			(const char *post_uri : itype(_Ptr<const char>), int   compression);
#endif /* LIBXML_OUTPUT_ENABLED */
XMLPUBFUN int XMLCALL
	xmlIOHTTPRead			(void * context, char *buffer : itype(_Ptr<char>), int len);
XMLPUBFUN int XMLCALL
	xmlIOHTTPClose			(void * context);
#endif /* LIBXML_HTTP_ENABLED */

/**
 * Default 'ftp://' protocol callbacks
 */
#ifdef LIBXML_FTP_ENABLED
XMLPUBFUN int XMLCALL
	xmlIOFTPMatch			(const char *filename : itype(_Ptr<const char>));
XMLPUBFUN void * XMLCALL
	xmlIOFTPOpen			(const char *filename : itype(_Ptr<const char>));
XMLPUBFUN int XMLCALL
	xmlIOFTPRead			(void * context, char *buffer : itype(_Ptr<char>), int len);
XMLPUBFUN int XMLCALL
	xmlIOFTPClose			(void * context);
#endif /* LIBXML_FTP_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* __XML_IO_H__ */
