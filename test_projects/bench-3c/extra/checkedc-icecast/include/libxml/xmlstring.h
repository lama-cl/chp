/*
 * Summary: set of routines to process strings
 * Description: type and interfaces needed for the internal string handling
 *              of the library, especially UTF8 processing.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_STRING_H__
#define __XML_STRING_H__

#include <stdarg.h>
#include <libxml/xmlversion.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * xmlChar:
 *
 * This is a basic byte in an UTF-8 encoded string.
 * It's unsigned allowing to pinpoint case where char * are assigned
 * to xmlChar * (possibly making serialization back impossible).
 */
typedef unsigned char xmlChar;

/**
 * BAD_CAST:
 *
 * Macro to cast a string to an xmlChar * when one know its safe.
 */
#define BAD_CAST (xmlChar *)

/*
 * xmlChar handling
 */
XMLPUBFUN xmlChar *xmlStrdup(const xmlChar *cur : itype(_Nt_array_ptr<const xmlChar>)) : itype(_Nt_array_ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlStrndup(const xmlChar *cur : itype(_Nt_array_ptr<const xmlChar>), int len) : itype(_Nt_array_ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlCharStrndup(const char *cur : itype(_Nt_array_ptr<const char>), int len) : itype(_Nt_array_ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlCharStrdup(const char *cur : itype(_Nt_array_ptr<const char>)) : itype(_Nt_array_ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlStrsub(const xmlChar *str : itype(_Nt_array_ptr<const xmlChar>), int start, int len) : itype(_Nt_array_ptr<xmlChar>);
XMLPUBFUN const xmlChar *xmlStrchr(const xmlChar *str : itype(_Nt_array_ptr<const xmlChar>), xmlChar val) : itype(_Nt_array_ptr<const xmlChar>);
XMLPUBFUN const xmlChar *xmlStrstr(const xmlChar *str : itype(_Nt_array_ptr<const xmlChar>), const xmlChar *val : itype(_Nt_array_ptr<const xmlChar>)) : itype(_Nt_array_ptr<const xmlChar>);
XMLPUBFUN const xmlChar *xmlStrcasestr(const xmlChar *str : itype(_Nt_array_ptr<const xmlChar>), const xmlChar *val : itype(_Nt_array_ptr<const xmlChar>)) : itype(_Nt_array_ptr<const xmlChar>);
XMLPUBFUN int XMLCALL
                xmlStrcmp                (const xmlChar *str1 : itype(_Nt_array_ptr<const xmlChar>), const xmlChar *str2 : itype(_Nt_array_ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
                xmlStrncmp               (const xmlChar *str1 : itype(_Nt_array_ptr<const xmlChar>), const xmlChar *str2 : itype(_Nt_array_ptr<const xmlChar>), int len);
XMLPUBFUN int XMLCALL
                xmlStrcasecmp            (const xmlChar *str1 : itype(_Nt_array_ptr<const xmlChar>), const xmlChar *str2 : itype(_Nt_array_ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
                xmlStrncasecmp           (const xmlChar *str1 : itype(_Nt_array_ptr<const xmlChar>), const xmlChar *str2 : itype(_Nt_array_ptr<const xmlChar>), int len);
XMLPUBFUN int XMLCALL
                xmlStrEqual              (const xmlChar *str1 : itype(_Nt_array_ptr<const xmlChar>), const xmlChar *str2 : itype(_Nt_array_ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
                xmlStrQEqual             (const xmlChar *pref : itype(_Nt_array_ptr<const xmlChar>), const xmlChar *name : itype(_Nt_array_ptr<const xmlChar>), const xmlChar *str : itype(_Nt_array_ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
                xmlStrlen                (const xmlChar *str : itype(_Nt_array_ptr<const xmlChar>));
XMLPUBFUN xmlChar *xmlStrcat(xmlChar *cur : itype(_Nt_array_ptr<xmlChar>), const xmlChar *add : itype(_Nt_array_ptr<const xmlChar>)) : itype(_Nt_array_ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlStrncat(xmlChar *cur : itype(_Nt_array_ptr<xmlChar>), const xmlChar *add : itype(_Nt_array_ptr<const xmlChar>), int len) : itype(_Nt_array_ptr<xmlChar>);
XMLPUBFUN xmlChar *xmlStrncatNew(const xmlChar *str1 : itype(_Nt_array_ptr<const xmlChar>), const xmlChar *str2 : itype(_Nt_array_ptr<const xmlChar>), int len) : itype(_Nt_array_ptr<xmlChar>);
XMLPUBFUN int XMLCALL
                xmlStrPrintf             (xmlChar *buf : itype(_Ptr<xmlChar>), int len, const char *msg : itype(_Ptr<const char>), ...) LIBXML_ATTR_FORMAT(3,4);
XMLPUBFUN int XMLCALL
                xmlStrVPrintf                (xmlChar *buf : itype(_Ptr<xmlChar>), int len, const char *msg : itype(_Ptr<const char>), va_list ap) LIBXML_ATTR_FORMAT(3,0);

XMLPUBFUN int XMLCALL
        xmlGetUTF8Char                   (const unsigned char *utf : itype(_Ptr<const unsigned char>), int *len : itype(_Ptr<int>));
XMLPUBFUN int XMLCALL
        xmlCheckUTF8                     (const unsigned char *utf : itype(_Ptr<const unsigned char>));
XMLPUBFUN int XMLCALL
        xmlUTF8Strsize                   (const xmlChar *utf : itype(_Ptr<const xmlChar>), int len);
XMLPUBFUN xmlChar *xmlUTF8Strndup(const xmlChar *utf : itype(_Ptr<const xmlChar>), int len) : itype(_Ptr<xmlChar>);
XMLPUBFUN const xmlChar *xmlUTF8Strpos(const xmlChar *utf : itype(_Ptr<const xmlChar>), int pos) : itype(_Ptr<const xmlChar>);
XMLPUBFUN int XMLCALL
        xmlUTF8Strloc                    (const xmlChar *utf : itype(_Ptr<const xmlChar>), const xmlChar *utfchar : itype(_Ptr<const xmlChar>));
XMLPUBFUN xmlChar *xmlUTF8Strsub(const xmlChar *utf : itype(_Ptr<const xmlChar>), int start, int len) : itype(_Ptr<xmlChar>);
XMLPUBFUN int XMLCALL
        xmlUTF8Strlen                    (const xmlChar *utf : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
        xmlUTF8Size                      (const xmlChar *utf : itype(_Ptr<const xmlChar>));
XMLPUBFUN int XMLCALL
        xmlUTF8Charcmp                   (const xmlChar *utf1 : itype(_Ptr<const xmlChar>), const xmlChar *utf2 : itype(_Ptr<const xmlChar>));

#ifdef __cplusplus
}
#endif
#endif /* __XML_STRING_H__ */
