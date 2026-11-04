/* timers.c - simple timer routines
**
** Copyright © 1995,1998,2000,2014 by Jef Poskanzer <jef@mail.acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>

#include "timers.h"

#pragma CHECKED_SCOPE on

#define HASH_SIZE 67
static Timer *timers[67] : itype(_Ptr<Timer> _Checked[HASH_SIZE]) = {((void *)0)};
static Timer *free_timers : itype(_Ptr<Timer>) = ((void *)0);
static int alloc_count, active_count, free_count;

ClientData JunkClientData;



_Checked static unsigned int
hash(Timer *t : itype(_Ptr<Timer>))
    {
    /* We can hash on the trigger time, even though it can change over
    ** the life of a timer via either the periodic bit or the tmr_reset()
    ** call.  This is because both of those guys call l_resort(), which
    ** recomputes the hash and moves the timer to the appropriate list.
    */
    return (
	(unsigned int) t->time.tv_sec ^
	(unsigned int) t->time.tv_usec ) % HASH_SIZE;
    }


_Checked static void
l_add(Timer *t : itype(_Ptr<Timer>))
    {
    int h = t->hash;
    _Ptr<Timer> t2 = ((void *)0);
    _Ptr<Timer> t2prev = ((void *)0);

    t2 = timers[h];
    if ( t2 == 0 )
	{
	/* The list is empty. */
	timers[h] = t;
	t->prev = 0;
        t->next = 0;
	}
    else
	{
	if ( t->time.tv_sec < t2->time.tv_sec ||
	     ( t->time.tv_sec == t2->time.tv_sec &&
	       t->time.tv_usec <= t2->time.tv_usec ) )
	    {
	    /* The new timer goes at the head of the list. */
	    timers[h] = t;
	    t->prev = (_Ptr<Timer>) 0;
	    t->next = t2;
	    t2->prev = t;
	    }
	else
	    {
	    /* Walk the list to find the insertion point. */
	    for ( t2prev = t2, t2 = t2->next; t2 != 0;
		  t2prev = t2, t2 = t2->next )
		{
		if ( t->time.tv_sec < t2->time.tv_sec ||
		     ( t->time.tv_sec == t2->time.tv_sec &&
		       t->time.tv_usec <= t2->time.tv_usec ) )
		    {
		    /* Found it. */
		    t2prev->next = t;
		    t->prev = t2prev;
		    t->next = t2;
		    t2->prev = t;
		    return;
		    }
		}
	    /* Oops, got to the end of the list.  Add to tail. */
	    t2prev->next = t;
	    t->prev = t2prev;
	    t->next = (_Ptr<Timer>) 0;
	    }
	}
    }


_Checked static void
l_remove(Timer *t : itype(_Ptr<Timer>))
    {
    int h = t->hash;

    if ( t->prev ==  0 )
	timers[h] = t->next;
    else
	t->prev->next = t->next;
    if ( t->next !=  0 )
	t->next->prev = t->prev;
    }


_Checked static void
l_resort(Timer *t : itype(_Ptr<Timer>))
    {
    /* Remove the timer from its old list. */
    l_remove( t );
    /* Recompute the hash. */
    t->hash = hash( t );
    /* And add it back in to its new list, sorted correctly. */
    l_add( t );
    }


_Checked void
tmr_init( void )
    {
    int h;

    for ( h = 0; h < HASH_SIZE; ++h )
	timers[h] = (_Ptr<Timer>) 0;
    free_timers = (_Ptr<Timer>) 0;
    alloc_count = active_count = free_count = 0;
    }


_Checked Timer *tmr_create(struct timeval *nowP : itype(_Ptr<struct timeval>), void ((*timer_proc)(ClientData, struct timeval *)) : itype(_Ptr<void (ClientData, _Ptr<struct timeval>)>), ClientData client_data, long msecs, int periodic) : itype(_Ptr<Timer>)
    {
    _Ptr<Timer> t = ((void *)0);

    if ( free_timers != 0 )
	{
	t = free_timers;
	free_timers = t->next;
	--free_count;
	}
    else
	{
	t = (_Ptr<Timer>) malloc<Timer>( sizeof(Timer) );
	if ( t == 0 )
	    return (_Ptr<Timer>) 0;
	++alloc_count;
	}

    t->timer_proc = timer_proc;
    t->client_data = client_data;
    t->msecs = msecs;
    t->periodic = periodic;
    if ( nowP != 0 )
	t->time = *nowP;
    else {
        _Ptr<struct timeval> tv = &t->time;
        _Unchecked { (void) gettimeofday( (struct timeval *) tv, 0 ); }
    }
    t->time.tv_sec += msecs / 1000L;
    t->time.tv_usec += ( msecs % 1000L ) * 1000L;
    if ( t->time.tv_usec >= 1000000L )
	{
	t->time.tv_sec += t->time.tv_usec / 1000000L;
	t->time.tv_usec %= 1000000L;
	}
    t->hash = hash( t );
    /* Add the new timer to the proper active list. */
    l_add( t );
    ++active_count;

    return t;
    }


_Checked struct timeval *tmr_timeout(struct timeval *nowP : itype(_Ptr<struct timeval>)) : itype(_Ptr<struct timeval>)
    {
    long msecs;
    static struct timeval timeout;

    msecs = tmr_mstimeout( nowP );
    if ( msecs == INFTIM )
	return (_Ptr<struct timeval>) 0;
    timeout.tv_sec = msecs / 1000L;
    timeout.tv_usec = ( msecs % 1000L ) * 1000L;
    return &timeout;
    }


_Checked long
tmr_mstimeout(struct timeval *nowP : itype(_Ptr<struct timeval>))
    {
    int h;
    int gotone;
    long msecs, m;
    _Ptr<Timer> t = ((void *)0);

    gotone = 0;
    msecs = 0;          /* make lint happy */
    /* Since the lists are sorted, we only need to look at the
    ** first timer on each one.
    */
    for ( h = 0; h < HASH_SIZE; ++h )
	{
	t = timers[h];
	if ( t != 0 )
	    {
	    m = ( t->time.tv_sec - nowP->tv_sec ) * 1000L +
		( t->time.tv_usec - nowP->tv_usec ) / 1000L;
	    if ( ! gotone )
		{
		msecs = m;
		gotone = 1;
		}
	    else if ( m < msecs )
		msecs = m;
	    }
	}
    if ( ! gotone )
	return INFTIM;
    if ( msecs <= 0 )
	msecs = 0;
    return msecs;
    }


_Checked void
tmr_run(struct timeval *nowP : itype(_Ptr<struct timeval>))
    {
    int h;
    _Ptr<Timer> t = ((void *)0);
    _Ptr<Timer> next = ((void *)0);

    for ( h = 0; h < HASH_SIZE; ++h )
	for ( t = timers[h]; t != 0; t = next )
	    {
	    next = t->next;
	    /* Since the lists are sorted, as soon as we find a timer
	    ** that isn't ready yet, we can go on to the next list.
	    */
	    if ( t->time.tv_sec > nowP->tv_sec ||
		 ( t->time.tv_sec == nowP->tv_sec &&
		   t->time.tv_usec > nowP->tv_usec ) )
		break;
	    (t->timer_proc)( t->client_data, nowP );
	    if ( t->periodic )
		{
		/* Reschedule. */
		t->time.tv_sec += t->msecs / 1000L;
		t->time.tv_usec += ( t->msecs % 1000L ) * 1000L;
		if ( t->time.tv_usec >= 1000000L )
		    {
		    t->time.tv_sec += t->time.tv_usec / 1000000L;
		    t->time.tv_usec %= 1000000L;
		    }
		l_resort( t );
		}
	    else
		tmr_cancel( t );
	    }
    }


_Checked void
tmr_reset(struct timeval *nowP : itype(_Ptr<struct timeval>), Timer *t : itype(_Ptr<Timer>))
    {
    t->time = *nowP;
    t->time.tv_sec += t->msecs / 1000L;
    t->time.tv_usec += ( t->msecs % 1000L ) * 1000L;
    if ( t->time.tv_usec >= 1000000L )
	{
	t->time.tv_sec += t->time.tv_usec / 1000000L;
	t->time.tv_usec %= 1000000L;
	}
    l_resort( t );
    }


_Checked void
tmr_cancel(Timer *t : itype(_Ptr<Timer>))
    {
    /* Remove it from its active list. */
    l_remove( t );
    --active_count;
    /* And put it on the free list. */
    t->next = free_timers;
    free_timers = t;
    ++free_count;
    t->prev = (_Ptr<Timer>) 0;
    }


_Checked void
tmr_cleanup( void )
    {
    _Ptr<Timer> t = ((void *)0);

    while ( free_timers != 0 )
	{
	t = free_timers;
	free_timers = t->next;
	--free_count;
	free<Timer>( t );
	--alloc_count;
	}
    }


_Checked void
tmr_term( void )
    {
    int h;

    for ( h = 0; h < HASH_SIZE; ++h )
	while ( timers[h] != 0 )
	    tmr_cancel( timers[h] );
    tmr_cleanup();
    }


/* Generate debugging statistics syslog message. */
_Checked void
tmr_logstats( long secs )
    {
    syslog(
	LOG_NOTICE, "  timers - %d allocated, %d active, %d free",
	alloc_count, active_count, free_count );
    if ( active_count + free_count != alloc_count )
	syslog( LOG_ERR, "timer counts don't add up!" );
    }
