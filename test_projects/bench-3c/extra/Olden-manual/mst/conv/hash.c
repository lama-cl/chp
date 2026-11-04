/* For copyright information, see olden_v1.0/COPYRIGHT */


#include <stdlib.h>
#include "hash.h"

#define printf(...)  { printf(__VA_ARGS__); }
#define assert(num,a) if (!(a)) {printf("Assertion failure:%d in hash\n",num); exit(-1);}

static int remaining = 0;
static char * temp ;

static void * localmalloc(int size) 
{
  void * blah;
  
  if (size>remaining) 
    {
      remaining = 32768;
      temp = calloc(remaining, sizeof(char));
      if (!temp) printf("Error! malloc returns null\n");
    }
  blah = temp;
  temp += size;
  remaining -= size;
  return blah;
}

#define localfree(sz)

Hash MakeHash(int size, int(*map)(unsigned int))
{
  Hash retval = NULL;
  int i;

  retval = (Hash) localmalloc(sizeof(*retval));
  retval->array = (HashEntry *)localmalloc(size*sizeof(HashEntry));
  retval->size = size;
  for (i=0; i<size; i++)
    retval->array[i] = NULL;
  retval->mapfunc = map;
  return retval;
}

 void *HashLookup(unsigned int key, Hash hash)
{
  int j;
  HashEntry ent = NULL;

  j = (hash->mapfunc)(key);        /* 14% miss in hash->mapfunc */  
  assert(1,j>=0);
  assert(2,j<hash->size);
  for (ent = hash->array[j];       /* 17% miss in hash->array[j] */ /* adt_pf can't detect :( */
       ent &&                      /* 47% miss in ent->key */       /* adt_pf can detect :) */
           ent->key!=key; 
       ent=ent->next);             /* 8% miss in ent->next */       /* adt_pf can detect :) */
  if (ent) return ent->entry;
  return NULL;
}

 void HashInsert(void *entry,unsigned int key,Hash hash)
{
  HashEntry ent = NULL;
  int j;
  
  assert(3,!HashLookup(key,hash));
  
  j = (hash->mapfunc)(key);
  ent = (HashEntry)localmalloc(sizeof(*ent));
  ent->next = hash->array[j];
  hash->array[j]=ent;
  ent->key = key;
  ent->entry = entry;
}

void HashDelete(unsigned key, Hash hash) {
  HashEntry tmp = NULL;
  int j = (hash->mapfunc)(key);
  int size = hash->size;
  ;
  HashEntry * ent = 0; 
   { ent = &hash->array[j]; }

  while (*ent && (*ent)->key != key) {
    ent = &(*ent)->next;
  }

  assert(4, *ent);

  tmp = *ent;
  *ent = (*ent)->next;
  localfree(tmp);
}

  
  


