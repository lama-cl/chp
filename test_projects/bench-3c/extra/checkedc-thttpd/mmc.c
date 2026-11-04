/* mmc.c - mmap cache
**
** Copyright © 1998,2001,2014 by Jef Poskanzer <jef@mail.acme.com>.
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

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif /* HAVE_MMAP */

#include "mmc.h"
#include "libhttpd.h"

#ifndef HAVE_INT64T
typedef long long int64_t;
#endif


/* Defines. */
#ifndef DEFAULT_EXPIRE_AGE
#define DEFAULT_EXPIRE_AGE 600
#endif
#ifndef DESIRED_FREE_COUNT
#define DESIRED_FREE_COUNT 100
#endif
#ifndef DESIRED_MAX_MAPPED_FILES
#define DESIRED_MAX_MAPPED_FILES 2000
#endif
#ifndef DESIRED_MAX_MAPPED_BYTES
#define DESIRED_MAX_MAPPED_BYTES 1000000000
#endif
#ifndef INITIAL_HASH_SIZE
#define INITIAL_HASH_SIZE (1 << 10)
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#pragma CHECKED_SCOPE on

/* The Map struct. */
typedef struct MapStruct {
    ino_t ino;
    dev_t dev;
    off_t size;
    time_t ct;
    int refcount;
    time_t reftime;
    void* addr : itype(_Ptr<void>);
    unsigned int hash;
    int hash_idx;
    struct MapStruct *next : itype(_Ptr<struct MapStruct>);
    } Map;


/* Globals. */
static Map *maps : itype(_Ptr<Map>) = (_Ptr<Map>) 0;
static Map *free_maps : itype(_Ptr<Map>) = (_Ptr<Map>) 0;
static int alloc_count = 0, map_count = 0, free_count = 0;
static size_t hash_size;
static Map **hash_table : itype(_Array_ptr<_Ptr<Map>>) count(hash_size) = (_Array_ptr<_Ptr<Map>>) 0;
static unsigned int hash_mask;
static time_t expire_age = DEFAULT_EXPIRE_AGE;
static off_t mapped_bytes = 0;



/* Forwards. */
static void panic( void );
static void really_unmap(Map **mm : itype(_Ptr<_Ptr<Map>>));
static int check_hash_size( void );
static int add_hash(Map *m : itype(_Ptr<Map>));
static Map *find_hash(ino_t ino, dev_t dev, off_t size, time_t ct) : itype(_Ptr<Map>);
static unsigned int hash( ino_t ino, dev_t dev, off_t size, time_t ct );


_Checked _Itype_for_any(T) void*
mmc_map(char *filename : itype(_Nt_array_ptr<char>), struct stat *sbP : itype(_Ptr<struct stat>), struct timeval *nowP : itype(_Ptr<struct timeval>)) : itype(_Array_ptr<T>)
    {
    time_t now;
    struct stat sb;
    _Ptr<Map> m = ((void *)0);
    int fd;

    /* Stat the file, if necessary. */
    if ( sbP !=  0 )
	sb = *sbP;
    else
	{
	if ( stat( filename, &sb ) != 0 )
	    {
	    syslog( LOG_ERR, "stat - %m" );
	    return (void*) 0;
	    }
	}

    /* Get the current time, if necessary. */
    if ( nowP !=  0 )
	now = nowP->tv_sec;
    else
	now = time(  0 );

    /* See if we have it mapped already, via the hash table. */
    if ( check_hash_size() < 0 )
	{
	syslog( LOG_ERR, "check_hash_size() failure" );
	return (void*) 0;
	}
    m = find_hash( sb.st_ino, sb.st_dev, sb.st_size, sb.st_ctime );
    if ( m !=  0 )
	{
	/* Yep.  Just return the existing map */
	++m->refcount;
	m->reftime = now;
	_Unchecked { return _Assume_bounds_cast<_Ptr<T>>(m->addr); }
	}

    /* Open the file. */
    _Unchecked { fd = open( filename, O_RDONLY ); }
    if ( fd < 0 )
	{
	syslog( LOG_ERR, "open - %m" );
	return (void*) 0;
	}

    /* Find a free Map entry or make a new one. */
    if ( free_maps !=  0 )
	{
	m = free_maps;
	free_maps = m->next;
	--free_count;
	}
    else
	{
	m = (_Ptr<Map>) malloc<Map>( sizeof(Map) );
	if ( m ==  0 )
	    {
	    (void) close( fd );
	    syslog( LOG_ERR, "out of memory allocating a Map" );
	    return (void*) 0;
	    }
	++alloc_count;
	}

    /* Fill in the Map entry. */
    m->ino = sb.st_ino;
    m->dev = sb.st_dev;
    m->size = sb.st_size;
    m->ct = sb.st_ctime;
    m->refcount = 1;
    m->reftime = now;

    /* Avoid doing anything for zero-length files; some systems don't like
    ** to mmap them, other systems dislike mallocing zero bytes.
    */
    if ( m->size == 0 )
	_Unchecked { m->addr = _Assume_bounds_cast<_Ptr<void>>(1); }	/* arbitrary non-NULL address */
    else
	{
	size_t size_size = (size_t) m->size;	/* loses on files >2GB */
#ifdef HAVE_MMAP
	/* Map the file into memory. */
        _Unchecked {
	  m->addr = mmap( 0, size_size, PROT_READ, MAP_PRIVATE, fd, 0 );
        }
	if ( (int) m->addr ==  -1 && errno == ENOMEM )
	    {
	    /* Ooo, out of address space.  Free all unreferenced maps
	    ** and try again.
	    */
	    panic();
            _Unchecked {
	      m->addr = mmap( 0, size_size, PROT_READ, MAP_PRIVATE, fd, 0 );
            }
	    }
	if ( (int) m->addr ==  -1 )
	    {
	    syslog( LOG_ERR, "mmap - %m" );
	    (void) close( fd );
	    free<Map>( m );
	    --alloc_count;
	    return (void*) 0;
	    }
#else /* HAVE_MMAP */
	/* Read the file into memory. */
	m->addr = (void*) malloc( size_size );
	if ( m->addr == (void*) 0 )
	    {
	    /* Ooo, out of memory.  Free all unreferenced maps
	    ** and try again.
	    */
	    panic();
	    m->addr = (void*) malloc( size_size );
	    }
	if ( m->addr == (void*) 0 )
	    {
	    syslog( LOG_ERR, "out of memory storing a file" );
	    (void) close( fd );
	    free( m );
	    --alloc_count;
	    return (void*) 0;
	    }
	if ( httpd_read_fully( fd, m->addr, size_size ) != size_size )
	    {
	    syslog( LOG_ERR, "read - %m" );
	    (void) close( fd );
	    free( m );
	    --alloc_count;
	    return (void*) 0;
	    }
#endif /* HAVE_MMAP */
	}
    (void) close( fd );

    /* Put the Map into the hash table. */
    if ( add_hash( m ) < 0 )
	{
	syslog( LOG_ERR, "add_hash() failure" );
	free<Map>( m );
	--alloc_count;
	return (void*) 0;
	}

    /* Put the Map on the active list. */
    m->next = maps;
    maps = m;
    ++map_count;

    /* Update the total byte count. */
    mapped_bytes += m->size;

    /* And return the address. */
    _Unchecked { return _Assume_bounds_cast<_Ptr<T>>(m->addr); }
    }


_Itype_for_any(T) void mmc_unmap(void* addr : itype(_Array_ptr<T>), struct stat *sbP : itype(_Ptr<struct stat>), struct timeval *nowP : itype(_Ptr<struct timeval>))
    _Unchecked {
    _Ptr<Map> m = (_Ptr<Map>) 0;

    /* Find the Map entry for this address.  First try a hash. */
    if ( sbP != (struct stat*) 0 )
	{
	m = find_hash( sbP->st_ino, sbP->st_dev, sbP->st_size, sbP->st_ctime );
	if ( m != (Map*) 0 && m->addr != addr )
	    m = (_Ptr<Map>) 0;
	}
    /* If that didn't work, try a full search. */
    if ( m == (Map*) 0 )
	for ( m = maps; m != (Map*) 0; m = m->next )
	    if ( m->addr == addr )
		break;
    if ( m == (Map*) 0 )
	syslog( LOG_ERR, "mmc_unmap failed to find entry!" );
    else if ( m->refcount <= 0 )
	syslog( LOG_ERR, "mmc_unmap found zero or negative refcount!" );
    else
	{
	--m->refcount;
	if ( nowP != (struct timeval*) 0 )
	    m->reftime = nowP->tv_sec;
	else
	    m->reftime = time( (time_t*) 0 );
	}
    }


_Checked void
mmc_cleanup(struct timeval *nowP : itype(_Ptr<struct timeval>))
    {
    time_t now;
    _Ptr<_Ptr<Map>> mm = ((void *)0);
    _Ptr<Map> m = ((void *)0);

    /* Get the current time, if necessary. */
    if ( nowP !=  0 )
	now = nowP->tv_sec;
    else
	now = time(  0 );

    /* Really unmap any unreferenced entries older than the age limit. */
    for ( mm = &maps; *mm !=  0; )
	{
	m = *mm;
	if ( m->refcount == 0 && now - m->reftime >= expire_age )
	    really_unmap( mm );
	else
	    mm = &(*mm)->next;
	}

    /* Adjust the age limit if there are too many bytes mapped, or
    ** too many or too few files mapped.
    */
    if ( mapped_bytes > DESIRED_MAX_MAPPED_BYTES )
	expire_age = MAX( ( expire_age * 2 ) / 3, DEFAULT_EXPIRE_AGE / 10 );
    else if ( map_count > DESIRED_MAX_MAPPED_FILES )
	expire_age = MAX( ( expire_age * 2 ) / 3, DEFAULT_EXPIRE_AGE / 10 );
    else if ( map_count < DESIRED_MAX_MAPPED_FILES / 2 )
	expire_age = MIN( ( expire_age * 5 ) / 4, DEFAULT_EXPIRE_AGE * 3 );

    /* Really free excess blocks on the free list. */
    while ( free_count > DESIRED_FREE_COUNT )
	{
	m = free_maps;
	free_maps = m->next;
	--free_count;
	free<Map>( m );
	--alloc_count;
	}
    }


_Checked static void
panic( void )
    {
    _Ptr<_Ptr<Map>> mm = ((void *)0);
    _Ptr<Map> m = ((void *)0);

    syslog( LOG_ERR, "mmc panic - freeing all unreferenced maps" );

    /* Really unmap all unreferenced entries. */
    for ( mm = &maps; *mm !=  0; )
	{
	m = *mm;
	if ( m->refcount == 0 )
	    really_unmap( mm );
	else
	    mm = &(*mm)->next;
	}
    }


_Checked static void
really_unmap(Map **mm : itype(_Ptr<_Ptr<Map>>))
    {
    _Ptr<Map> m = ((void *)0);

    m = *mm;
    if ( m->size != 0 )
	{
#ifdef HAVE_MMAP
        int res = 0;
        _Unchecked { munmap( (void*) m->addr, m->size ); }
	if ( res < 0 )
	    syslog( LOG_ERR, "munmap - %m" );
#else /* HAVE_MMAP */
	free( m->addr );
#endif /* HAVE_MMAP */
	}
    /* Update the total byte count. */
    mapped_bytes -= m->size;
    /* And move the Map to the free list. */
    *mm = m->next;
    --map_count;
    m->next = free_maps;
    free_maps = m;
    ++free_count;
    /* This will sometimes break hash chains, but that's harmless; the
    ** unmapping code that searches the hash table knows to keep searching.
    */
    hash_table[m->hash_idx] = (_Ptr<Map>) 0;
    }


_Checked void
mmc_term( void )
    {
    _Ptr<Map> m = ((void *)0);

    while ( maps !=  0 )
	really_unmap( &maps );
    while ( free_maps !=  0 )
	{
	m = free_maps;
	free_maps = m->next;
	--free_count;
	free<Map>( m );
	--alloc_count;
	}
    }


/* Make sure the hash table is big enough. */
_Checked static int
check_hash_size( void )
    {
    int i;
    _Ptr<Map> m = ((void *)0);

    /* Are we just starting out? */
    if ( hash_table == 0 )
	{
	hash_size = INITIAL_HASH_SIZE;
	hash_mask = hash_size - 1;
	}
    /* Is it at least three times bigger than the number of entries? */
    else if ( hash_size >= map_count * 3 )
	return 0;
    else
	{
	/* No, got to expand. */
	free<_Ptr<Map>>( hash_table );
	/* Double the hash size until it's big enough. */
	do
	    {
	    hash_size = hash_size << 1;
	    }
	while ( hash_size < map_count * 6 );
	hash_mask = hash_size - 1;
	}
    /* Make the new table. */
    hash_table = (_Array_ptr<_Ptr<Map>>) malloc<_Ptr<Map>>( hash_size * sizeof(Map*) );
    if ( hash_table ==  0 )
	return -1;
    /* Clear it. */
    for ( i = 0; i < hash_size; ++i )
	hash_table[i] = (_Ptr<Map>) 0;
    /* And rehash all entries. */
    for ( m = maps; m !=  0; m = m->next )
	if ( add_hash( m ) < 0 )
	    return -1;
    return 0;
    }


_Checked static int
add_hash(Map *m : itype(_Ptr<Map>))
    {
    unsigned int h, he, i;

    h = hash( m->ino, m->dev, m->size, m->ct );
    he = ( h + hash_size - 1 ) & hash_mask;
    for ( i = h; ; i = ( i + 1 ) & hash_mask )
	{
	if ( hash_table[i] == 0 )
	    {
	    hash_table[i] = m;
	    m->hash = h;
	    m->hash_idx = i;
	    return 0;
	    }
	if ( i == he )
	    break;
	}
    return -1;
    }


_Checked static Map *find_hash(ino_t ino, dev_t dev, off_t size, time_t ct) : itype(_Ptr<Map>)
    {
    unsigned int h, he, i;
    _Ptr<Map> m = ((void *)0);

    h = hash( ino, dev, size, ct );
    he = ( h + hash_size - 1 ) & hash_mask;
    for ( i = h; ; i = ( i + 1 ) & hash_mask )
	{
	m = hash_table[i];
	if ( m == 0 )
	    break;
	if ( m->hash == h && m->ino == ino && m->dev == dev &&
	     m->size == size && m->ct == ct )
	    return m;
	if ( i == he )
	    break;
	}
    return (_Ptr<Map>) 0;
    }

_Checked static unsigned int
hash( ino_t ino, dev_t dev, off_t size, time_t ct )
    {
    unsigned int h = 177573;

    h ^= ino;
    h += h << 5;
    h ^= dev;
    h += h << 5;
    h ^= size;
    h += h << 5;
    h ^= ct;

    return h & hash_mask;
    }


/* Generate debugging statistics syslog message. */
_Checked void
mmc_logstats( long secs )
    {
    syslog(
	LOG_NOTICE, "  map cache - %d allocated, %d active (%lld bytes), %d free; hash size: %zd; expire age: %lld",
	alloc_count, map_count, (long long) mapped_bytes, free_count, hash_size,
	(long long) expire_age );
    if ( map_count + free_count != alloc_count )
	syslog( LOG_ERR, "map counts don't add up!" );
    }
