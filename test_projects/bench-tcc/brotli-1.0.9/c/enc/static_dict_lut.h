/* Copyright 2015 Google Inc. All Rights Reserved.

   Distributed under MIT license.
   See file LICENSE for detail or copy at https://opensource.org/licenses/MIT
*/

/* Lookup table for static dictionary and transforms. */

#ifndef BROTLI_ENC_STATIC_DICT_LUT_H_
#define BROTLI_ENC_STATIC_DICT_LUT_H_

#include <brotli/types.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct DictWord {
  /* Highest bit is used to indicate end of bucket. */
  uint8_t len;
  uint8_t transform;
  uint16_t idx;
} DictWord;

static const int kDictNumBits = 15;
static const uint32_t kDictHashMul32 = 0x1E35A7BD;

static const uint16_t kStaticDictionaryBuckets[32768] = {
};

static const DictWord kStaticDictionaryWords[31705] = {
};

#if defined(__cplusplus) || defined(c_plusplus)
}  /* extern "C" */
#endif

#endif  /* BROTLI_ENC_STATIC_DICT_LUT_H_ */
