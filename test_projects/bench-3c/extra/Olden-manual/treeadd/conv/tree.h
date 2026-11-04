/* For copyright information, see olden_v1.0/COPYRIGHT */

/* tree.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#ifdef TORONTO
#define chatting(...)  { printf(__VA_ARGS__); }
#define PLAIN
#endif

typedef struct tree {
    int		val;
    struct tree * left;
    struct tree * right;
} tree_t;

extern tree_t * TreeAlloc(int level, int lo, int hi);
int TreeAdd (tree_t * t);






