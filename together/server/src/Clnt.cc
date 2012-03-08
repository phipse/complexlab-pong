#include <cstdio>
#include <cstring>
#include <l4/cxx/ipc_stream>
#include <l4/re/env>
#include <l4/re/util/video/goos_fb>
#include <l4/re/video/view>
#include <l4/re/util/cap_alloc>
#include <l4/libgfxbitmap/font.h>
#include <l4/libgfxbitmap/bitmap.h>
#include <l4/util/util.h>


int main( void )
{
  printf( "Hello World!\n");

  L4::Cap<void> wrapper_cap = L4Re::Env::env()->get_cap<void>( "ds_svr" );

  L4::Ipc::Iostream io( l4_utcb() );
  io << 0 << L4Re::Env::env()->main_thread();
  l4_msgtag_t tag = io.call( wrapper_cap.cap() );

  if( tag.has_error() )
  {
    printf( "snd cap msg error: %lu\n", l4_utcb_tcr()->error );
    return -1;
  }

  //get fb_svr cap
  io.reset();
  L4::Cap<L4Re::Video::Goos> recv = 
      L4Re::Util::cap_alloc.alloc<L4Re::Video::Goos>();
  
  io << 1;
  io << L4::Ipc::Small_buf( recv.cap() );
  tag = io.call( wrapper_cap.cap() );

  if( tag.has_error() )
  {
    printf( "instream msg error: %lu\n", l4_utcb_tcr()->error );
    return -2;
  }

  L4Re::Util::Video::Goos_fb goos(recv);
  l4_addr_t ds_start;
  long err;
    
  err = L4Re::Env::env()->rm()->attach( &ds_start, goos.buffer()->size(),
     L4Re::Rm::Search_addr, goos.buffer() );
  
  if( err )
  {
    printf("Couldn't attach ds: %li\n", err );
    return -1;
  }
  
  L4Re::Video::View::Info info;
  goos.view_info( &info );
  
  gfxbitmap_color_pix_t cpix = gfxbitmap_convert_color( 
    (l4re_video_view_info_t*)&info, 0xfff9 );
  gfxbitmap_fill( (l4_uint8_t*)ds_start,
      (l4re_video_view_info_t*)&info, 40, 40, 200, 200, cpix );


  // testing the DS switch 
  io.reset();
  io << 3 ;
  tag = io.call( wrapper_cap.cap() );
  if( tag.has_error() )
    printf( "wrapper call 3 error\n" );

  for( unsigned i = 0; i < 100000; i++ )
  {
    ((int*)ds_start)[i] = 0xffffff99;
  }

  io.reset();
  io << 3;
  tag = io.call( wrapper_cap.cap() );
  if( tag.has_error() )
    printf( "wrapper call 3 error\n" );

  for( unsigned i = 50000; i < 100000; i++ )
  {
    ((int*)ds_start)[i] = 0xffffffff;
  }
  
  //get keyboard 
  io.reset();
  L4::Cap<void> keyboard = L4Re::Env::env()->get_cap<void>("keyb");
  if( !keyboard.is_valid() )
  {
    printf( "Invalid keyboard cap!\n" );
    return -5;
  }
  
  io << 1 << L4Re::Env::env()->main_thread();
  tag = io.call( keyboard.cap() );
  
  if( tag.has_error() )
  {
    printf( "keyboard call error: %lu\n", l4_utcb_tcr()->error );
    return -6;
  }
 
  // print received stuff into fb
  gfxbitmap_font_init();
  io << 3;
  
  L4::Ipc::Istream in( l4_utcb() );
  while( 1 ) 
  {
    l4_umword_t sender = 0;
    in.wait( &sender );
    char rec; 
    in.get(rec);
    printf( "got: '%c'\n", rec );
    if( rec == '5' ) 
      for( unsigned i = 100000; i < 200000; i++ )
	((int*)ds_start)[i] = 0xffffff;

    in.reset();
  }

  
  printf( "sleeping!\n");
  l4_sleep_forever();
  return 0;
}
