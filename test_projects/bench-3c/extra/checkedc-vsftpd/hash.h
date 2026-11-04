#ifndef VSFTP_HASH_H
#define VSFTP_HASH_H

typedef unsigned int (*hashfunc_t)(unsigned int, void*);

struct hash
{
  unsigned int buckets;
  unsigned int key_size;
  unsigned int value_size;
  hashfunc_t hash_func : itype(_Ptr<unsigned int (unsigned int, _Array_ptr<void> : byte_count(0))>);
  struct hash_node **p_nodes : itype(_Array_ptr<_Ptr<struct hash_node>>) count(buckets);
};

struct hash *hash_alloc(unsigned int buckets, unsigned int key_size, unsigned int value_size, hashfunc_t hash_func : itype(_Ptr<unsigned int(unsigned int, _Array_ptr<void> : byte_count(0))>)) : itype(_Ptr<struct hash>);
void* hash_lookup_entry(struct hash *p_hash : itype(_Ptr<struct hash>), void* p_key : itype(_Array_ptr<void>) byte_count(p_hash->key_size)) : itype(_Array_ptr<void>) byte_count(p_hash->value_size);
void hash_add_entry(struct hash *p_hash : itype(_Ptr<struct hash>), void* p_key : itype(_Array_ptr<void>) byte_count(p_hash->key_size), void* p_value : itype(_Array_ptr<void>) byte_count(p_hash->value_size));
void hash_free_entry(struct hash *p_hash : itype(_Ptr<struct hash>), void* p_key : itype(_Array_ptr<void>) byte_count(p_hash->key_size));

#endif /* VSFTP_HASH_H */

