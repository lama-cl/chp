/* Icecast
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright 2000-2004, Jack Moffitt <jack@xiph.org, 
 *                      Michael Smith <msmith@xiph.org>,
 *                      oddsock <oddsock@xiph.org>,
 *                      Karl Heyes <karl@xiph.org>
 *                      and others (see AUTHORS for details).
 */

/* refbuf.c
**
** reference counting buffer implementation
**
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "refbuf.h"

#define CATMODULE "refbuf"

#include "logging.h"

#pragma CHECKED_SCOPE on

void refbuf_initialize(void)
{
}

void refbuf_shutdown(void)
{
}

void refbuf_widen(_Ptr<refbuf_t> r) { 
  int len = strlen(r->data);
  r->data = _Dynamic_bounds_cast<_Nt_array_ptr<char>>(r->data, count(len)), r->len = len;
}

refbuf_t *refbuf_new(unsigned int size) : itype(_Ptr<refbuf_t>)
{
    _Ptr<refbuf_t> refbuf = ((void *)0);

    refbuf = malloc<refbuf_t>(sizeof(refbuf_t));
    if (refbuf == NULL)
        abort();
    refbuf->data = NULL;
    if (size)
    {
      _Array_ptr<char> raw : count(sizeof(char) * size) = calloc<char>(sizeof(char), size); 
      raw[size-1] = '\0';
      _Unchecked { 
        refbuf->len = size;
        refbuf->data = _Assume_bounds_cast<_Nt_array_ptr<char>>(raw, count(sizeof(char) * (size - 1))), refbuf->total_length = size;
      }
        if (refbuf->data == NULL)
            abort();
    }
    refbuf->sync_point = 0;
    refbuf->_count = 1;
    refbuf->next = NULL;
    refbuf->associated = NULL;

    return refbuf;
}

void refbuf_addref(refbuf_t *self : itype(_Ptr<refbuf_t>))
{
    self->_count++;
}

static void refbuf_release_associated (_Ptr<refbuf_t> ref)
{
    if (ref == NULL)
        return;
    while (ref)
    {
        _Ptr<refbuf_t> to_go = ref;
        ref = to_go->next;
        if ( to_go->_count == 1 )
	    to_go->next = NULL;
        refbuf_release (to_go);
    }
}

void refbuf_release(refbuf_t *self : itype(_Ptr<refbuf_t>))
{
    if (self == NULL)
        return;
    self->_count--;
    if (self->_count == 0)
    {
        refbuf_release_associated (self->associated);
        if (self->next) _Unchecked {
            ICECAST_LOG_ERROR("next not null");
        }
        free<char>(self->data);
        free<refbuf_t>(self);
    }
}

