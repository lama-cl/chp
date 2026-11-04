/*
 * Summary: Locale handling
 * Description: Interfaces for locale handling. Needed for language dependent
 *              sorting.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Nick Wellnhofer
 */

#ifndef __XML_XSLTLOCALE_H__
#define __XML_XSLTLOCALE_H__

#include <libxml/xmlstring.h>
#include "xsltexports.h"

#ifdef HAVE_STRXFRM_L

/*
 * XSLT_LOCALE_POSIX:
 * Macro indicating to use POSIX locale extensions
 */
#define XSLT_LOCALE_POSIX

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#ifdef HAVE_XLOCALE_H
#include <xlocale.h>
#endif

typedef locale_t xsltLocale;
typedef xmlChar xsltLocaleChar;

#elif defined(_WIN32) && !defined(__CYGWIN__)

/*
 * XSLT_LOCALE_WINAPI:
 * Macro indicating to use WinAPI for extended locale support
 */
#define XSLT_LOCALE_WINAPI

#include <windows.h>
#include <winnls.h>

typedef LCID xsltLocale;
typedef wchar_t xsltLocaleChar;

#else

/*
 * XSLT_LOCALE_NONE:
 * Macro indicating that there's no extended locale support
 */
#define XSLT_LOCALE_NONE

typedef void *xsltLocale;
typedef xmlChar xsltLocaleChar;

#endif

XSLTPUBFUN xsltLocale XSLTCALL
	xsltNewLocale			(const xmlChar *langName : itype(_Ptr<const xmlChar>));
XSLTPUBFUN void XSLTCALL
	xsltFreeLocale			(xsltLocale locale);
XSLTPUBFUN xsltLocaleChar *xsltStrxfrm(xsltLocale locale, const xmlChar *string : itype(_Ptr<const xmlChar>)) : itype(_Ptr<xsltLocaleChar>);
XSLTPUBFUN int XSLTCALL
	xsltLocaleStrcmp		(xsltLocale locale, const xsltLocaleChar *str1 : itype(_Ptr<const xsltLocaleChar>), const xsltLocaleChar *str2 : itype(_Ptr<const xsltLocaleChar>));
XSLTPUBFUN void XSLTCALL
	xsltFreeLocales			(void);

#endif /* __XML_XSLTLOCALE_H__ */
