#ifndef VSF_IPADDRPARSE_H
#define VSF_IPADDRPARSE_H

struct mystr;

/* Effectively doing the same sort of job as inet_pton. Since inet_pton does
 * a non-trivial amount of parsing, we'll do it ourselves for maximum security
 * and safety.
 */

#include<netinet/in.h>

_Array_ptr<const unsigned char> vsf_sysutil_parse_ipv6(_Ptr<const struct mystr> p_str) : byte_count(sizeof(struct in6_addr));

_Array_ptr<const unsigned char> vsf_sysutil_parse_ipv4(_Ptr<const struct mystr> p_str) : byte_count(sizeof(struct in_addr));

_Array_ptr<const unsigned char> vsf_sysutil_parse_uchar_string_sep(_Ptr<const struct mystr> p_str, char sep, _Array_ptr<unsigned char> p_items : count(items), unsigned int items) : count(items);

#endif /* VSF_IPADDRPARSE_H */

