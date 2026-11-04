/*
 * Part of Very Secure FTPd
 * Licence: GPL v2
 * Author: Chris Evans
 * strlist.c
 */

/* Anti-lamer measures deployed, sir! */
#define PRIVATE_HANDS_OFF_alloc_len alloc_len
#define PRIVATE_HANDS_OFF_list_len list_len
#define PRIVATE_HANDS_OFF_p_nodes p_nodes
#include "strlist.h"

#include "str.h"
#include "utility.h"
#include "sysutil.h"

#pragma CHECKED_SCOPE on
struct mystr_list_node
{
  struct mystr str;
  struct mystr sort_key_str;
};

/* File locals */
static const unsigned int kMaxStrlist = 10 * 1000 * 1000;

static struct mystr s_null_str;

static int sort_compare_func(_Ptr<const struct mystr_list_node> p1, _Ptr<const struct mystr_list_node> p2);
static int sort_compare_func_reverse(_Ptr<const struct mystr_list_node> p1,  _Ptr<const struct mystr_list_node> p2);
static int sort_compare_common(_Ptr<const struct mystr_list_node> p1, _Ptr<const struct mystr_list_node> p2, int reverse);

void
str_list_free(struct mystr_list *p_list : itype(_Ptr<struct mystr_list>))
{
  unsigned int i;
  for (i=0; i < p_list->list_len; ++i)
  {
    str_free(&p_list->p_nodes[i].str);
    str_free(&p_list->p_nodes[i].sort_key_str);
  }
  if (p_list->p_nodes)
  {
    vsf_sysutil_free<struct mystr_list_node>(p_list->p_nodes);
  }
  p_list->list_len = 0;
  p_list->alloc_len = 0, p_list->p_nodes = 0;

}

unsigned int
str_list_get_length(const struct mystr_list *p_list : itype(_Ptr<const struct mystr_list>))
{
  return p_list->list_len;
}

int
str_list_contains_str(const struct mystr_list *p_list : itype(_Ptr<const struct mystr_list>), const struct mystr *p_str : itype(_Ptr<const struct mystr>))
{
  unsigned int i;
  for (i=0; i < p_list->list_len; ++i)
  {
    if (str_equal(p_str, &p_list->p_nodes[i].str))
    {
      return 1;
    }
  }
  return 0;
}

void
str_list_add(struct mystr_list *p_list : itype(_Ptr<struct mystr_list>), const struct mystr *p_str : itype(_Ptr<const struct mystr>), const struct mystr *p_sort_key_str : itype(_Ptr<const struct mystr>))
{
  /* Expand the node allocation if we have to */
  if (p_list->list_len == p_list->alloc_len)
  {
    if (p_list->alloc_len == 0)
    {
      p_list->alloc_len = 32,
      p_list->p_nodes = vsf_sysutil_malloc<struct mystr_list_node>(
          p_list->alloc_len * (unsigned int) sizeof(struct mystr_list_node));
    }
    else
    {
      unsigned int new_len = p_list->alloc_len * 2;
      if (new_len > kMaxStrlist)
      {
        die("excessive strlist");
      }
      p_list->alloc_len = new_len,
      p_list->p_nodes = vsf_sysutil_realloc<struct mystr_list_node>(
          p_list->p_nodes,
          p_list->alloc_len * (unsigned int) sizeof(struct mystr_list_node));
    }
  }

  // Opening a scope so that the compiler doesn't think incrementing list_len
  // break bounds of nodes.
  {
    _Array_ptr<struct mystr_list_node> nodes : count(p_list->list_len + 1) = _Dynamic_bounds_cast<_Array_ptr<struct mystr_list_node>>(p_list->p_nodes, count(p_list->list_len + 1));
    nodes[p_list->list_len].str = s_null_str;
    nodes[p_list->list_len].sort_key_str = s_null_str;
    str_copy(&nodes[p_list->list_len].str, p_str);
    if (p_sort_key_str)
    {
      str_copy(&nodes[p_list->list_len].sort_key_str, p_sort_key_str);
    }
  }

  p_list->list_len++;
}

void
str_list_sort(struct mystr_list *p_list : itype(_Ptr<struct mystr_list>), int reverse)
{
  _Array_ptr<struct mystr_list_node> nodes : count(p_list->list_len) = _Dynamic_bounds_cast<_Array_ptr<struct mystr_list_node>>(p_list->p_nodes, count(p_list->list_len));
  if (!reverse)
  {
    vsf_sysutil_qsort<struct mystr_list_node>(nodes, p_list->list_len,
                      sizeof(struct mystr_list_node), sort_compare_func);
  }
  else
  {
    vsf_sysutil_qsort<struct mystr_list_node>(nodes, p_list->list_len,
                      sizeof(struct mystr_list_node),
                      &sort_compare_func_reverse);
  }
}


static int
sort_compare_func(_Ptr<const struct mystr_list_node> p1, _Ptr<const struct mystr_list_node> p2)
{
  return sort_compare_common(p1, p2, 0);
}

static int
sort_compare_func_reverse(_Ptr<const struct mystr_list_node> p1,  _Ptr<const struct mystr_list_node> p2)
{
  return sort_compare_common(p1, p2, 1);
}

static int
sort_compare_common(_Ptr<const struct mystr_list_node> p1, _Ptr<const struct mystr_list_node> p2, int reverse)
{
  _Ptr<const struct mystr> p_cmp1 = ((void *)0);
  _Ptr<const struct mystr> p_cmp2 = ((void *)0);
  _Ptr<const struct mystr_list_node> p_node1 = 0;
  _Ptr<const struct mystr_list_node> p_node2 = 0;
  _Unchecked {
    p_node1 = _Assume_bounds_cast<_Ptr<const struct mystr_list_node>>(p1);
    p_node2 = _Assume_bounds_cast<_Ptr<const struct mystr_list_node>>(p2);
  }
  if (!str_isempty(&p_node1->sort_key_str))
  {
    p_cmp1 = &p_node1->sort_key_str;
  }
  else
  {
    p_cmp1 = &p_node1->str;
  }
  if (!str_isempty(&p_node2->sort_key_str))
  {
    p_cmp2 = &p_node2->sort_key_str;
  }
  else
  {
    p_cmp2 = &p_node2->str;
  }

  if (reverse)
  {
    return str_strcmp(p_cmp2, p_cmp1);
  }
  else
  {
    return str_strcmp(p_cmp1, p_cmp2);
  }
}

const struct mystr *str_list_get_pstr(const struct mystr_list *p_list : itype(_Ptr<const struct mystr_list>), unsigned int indexx) : itype(_Ptr<const struct mystr>)
{
  if (indexx >= p_list->list_len)
  {
    bug("indexx out of range in str_list_get_str");
  }
  return &p_list->p_nodes[indexx].str;
}

