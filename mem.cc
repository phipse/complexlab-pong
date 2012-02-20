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
 *    implementing shared memory for usedList and freeList /
 *    out-of-memory detection and reallocation
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

typedef struct Memory_header 
{
  unsigned size;
  Memory_header *next;
   
} mem_header;



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
  *freeList = static_cast<mem_header*>(addr);
  (*freeList)->size = INITSIZE;
  (*freeList)->next = 0;
  if( *freeList != addr )
  {
    fprintf( stdout, "Malloc_init: casting fault!\n" );
    return false;
  }
  *lastValidAddress = static_cast<void*>( (char*)addr + INITSIZE + 1);
  printf( "last valid address: %p\n", *lastValidAddress);
  return true;
}



bool
reallocate_memory( void* lastValidAddress)
{
  L4::Cap<L4Re::Mem_alloc> memCap = L4Re::Env::env()->mem_alloc();
  L4::Cap<L4Re::Dataspace> dsCap = L4Re::Util::
			cap_alloc.alloc<L4Re::Dataspace>();
  if( int ret = memCap->alloc( INITSIZE, dsCap, L4Re::Mem_alloc::Continuous ) )
  {
    fprintf( stdout, "Memory reallocation failed: %i\n", ret );
    return false;
  }
  l4_addr_t* attachAddr = static_cast<l4_addr_t*>(lastValidAddress);
  printf( "attachAddr: %p lvd: %p\n", attachAddr, lastValidAddress );
  if( int ret = L4Re::Env::env()->rm()->attach( &attachAddr, INITSIZE, 
	0, dsCap ) )
  {
    fprintf( stdout, "Attaching memory to region failed: %i\n", ret );
    return false;
  }
/*  if( attachAddr != lastValidAddress ) 
  {
    fprintf( stdout, "Attach fault! %p %p \n", lastValidAddress, attachAddr );
    return false;
  }*/
  return true;
}



bool
scan_free_list( 
    unsigned size,	      // *IN*  size of the to be located slice
    mem_header **header,      // *OUT* address of the return pointer
    mem_header *freeList )    // *IN*  headpointer to the list of free slices
{
/* Scans the free list to find a suitable slice of memeory. Returns true and a
 * mem_header or false.
 */
  header = 0;
  while( freeList != 0 )
  {
    if( freeList->size == size )
    {
      *header = freeList;
      return true;
    }
    else
      freeList = freeList->next;
  }
  return false;
}



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
  printf( "createSlice: &header : %p\n", *header );
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



void
enqueue_used_list( 
    mem_header *element,     // *IN-OUT* element to enqueue 
    mem_header **usedList )  // *IN-OUT* pointer to the first used element
{
/* enquese an element in the list of used mem_headers. */
  if( *usedList == 0 )
    *usedList = element;
  else
  {
    element->next = *usedList;
    *usedList = element;
    /*
    mem_header* iter = *usedList;
    while( iter->next != 0 )
      iter = iter->next;
    iter->next = element;*/
  }
}


void
dequeue_used_enqueue_free( 
	void* toFree,		    // *IN* pointer to the to be freed element
	mem_header** usedList,	    // *IN-OUT*
	mem_header** freeList )	    // *IN-OUT*
{
  printf( "toFree: %p\n", toFree) ;
  toFree = static_cast<void*>( 
      static_cast<char*>(toFree) - sizeof(mem_header) );
  mem_header* element = static_cast<mem_header*>(toFree);
  mem_header* iter = *usedList;
  mem_header* previous = 0;
  if( element == *usedList )
    *usedList = (*usedList)->next;
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
  previous = *freeList;
  if( previous == 0 )
    *freeList = iter; 
  else
  {
    while( previous->next != 0 )
      previous = previous->next;
    previous->next = iter;
  }
  iter->next = 0;
}



/*void
init_free( mem_header** uList, mem_header** fList, bool write ) 
{*/
/* Stores pointers to the pointers to the list of used and free slices, so free
 * and malloc share the same lists.
 * PROBABLY WRONG!
 *//*
  static mem_header* usedList = 0;
  static mem_header* freeList = 0;
  if( write )
  {
    usedList = *uList;
    freeList = *fList;
  }
  else
  {
    *uList = usedList;
    *fList = freeList;
  }
}
*/

static mem_header *usedList = 0;
static mem_header *freeList = 0;

void
print_used_free()
{
  mem_header* ul = usedList;
  for( ; ul != 0; ul = ul->next )
    printf( "used: %p\n", ul);
  mem_header* fl = freeList;
  for( ; fl != 0; fl = fl->next )
    printf( "free: %p\n", fl);
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
    printf( "Malloc: Scan free list failed\n" );
    while( !create_slice( size, &ret, &freeList ) )
    {
      fprintf( stdout, "Slice creation failed! Out of memory?\n" );
      if( !reallocate_memory( lastValidAddress ) )
      {
	fprintf( stdout, "PANIC: Additional memory allocation failed!\n" );
	l4_sleep_forever();
      }
    }
    printf( "Malloc: slice creation successful\n" );
  }
  enqueue_used_list( ret, &usedList );

  void* returnAddress = static_cast<void*>( ret );
  returnAddress = static_cast<void*>( 
      static_cast<char*>(returnAddress) + sizeof(mem_header) );
//  print_used_free();

  return returnAddress;
}



void free(void *p) throw()
{
//  static mem_header* usedList = 0;
//  static mem_header* freeList = 0;
//  static bool initialized = false;
//  if( !initialized ) 
//    init_free( &usedList, &freeList, false );

  dequeue_used_enqueue_free( p, &usedList, &freeList );
  //print_used_free();
}
