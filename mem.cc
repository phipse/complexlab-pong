/**
 * \file   libc_backends/l4re_mem/mem.cc
 */
/*
 * (c) 2004-2009 Technische Universität Dresden
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU Lesser General Public License 2.1.
 * Please see the COPYING-LGPL-2.1 file for details.
 */
/* Author: Philipp Eppelt
 * Date: 12.02.2012
 * Purpose: Implementation of malloc and free
 * ToDo: severe testing and debugging / circumvent global variables /
 *	  sort-insert into freeList / coalesce free chunks -> garbage
 *	  collection
 */
#include <stdlib.h>
#include <cstdio>
#include <assert.h>

#include <l4/sys/kdebug.h>
#include <l4/re/util/cap_alloc>
#include <l4/re/env>
#include <l4/re/mem_alloc>
#include <l4/util/util.h>

#define INITSIZE (1 * L4_PAGESIZE) 
#define DEBUG 0

typedef struct Memory_header 
{
  unsigned size;
  Memory_header *next;
   
} mem_header;

typedef struct nxt_ds
{
  unsigned size;
  nxt_ds* next;
} next_dataspace;

static next_dataspace* dsList;




bool 
create_slice( 
    unsigned size,	      // *IN*	  size of the new slice
    mem_header **header,      // *OUT*	  pointer to the new slice
    mem_header **freeList )  // *IN-OUT*  headpointer to list of free slices
{
/* Creates a new slice at the end of the first fitting free memory chunk or
 * searches for a suitable chunk and slices it, if it is not too small. 
 * */
  //printf( "Malloc create_slice: hello!\n" );
  unsigned neededSpace = size + sizeof(mem_header);
  if( (*freeList)->size >= neededSpace )
  {
    // shouldn't it be -size instead of -neededSpace?
    void* newChunk = static_cast<void*>( 
	static_cast<char*>( static_cast<void*>(*freeList) ) + 
	  (*freeList)->size - neededSpace );
    *header = static_cast<mem_header*>(newChunk);
//  printf( "createSlice: &header : %p\n", *header );
//  printf( "header->size: %p\n", &((*header)->size) );
    (*header)->size = size;
    (*header)->next = 0;
    (*freeList)->size -= neededSpace;

    // if true the header chunk was used. new header needed.
    if( *freeList ==  *header ) 
      *freeList = (*freeList)->next;
  }
  else
  {
    mem_header* iter = *freeList;
    mem_header* previous = 0;
    while( iter != 0 && iter->size < neededSpace )
    {
      previous = iter;
      iter = iter->next;
    }
    if( iter == 0 ) return false;
    // if the size of the free chunk is not at least 2*sizeof(mem_header) +
    // size + 1 a slicing isn't necessary.
    if( iter->size > ( neededSpace + sizeof(mem_header) + 1 ) )
    {
      void* newSlice = static_cast<void*>( 
	  static_cast<char*>( static_cast<void*>(iter) ) + neededSpace) ;
      mem_header* remainder = static_cast<mem_header*>( newSlice );
      remainder->size = iter->size - neededSpace;
      remainder->next = iter->next;
    // dequeue slice 'iter' and set next to 0
      previous->next = remainder;
      iter->next = 0;
      iter->size = size;
      *header = iter;
    }
    else 
    {
      previous->next = iter->next; 
      iter->next = 0;
      *header = iter;
    }
  }
  return true;  
}



static mem_header *usedList = 0;
static mem_header *freeList = 0;



void
enqueue_used_list( 
    mem_header *element )    // *IN-OUT* element to enqueue 
{
/* enquese an element in the list of used mem_headers. */
  if( usedList == 0 )
    usedList = element;
  else
  { 
    mem_header* iter = usedList;
    while( iter->next != 0 )
      iter = iter->next;
    iter->next = element;
//    printf( "elenext: %p, ele: %p\n", element->next, element );
  }
  element->next = 0;
#if DEBUG
  mem_header* iter = usedList;
  for( ; iter; iter = iter->next )
    printf( "iter: %p\n", iter );
#endif
}



void
dequeue_used_enqueue_free( 
	void* toFree )		    // *IN* pointer to the to be freed element
{
  printf( "toFree: %p\n", toFree) ;
  toFree = static_cast<void*>( 
      static_cast<char*>(toFree) - sizeof(mem_header) );
  mem_header* element = static_cast<mem_header*>(toFree);
  
  printf( "toFree: %p, size: %u\n", element, element->size );
  mem_header* iter = usedList;
  mem_header* previous = 0;

  if( element == iter )
    usedList = usedList->next;
  else
  {
    while( iter != element )
    {
      previous = iter;
      iter = iter->next;
    }
    previous->next = iter->next; 
  }
  //reuse of previous for iteration through freeList
  previous = freeList;
  if( freeList == 0 )
    freeList = iter; 
  else
  {
    while( previous->next != 0 )
      previous = previous->next;
    previous->next = iter;
  }
  iter->next = 0;
}


bool
scan_free_list( 
    unsigned size,	      // *IN*  size of the to be located slice
    mem_header **header,      // *OUT* address of the return pointer
    mem_header *fl )    // *IN*  headpointer to the list of free slices
{
/* Scans the free list to find a suitable slice of memeory. Returns true and a
 * mem_header or false.
 */
  *header = 0;
  mem_header* prev = fl;
  while( fl != 0 )
  {
#if DEBUG
      printf( "fl: %p\n", fl);
#endif
    if( fl->size == size )
    {
      *header = fl;
      if( *header == freeList )
	freeList = freeList->next;
      else
	prev->next = fl->next; 
      return true;
    }
    else
    {
      prev = fl;
      fl = fl->next;
    }
  }
  return false;
}

  

void
print_used_free()
{
  mem_header* ul = usedList;
  for( ; ul != 0; ul = ul->next )
    printf( "used: %p, size: %u\n", ul, ul->size);
  mem_header* fl = freeList;
  for( ; fl != 0; fl = fl->next )
    printf( "free: %p, size: %u\n", fl, fl->size);
}



bool
malloc_init( 
    mem_header** freeList,    // *OUT* pointer to the first header
    void** lastValidAddress ) // *OUT* pointer to the end of our memory 
{
/* Allocates a chunk of memory and attaches it to the local address space.
 * Additionaly it creates the first header at the location of the returned
 * address and returns a pointer to this first header. 
 * If anything failes, it returns false and an error message. Otherwise it 
 * returns true. 
 */

  L4::Cap<L4Re::Mem_alloc> memcap = L4Re::Env::env()->mem_alloc();
  L4::Cap<L4Re::Dataspace> dscap = L4Re::Util::
		          cap_alloc.alloc<L4Re::Dataspace>();
  if( int ret = memcap->alloc( INITSIZE, dscap, 
			  L4Re::Mem_alloc::Continuous ) )
  {
    fprintf( stdout, "Memory allocation failed: %i\n", ret );
    return false;
  }
  void *addr = 0;
  if( int ret = L4Re::Env::env()->rm()->attach( &addr, INITSIZE, 
	L4Re::Rm::Search_addr, dscap ) )
  {
    fprintf( stdout, "Memory not attached to region: %i\n", ret );
    return false;
  }
  dsList = static_cast<next_dataspace*>(addr);
  dsList->size = INITSIZE;
  dsList->next = 0;
  *lastValidAddress = static_cast<void*>( (char*)addr + INITSIZE );
  addr = static_cast<void*>( static_cast<char*>( addr ) + sizeof(next_dataspace) );
  *freeList = static_cast<mem_header*>(addr);
  (*freeList)->size = INITSIZE - sizeof(mem_header) - sizeof(next_dataspace);
  (*freeList)->next = 0;
#if DEBUG 
  printf( "last valid address: %p\n", *lastValidAddress);
  printf( "dsList: %p\n", dsList );
  printf( "firstSlice: %p size: %u\n", *freeList, (*freeList)->size );
#endif
  return true;
}



bool
reallocate_memory( unsigned size )
{
  L4::Cap<L4Re::Mem_alloc> memCap = L4Re::Env::env()->mem_alloc();
  L4::Cap<L4Re::Dataspace> dsCap = L4Re::Util::
			cap_alloc.alloc<L4Re::Dataspace>();
  if( int ret = memCap->alloc( size, dsCap, L4Re::Mem_alloc::Continuous ) )
  {
    fprintf( stdout, "Memory reallocation failed: %i\n", ret );
    return false;
  }
  void* newDs = 0;
  if( int ret = L4Re::Env::env()->rm()->attach( &newDs, size, 
	L4Re::Rm::Search_addr, dsCap ) )
  {
    fprintf( stdout, "Attaching memory to region failed: %i\n", ret );
    return false;
  }
  next_dataspace* iter = dsList;
  while( iter->next != 0 )
    iter = iter->next;
  iter->next = static_cast<next_dataspace*>(newDs);
  iter->next->next = 0;
  unsigned allocSize = 0;
  if( size % L4_PAGESIZE != 0 )
    allocSize = (size / L4_PAGESIZE + 1) * L4_PAGESIZE;
  else
    allocSize = size / L4_PAGESIZE * L4_PAGESIZE;
  iter->next->size = allocSize -sizeof(next_dataspace);
  
  mem_header* flIter = freeList;
  while( flIter->next != 0 )
    flIter = flIter->next;
  flIter->next = static_cast<mem_header*>( static_cast<void*> (
		static_cast<char*>(newDs) + sizeof(next_dataspace) ) );
  flIter->next->size = allocSize - sizeof(next_dataspace) - sizeof(mem_header);

#if 1
  iter = iter->next;
  flIter = flIter->next;
  printf( "DS size: %u, iter: %p\n", iter->size, iter );
  printf( "Mem_header: %p, size: %u\n", flIter, flIter->size );
#endif

  return true;
}




void*
malloc(unsigned size) throw()
{
 // enter_kdebug("malloc");

//  static mem_header *usedList = 0;
//  static mem_header *freeList = 0;
  static void* lastValidAddress = 0;
  if( freeList == 0 )
  {
    if( !malloc_init( &freeList, &lastValidAddress ) )
    {
      fprintf( stdout, "Malloc init failed!\n\n" );
      throw( "Malloc init failed!\n" );
    }
    //else
    //  init_free( &usedList, &freeList, true );
    printf( "Malloc: Init done! freeList: %p \n", freeList );
    printf( "Malloc: freelist->size: %u \n", freeList->size );
  }

  mem_header *ret=0;
  if( !scan_free_list( size, &ret, freeList ) )
  {
    //printf( "Malloc: Scan free list failed\n" );
    while( !create_slice( size, &ret, &freeList ) )
    {
      fprintf( stdout, "Slice creation failed! Out of memory? Allocating!\n" );
      if( !reallocate_memory( size ) )
      {
	fprintf( stdout, "PANIC: Additional memory allocation failed!\n" );
	l4_sleep_forever();
      }
    }
    //printf( "Malloc: slice creation successful\n" );
  }
  enqueue_used_list( ret );

  void* returnAddress = static_cast<void*>( ret );
  returnAddress = static_cast<void*>( 
      static_cast<char*>(returnAddress) + sizeof(mem_header) );
//  print_used_free();

  return returnAddress;
}



void
coalesce_neighbouring(  )
{
  mem_header* fl = freeList;
  mem_header* prev = 0;
//  print_used_free();
  while( fl->next != 0 )
  {
    void* endPtr = (void*) ((unsigned)(fl->next ) 
	  + fl->next->size + sizeof(mem_header) );
#if DEBUG 
    printf( "endPtr: %p, fl: %p\n", endPtr, fl );
    printf( "flnext: %p, sizeof: %u\n", fl->next, fl->next->size );
#endif
    if( endPtr == fl )
    {
      fl->next->size += fl->size + sizeof(mem_header);
      prev->next = fl->next;
    }
    else
    {
      prev = fl;
      fl = fl->next;
    }
    endPtr = 0;
  }
}



void
deallocateDataspace()
{
  next_dataspace* iter = dsList;
  next_dataspace* prev = dsList;
  mem_header* memIter = freeList;
  mem_header* memPrev = freeList;

  for(; iter; iter = iter->next )
  {
    mem_header* first = static_cast<mem_header*>( static_cast<void*>( 
	  static_cast<char*>( static_cast<void*>( iter ) )+ sizeof(next_dataspace) ) );
    l4_addr_t math = first->size + sizeof(mem_header) + sizeof(next_dataspace );
    printf( "Ds: %p, first: %p\n", iter, first );
    printf( "math: %lu, iter-size: %u \n", math, iter->size );
    printf( "sizeof mem_header: %i, nxt_ds: %i\n", sizeof(mem_header), sizeof(next_dataspace) );
    if( iter->size == first->size + sizeof(mem_header) + sizeof(next_dataspace) )
    {
      prev->next = iter->next;
      int err = L4Re::Env::env()->rm()->detach( static_cast<void*>(iter), 0 );

      if( err )
      {
	printf( "Dataspace detach error: %i\n", err );
	printf( "Dataspace still attached and enqueued!\n" );
	prev->next = iter;
      }
      else
      {
	while( memIter != first )
	{
	  memPrev = memIter;
	  memIter = memIter->next;
	}
	memPrev->next = memIter->next;
      }
#if 1
      printf( "Ds deallocated: %p, freeListEntry: %p\n", iter, memIter );
#endif
    }
  } 
}

  

void free(void *p) throw()
{
  dequeue_used_enqueue_free( p );
  static int cnt = 0;
  if( cnt >= 0 )
  {
    coalesce_neighbouring( );
    cnt = 0;
    deallocateDataspace();
  }
  cnt++;
  
    
    
  print_used_free();
}
