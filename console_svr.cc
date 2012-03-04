#include "console_svr.h"
#include <pthread-l4.h>
#include <l4/libgfxbitmap/font.h>
#include <l4/libgfxbitmap/bitmap.h>

SessionServer<Wrapper_so, Cap_hooks> session;


Wrapper_so::Wrapper_so()
{
  _fb = new L4Re::Util::Video::Goos_fb("fbuffer");
  _fbuffer = _fb->attach_buffer();
  _fb->goos()->info( &_gooInfo );
  _fb->view_info( &_info );
  _vds = new VDS( (l4_addr_t) _fbuffer,  _fb->buffer()->size() );
  _vfb = new VFB( _vds->obj_cap(), _gooInfo, _info );

 if( !session.server.registry()->register_obj( this ).is_valid() ) 
    printf( "Wrapper not registered!\n" );

// draw dummy 
  gfxbitmap_color_pix_t cpix = gfxbitmap_convert_color( 
    (l4re_video_view_info_t*)&_info, 500 );
  gfxbitmap_fill( (l4_uint8_t*)_fbuffer,
      (l4re_video_view_info_t*)&_info, 10, 10, 200, 200, cpix );

}



int
Wrapper_so::dispatch( l4_umword_t , L4::Ipc::Iostream &io )
{
  l4_umword_t op;
  io >> op;

  switch( op )
  {
    case 0:
      _client = rcv_cap;
      return L4_EOK;

    case 1: 
      io << _vfb->obj_cap();
      return L4_EOK;

    case 2: // not used so far
      io << _vds->obj_cap();
      return L4_EOK;

    case 3:
      switchClient();
      return L4_EOK;

    default:
      return -L4_ENOSYS;
  }
}


void
Wrapper_so::switchClient()
{
  _vds->changeFb();
  // take clients dummy dataspace
  // memcopy the framebuffer to this dataspace
  // let client use the dummy dataspace
  // 
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
}



VFB::VFB(L4::Cap<void> cap, L4Re::Video::Goos::Info gi, L4Re::Video::View::Info vi) 
{// register at obj_registry
  
  Goos_svr::_fb_ds = L4::cap_cast<L4Re::Dataspace>(cap);
  Goos_svr::_screen_info = gi;
  Goos_svr::_view_info = vi;

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
    Dataspace_svr::_ds_start = dummyStart;
  }
  else
  { // switch to realStart
    Dataspace_svr::_ds_start = realStart;
  }
  l4_addr_t tmp = start;
  
  printf( "old_dsstart: %x, new_dsstart: %x \n", start, _ds_start );

  L4::Cap<L4::Task> task = L4Re::Env::env()->task();
  while( tmp < start + _ds_size )
  {
    task->unmap( l4_fpage( tmp, L4_SUPERPAGESIZE, L4_FPAGE_RW ),
	L4_FP_OTHER_SPACES );
    tmp += L4_SUPERPAGESIZE;
  }
  
  memcpy( (void*)_ds_start, (void*) start, _ds_size );

  
  return L4_EOK;
}


void*
switcher( void* )
{
  printf( " ==== switcher ==== \n" );
  // listens for special key strokes and switches the fb rights
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

  // keep track of the active wrapper_so and switch on key event
  pthread_t s;
  pthread_create( &s, 0, switcher, 0 ); 
  

  session.server.loop();

  return 0;
} 

