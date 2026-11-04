#ifndef VSF_SECBUF_H
#define VSF_SECBUF_H

struct secbuf {
  char *p_ptr : itype(_Array_ptr<char>) count(size);
  char *noaccess_page : itype(_Array_ptr<char>) bounds(p_ptr - map_offset, p_ptr);
  unsigned int size;
  unsigned int map_offset;
};

// struct secbuf contains extra informations that's needed to properly
// free the buffer, but if the buffer is stored in a static variable
// that is never deallocated, then this can be used for a simpler interface.
#define vsf_secbuf_static_alloc(P, S) { \
  _Ptr<struct secbuf> __tmp_secbuf = &(struct secbuf){0, 0, (S), 0}; \
  vsf_secbuf_alloc(__tmp_secbuf); \
  P = __tmp_secbuf->p_ptr;}

/* vsf_secbuf_alloc()
 * PURPOSE
 * Allocate a "secure buffer". A secure buffer is one which will attempt to
 * catch out of bounds accesses by crashing the program (rather than
 * corrupting memory). It works by using UNIX memory protection. It isn't
 * foolproof.
 * PARAMETERS
 * p_ptr        - pointer to a pointer which is to contain the secure buffer.
 *                Any previous buffer pointed to is freed.
 * size         - size in bytes required for the secure buffer.
 */
void vsf_secbuf_alloc(struct secbuf *buf : itype(_Ptr<struct secbuf>));

/* vsf_secbuf_free()
 * PURPOSE
 * Frees a "secure buffer".
 * PARAMETERS
 * p_ptr        - pointer to a pointer containing the buffer to be freed. The
 *                buffer pointer is nullified by this call.
 */
void vsf_secbuf_free(struct secbuf *buf : itype(_Ptr<struct secbuf>));

#endif /* VSF_SECBUF_H */

