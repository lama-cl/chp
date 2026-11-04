#ifndef VSF_STRLIST_H
#define VSF_STRLIST_H

/* Forward declarations */
struct mystr;
struct mystr_list_node;

struct mystr_list
{
  unsigned int PRIVATE_HANDS_OFF_alloc_len;
  unsigned int PRIVATE_HANDS_OFF_list_len;
  struct mystr_list_node* PRIVATE_HANDS_OFF_p_nodes : itype(_Array_ptr<struct mystr_list_node>) count(PRIVATE_HANDS_OFF_alloc_len);
};

#define INIT_STRLIST \
  { 0, 0, (void*)0 }

void str_list_free(struct mystr_list *p_list : itype(_Ptr<struct mystr_list>));

void str_list_add(struct mystr_list *p_list : itype(_Ptr<struct mystr_list>), const struct mystr *p_str : itype(_Ptr<const struct mystr>), const struct mystr *p_sort_key_str : itype(_Ptr<const struct mystr>));
void str_list_sort(struct mystr_list *p_list : itype(_Ptr<struct mystr_list>), int reverse);

unsigned int str_list_get_length(const struct mystr_list *p_list : itype(_Ptr<const struct mystr_list>));
int str_list_contains_str(const struct mystr_list *p_list : itype(_Ptr<const struct mystr_list>), const struct mystr *p_str : itype(_Ptr<const struct mystr>));

const struct mystr *str_list_get_pstr(const struct mystr_list *p_list : itype(_Ptr<const struct mystr_list>), unsigned int indexx) : itype(_Ptr<const struct mystr>);

#endif /* VSF_STRLIST_H */

