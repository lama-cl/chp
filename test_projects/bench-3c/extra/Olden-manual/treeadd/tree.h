/* For copyright information, see olden_v1.0/COPYRIGHT */

/* tree.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdchecked.h>

#pragma CHECKED_SCOPE ON

#ifdef TORONTO
#define chatting(...) _Unchecked { printf(__VA_ARGS__); }
#define PLAIN
#endif

typedef struct tree {
    int		val;
    ptr<struct tree> left;
    ptr<struct tree> right;
} tree_t;

extern ptr<tree_t> TreeAlloc(int level, int lo, int hi);
int TreeAdd (ptr<tree_t> t);






#pragma CHECKED_SCOPE OFF
