/*
 * Part of Very Secure FTPd
 * Licence: GPL v2
 * Author: Chris Evans
 * hash.c
 *
 * Routines to handle simple hash table lookups and modifications.
 */

#include "hash.h"
#include "sysutil.h"
#include "utility.h"

#pragma CHECKED_SCOPE on

struct hash_node
{
  _Array_ptr<void> p_key : byte_count(key_size);
  _Array_ptr<void> p_value : byte_count(value_size);
  _Ptr<struct hash_node> p_prev;
  _Ptr<struct hash_node> p_next;

  unsigned int key_size;
  unsigned int value_size;
};

/* Internal functions */
struct hash_node **hash_get_bucket(struct hash *p_hash : itype(_Ptr<struct hash>), void* p_key : byte_count(p_hash->key_size)) : itype(_Ptr<_Ptr<struct hash_node>>);
struct hash_node *hash_get_node_by_key(struct hash *p_hash : itype(_Ptr<struct hash>), void* p_key : byte_count(p_hash->key_size)) : itype(_Ptr<struct hash_node>);

struct hash *hash_alloc(unsigned int buckets, unsigned int key_size, unsigned int value_size, hashfunc_t hash_func : itype(_Ptr<unsigned int (unsigned int, _Array_ptr<void> : byte_count(0))>)) : itype(_Ptr<struct hash>)
{
  _Ptr<struct hash> p_hash = vsf_sysutil_malloc<struct hash>(sizeof(*p_hash));
  p_hash->key_size = key_size;
  p_hash->value_size = value_size;
  p_hash->hash_func = hash_func;
  _Array_ptr<_Ptr<struct hash_node>> tmp : count(buckets) = vsf_sysutil_malloc<_Ptr<struct hash_node>>(((unsigned int )sizeof(_Ptr<struct hash_node>)) * buckets);
  p_hash->buckets = buckets, p_hash->p_nodes = _Dynamic_bounds_cast<_Array_ptr<_Ptr<struct hash_node>>>(tmp, count(p_hash->buckets));
  vsf_sysutil_memclr<_Ptr<struct hash_node>>(p_hash->p_nodes, ((unsigned int)sizeof(_Ptr<struct hash_node>)) * p_hash->buckets);
  return p_hash;
}

void*
hash_lookup_entry(struct hash *p_hash : itype(_Ptr<struct hash>), void* p_key : itype(_Array_ptr<void>) byte_count(p_hash->key_size)) : itype(_Array_ptr<void>) byte_count(p_hash->value_size)
{
  _Ptr<struct hash_node> p_node = hash_get_node_by_key(p_hash, p_key);
  if (!p_node)
  {
    return 0;
  }
  return _Dynamic_bounds_cast<_Array_ptr<void>>(p_node->p_value, byte_count(p_hash->value_size));
}

void
hash_add_entry(struct hash *p_hash : itype(_Ptr<struct hash>), void* p_key : itype(_Array_ptr<void>) byte_count(p_hash->key_size), void* p_value : itype(_Array_ptr<void>) byte_count(p_hash->value_size))
{
  _Ptr<_Ptr<struct hash_node>> p_bucket = ((void *)0);
  _Ptr<struct hash_node> p_new_node = 0;
  if (hash_lookup_entry(p_hash, p_key))
  {
    bug("duplicate hash key");
  }
  p_bucket = hash_get_bucket(p_hash, p_key);
  p_new_node = vsf_sysutil_malloc<struct hash_node>(sizeof(*p_new_node));
  p_new_node->p_prev = 0;
  p_new_node->p_next = 0;

  _Array_ptr<void> new_key : byte_count(p_hash->key_size) = vsf_sysutil_malloc<void>(p_hash->key_size);
  vsf_sysutil_memcpy<void>(new_key, p_key, p_hash->key_size);
  p_new_node->key_size = p_hash->key_size, p_new_node->p_key = new_key;

  _Array_ptr<void> new_value : byte_count(p_hash->value_size) = vsf_sysutil_malloc<void>(p_hash->value_size);
  vsf_sysutil_memcpy<void>(new_value, p_value, p_hash->value_size);
  p_new_node->value_size = p_hash->value_size, p_new_node->p_value = new_value;

  if (!*p_bucket)
  {
    *p_bucket = p_new_node;    
  }
  else
  {
    p_new_node->p_next = *p_bucket;
    (*p_bucket)->p_prev = p_new_node;
    *p_bucket = p_new_node;
  }
}

void
hash_free_entry(struct hash *p_hash : itype(_Ptr<struct hash>), void* p_key : itype(_Array_ptr<void>) byte_count(p_hash->key_size))
{
  _Ptr<struct hash_node> p_node = hash_get_node_by_key(p_hash, p_key);
  if (!p_node)
  {
    bug("hash node not found");
  }
  vsf_sysutil_free<void>(p_node->p_key);
  vsf_sysutil_free<void>(p_node->p_value);

  if (p_node->p_prev)
  {
    p_node->p_prev->p_next = p_node->p_next;
  }
  else
  {
    _Ptr<_Ptr<struct hash_node>> p_bucket = hash_get_bucket(p_hash, p_key);
    *p_bucket = p_node->p_next;
  }
  if (p_node->p_next)
  {
    p_node->p_next->p_prev = p_node->p_prev;
  }

  vsf_sysutil_free<struct hash_node>(p_node);
}

struct hash_node **hash_get_bucket(struct hash *p_hash : itype(_Ptr<struct hash>), void* p_key : byte_count(p_hash->key_size)) : itype(_Ptr<_Ptr<struct hash_node>>)
{
  _Ptr<void> unsafe_key = 0;
  _Unchecked {
    unsafe_key = _Assume_bounds_cast<_Ptr<void>>(p_key);
  }
  unsigned int bucket = (*p_hash->hash_func)(p_hash->buckets, unsafe_key);
  if (bucket >= p_hash->buckets)
  {
    bug("bad bucket lookup");
  }
  return &(p_hash->p_nodes[bucket]);
}

struct hash_node *hash_get_node_by_key(struct hash *p_hash : itype(_Ptr<struct hash>), void* p_key : byte_count(p_hash->key_size)) : itype(_Ptr<struct hash_node>)
{
  _Ptr<_Ptr<struct hash_node>> p_bucket = hash_get_bucket(p_hash, p_key);
  _Ptr<struct hash_node> p_node = *p_bucket;
  if (!p_node)
  {
    return p_node;
  }
  while (p_node != 0 &&
      ({ _Array_ptr<void> tmp : byte_count(p_hash->key_size) = _Dynamic_bounds_cast<_Array_ptr<void>>(p_node->p_key, byte_count(p_hash->key_size)); vsf_sysutil_memcmp<void>(p_key, tmp , p_hash->key_size); }) != 0)
  {
    p_node = p_node->p_next;
  }
  return p_node;
}

