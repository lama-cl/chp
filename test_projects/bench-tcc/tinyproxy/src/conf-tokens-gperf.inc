/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: /usr/bin/gperf conf-tokens.gperf  */
/* Computed positions: -k'2,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 1 "conf-tokens.gperf"

#include <string.h>
#include <stdlib.h>
#include "conf-tokens.h"

#define CDS_TOTAL_KEYWORDS 40
#define CDS_MIN_WORD_LENGTH 4
#define CDS_MAX_WORD_LENGTH 19
#define CDS_MIN_HASH_VALUE 5
#define CDS_MAX_HASH_VALUE 84
/* maximum key range = 80, duplicates = 0 */

#ifndef GPERF_DOWNCASE
#define GPERF_DOWNCASE 1
static unsigned char gperf_downcase[256] =
  {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
     30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
     45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
     60,  61,  62,  63,  64,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
    255
  };
#endif

#ifndef GPERF_CASE_MEMCMP
#define GPERF_CASE_MEMCMP 1
static int
gperf_case_memcmp (register const char *s1, register const char *s2, register size_t n)
{
  for (; n > 0;)
    {
      unsigned char c1 = gperf_downcase[(unsigned char)*s1++];
      unsigned char c2 = gperf_downcase[(unsigned char)*s2++];
      if (c1 == c2)
        {
          n--;
          continue;
        }
      return (int)c1 - (int)c2;
    }
  return 0;
}
#endif

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 25, 85, 15, 40,  5,
      85, 15, 30,  5, 85, 85,  0, 15,  0, 50,
       0, 85, 20, 15,  0, 85, 85,  0, 85, 35,
      85, 85, 85, 85, 85, 85, 85, 25, 85, 15,
      40,  5, 85, 15, 30,  5, 85, 85,  0, 15,
       0, 50,  0, 85, 20, 15,  0, 85, 85,  0,
      85, 35, 85, 85, 85, 85, 85, 85
    };
  return len + asso_values[(unsigned char)str[1]] + asso_values[(unsigned char)str[len - 1]];
}

const struct config_directive_entry *
config_directive_find (register const char *str, register size_t len)
{
  static const unsigned char lengthtable[] =
    {
       0,  0,  0,  0,  0,  5,  0,  0,  8,  0,  0,  6,  7,  8,
       0,  0,  0,  7,  8, 14, 10,  0, 12,  8,  9,  5, 16, 12,
       0, 19, 10,  6, 12,  0,  9, 15,  0,  0,  0,  4,  0, 16,
       0,  0,  4, 10, 11,  0,  0,  4, 10, 11,  0,  0,  4, 15,
       6, 17,  8, 14,  0, 11,  7,  0,  9,  0,  0,  0,  0,  9,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      19
    };
  static const struct config_directive_entry wordlist[] =
    {
      {"",CD_NIL}, {"",CD_NIL}, {"",CD_NIL}, {"",CD_NIL},
      {"",CD_NIL},
#line 43 "conf-tokens.gperf"
      {"allow", CD_allow},
      {"",CD_NIL}, {"",CD_NIL},
#line 27 "conf-tokens.gperf"
      {"stathost", CD_stathost},
      {"",CD_NIL}, {"",CD_NIL},
#line 42 "conf-tokens.gperf"
      {"listen", CD_listen},
#line 38 "conf-tokens.gperf"
      {"timeout", CD_timeout},
#line 26 "conf-tokens.gperf"
      {"statfile", CD_statfile},
      {"",CD_NIL}, {"",CD_NIL}, {"",CD_NIL},
#line 22 "conf-tokens.gperf"
      {"pidfile", CD_pidfile},
#line 30 "conf-tokens.gperf"
      {"bindsame", CD_bindsame},
#line 55 "conf-tokens.gperf"
      {"reversebaseurl", CD_reversebaseurl},
#line 54 "conf-tokens.gperf"
      {"filtertype", CD_filtertype},
      {"",CD_NIL},
#line 24 "conf-tokens.gperf"
      {"viaproxyname", CD_viaproxyname},
#line 59 "conf-tokens.gperf"
      {"upstream", CD_upstream},
#line 23 "conf-tokens.gperf"
      {"anonymous", CD_anonymous},
#line 41 "conf-tokens.gperf"
      {"group", CD_group},
#line 25 "conf-tokens.gperf"
      {"defaulterrorfile", CD_defaulterrorfile},
#line 36 "conf-tokens.gperf"
      {"startservers", CD_startservers},
      {"",CD_NIL},
#line 53 "conf-tokens.gperf"
      {"filtercasesensitive", CD_filtercasesensitive},
#line 50 "conf-tokens.gperf"
      {"filterurls", CD_filterurls},
#line 49 "conf-tokens.gperf"
      {"filter", CD_filter},
#line 57 "conf-tokens.gperf"
      {"reversemagic", CD_reversemagic},
      {"",CD_NIL},
#line 47 "conf-tokens.gperf"
      {"errorfile", CD_errorfile},
#line 35 "conf-tokens.gperf"
      {"minspareservers", CD_minspareservers},
      {"",CD_NIL}, {"",CD_NIL}, {"",CD_NIL},
#line 40 "conf-tokens.gperf"
      {"user", CD_user},
      {"",CD_NIL},
#line 31 "conf-tokens.gperf"
      {"disableviaheader", CD_disableviaheader},
      {"",CD_NIL}, {"",CD_NIL},
#line 44 "conf-tokens.gperf"
      {"deny", CD_deny},
#line 28 "conf-tokens.gperf"
      {"xtinyproxy", CD_xtinyproxy},
#line 58 "conf-tokens.gperf"
      {"reversepath", CD_reversepath},
      {"",CD_NIL}, {"",CD_NIL},
#line 45 "conf-tokens.gperf"
      {"bind", CD_bind},
#line 33 "conf-tokens.gperf"
      {"maxclients", CD_maxclients},
#line 56 "conf-tokens.gperf"
      {"reverseonly", CD_reverseonly},
      {"",CD_NIL}, {"",CD_NIL},
#line 32 "conf-tokens.gperf"
      {"port", CD_port},
#line 34 "conf-tokens.gperf"
      {"maxspareservers", CD_maxspareservers},
#line 29 "conf-tokens.gperf"
      {"syslog", CD_syslog},
#line 52 "conf-tokens.gperf"
      {"filterdefaultdeny", CD_filterdefaultdeny},
#line 60 "conf-tokens.gperf"
      {"loglevel", CD_loglevel},
#line 51 "conf-tokens.gperf"
      {"filterextended", CD_filterextended},
      {"",CD_NIL},
#line 39 "conf-tokens.gperf"
      {"connectport", CD_connectport},
#line 21 "conf-tokens.gperf"
      {"logfile", CD_logfile},
      {"",CD_NIL},
#line 46 "conf-tokens.gperf"
      {"basicauth", CD_basicauth},
      {"",CD_NIL}, {"",CD_NIL}, {"",CD_NIL}, {"",CD_NIL},
#line 48 "conf-tokens.gperf"
      {"addheader", CD_addheader},
      {"",CD_NIL}, {"",CD_NIL}, {"",CD_NIL}, {"",CD_NIL},
      {"",CD_NIL}, {"",CD_NIL}, {"",CD_NIL}, {"",CD_NIL},
      {"",CD_NIL}, {"",CD_NIL}, {"",CD_NIL}, {"",CD_NIL},
      {"",CD_NIL}, {"",CD_NIL},
#line 37 "conf-tokens.gperf"
      {"maxrequestsperchild", CD_maxrequestsperchild}
    };

  if (len <= CDS_MAX_WORD_LENGTH && len >= CDS_MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= CDS_MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            register const char *s = wordlist[key].name;

            if ((((unsigned char)*str ^ (unsigned char)*s) & ~32) == 0 && !gperf_case_memcmp (str, s, len))
              return &wordlist[key];
          }
    }
  return 0;
}
#line 61 "conf-tokens.gperf"


