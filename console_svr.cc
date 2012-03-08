#include <l4/Console/console_svr.h>
#include <pthread-l4.h>
#include <l4/sys/semaphore>
#include <l4/libgfxbitmap/font.h>
#include <l4/libgfxbitmap/bitmap.h>
#include <l4/util/util.h>

SessionServer<VFB, Cap_hooks> session;
L4::Cap<L4::K_semaphore> semcap = L4Re::Util::cap_alloc.alloc<L4::K_semaphore>();
L4::Semaphore sem(semcap);

VFB::VFB()
 {
  _vds = new VDS( (l4_addr_t) _fbuffer,  _fb->buffer()->size() );
  init( _vds->obj_cap(), _gooInfo, _info );
}



VDS::VDS( l4_addr_t addr, l4_size_t sz )
{
  Dataspace_svr::_ds_start = addr;
  Dataspace_svr::_ds_size = sz;
  Dataspace_svr::_rw_flags = Dataspace_svr::Writable;
  realStart = addr;

  if( !session.server.registry()->register_obj( 
      this ).is_valid() )
    printf( "VDS not registered!\n");

  myDummyDs = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  L4Re::Env::env()->mem_alloc()->alloc( sz, myDummyDs );
  myDummyDs->clear( 0, sz );

//  fprintf( stdout, "dummySize: %x\n", myDummyDs->size() );
  
  L4Re::Env::env()->rm()->attach( &dummyStart, myDummyDs->size(), 
      L4Re::Rm::Search_addr, myDummyDs );
  
  if( session.sessions.size() > 0 ) 
    Dataspace_svr::_ds_start = dummyStart;
  Dataspace_svr::clear( 0, sz );

}


void
VFB::init(L4::Cap<void> cap, L4Re::Video::Goos::Info gi, L4Re::Video::View::Info vi) 
{// register at obj_registry
  
  Goos_svr::_fb_ds = L4::cap_cast<L4Re::Dataspace>(cap);
  Goos_svr::_screen_info = gi;
  Goos_svr::_view_info = vi;

//  printf( "VFB init: _gi %lu, vi %lu\n", gi.width, vi.bytes_per_line );
  if( !session.server.registry()->register_obj( 
      this ).is_valid() )
    printf( "VFB not registered!\n");
}

int 
VDS::changeFb()
{

  l4_addr_t start = _ds_start;
  if( start == realStart )
  { // switch to dummyStart
    memcpy( (void*) dummyStart, (void*) realStart, _ds_size );
    Dataspace_svr::_ds_start = dummyStart;
  }
  else if( start == dummyStart )
  { // switch to realStart
    memcpy( (void*) realStart, (void *) dummyStart, _ds_size );
    Dataspace_svr::_ds_start = realStart;
  }
  else 
  {
    printf( "fb dataspaces are not valid! Sleeping!\n" );
    l4_sleep_forever();
  }

  // save current fb state  OR  restore old state
//  memcpy( (void*)_ds_start, (void*) start, _ds_size );
  
  printf( "old_dsstart: %x, new_dsstart: %x \n", start, _ds_start );

  // remove fb mapping from virt addr space
  if( start == realStart )
  {
    l4_addr_t tmp = start;
    L4::Cap<L4::Task> task = L4Re::Env::env()->task();
    while( tmp < start + _ds_size )
    {
      task->unmap( l4_fpage( tmp, L4_SUPERPAGESIZE, L4_FPAGE_RW ),
	  L4_FP_OTHER_SPACES );
      tmp += L4_SUPERPAGESIZE;
    }
  }
  // do I need an else to get the fpage mapping?


  
  return L4_EOK;
}

void
VFB::changeDs()
{
  _vds->changeFb(); 
}

void*
switcher( void* )
{
  printf( " ==== switcher ==== \n" );
  // listens for special key strokes and switches the fb rights
  L4::Cap<void> keyb = L4Re::Env::env()->get_cap<void>("keyb");
  L4::Ipc::Iostream io( l4_utcb() );

  io << 1; 
  io << L4::Cap<L4::Thread>( pthread_getl4cap( pthread_self() ) );
  l4_msgtag_t tag = io.call( keyb.cap() );
  if( tag.has_error() )
  {
    printf( "keyboard registration failed: %lu\n", l4_utcb_tcr()->error );
    return 0;
  }

  L4::Ipc::Istream in( l4_utcb() );
  int current = 0;
  char rec;
  while( 1 )
  {
    l4_umword_t sender = 0;
    in.wait( &sender );
    in >> rec;
    in.reset();
    
    if( rec == '5' && current != 0 )
    {
      session.sessions[0].changeDs();
      session.sessions[1].changeDs();
      current = 0;
    }
    else if( rec == '6' && current != 1 )
    {
      session.sessions[1].changeDs();
      session.sessions[0].changeDs();
      current = 1;
    }
  }

  return 0;
}


int main( void )
{
  printf( "Hello console_svr!\n");
  if( !session.server.registry()->register_obj( 
	&session, "ds_svr" ).is_valid())
  {
    printf( "Can't register sessions\n" );
    return -1;
  }

  // grab fb and provide it to every wrapper_so as global variable
  _fb = new L4Re::Util::Video::Goos_fb("fbuffer");
  _fbuffer = _fb->attach_buffer();
  _fb->goos()->info( &_gooInfo );
  _fb->view_info( &_info );

  // keep track of the active wrapper_so and switch on key event
  pthread_t s;
  pthread_create( &s, 0, switcher, 0 ); 
  
  
  session.server.loop();

  return 0;
} 

