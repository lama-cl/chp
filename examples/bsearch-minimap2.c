typedef unsigned long size_t;

__inline extern void *( __attribute__((__nonnull__(1,2,5), __gnu_inline__)) bsearch)(void const   *__key ,
                                                                                     void const   *__base ,
                                                                                     size_t __nmemb ,
                                                                                     size_t __size ,
                                                                                     int (*__compar)(void const   * ,
                                                                                                     void const   * ) ) 
{ 
  size_t __l ;
  size_t __u ;
  size_t __idx ;
  void const   *__p ;
  int __comparison ;

  {
  __l = (size_t )0;
  __u = __nmemb;
  while (__l < __u) {
    __idx = (__l + __u) / 2UL;
    __p = (void const   *)((void *)((char const   *)__base + __idx * __size));
    __comparison = (*__compar)(__key, __p);
    if (__comparison < 0) {
      __u = __idx;
    } else
    if (__comparison > 0) {
      __l = __idx + 1UL;
    } else {
      return ((void *)__p);
    }
  }
  return ((void *)0);
}
}
