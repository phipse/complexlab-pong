/* Maintainer:	Philipp Eppelt
 * Date:	20/02/2012
 * Purpose:	Implementation of the Client side for Hello-Client-Server
 *		example, the framebuffer assignment including scrolling back
 *		and forth in pagesize steps.
 * ToDo:	Spliting this into several files // extract number of lines
 *		dynamicly out of the buffer_info // 
 */

#include <cstdio>
#include <cstring>

#include <l4/re/env>
#include <l4/re/util/meta>
#include <l4/sys/capability>
#include <l4/cxx/ipc_stream>
#include <l4/sys/kdebug.h>
#include <l4/re/util/object_registry>
#include <l4/sys/types.h>
#include <l4/re/util/video/goos_fb>
#include <l4/re/video/goos>
#include <l4/re/video/colors>
#include <l4/re/video/view>
#include <l4/libgfxbitmap/bitmap.h>
#include <l4/libgfxbitmap/font.h>
#include <l4/sys/kdebug.h>

#include "shared.h"

#define LINES_PER_PAGE 51


typedef struct text_tracker_t
{
  unsigned linenbr;
  const char* text;
  struct text_tracker_t* next;
  struct text_tracker_t* prev;
} text_tracker_t;



class Hello 
{
  public:
  int show( char const *str );

};



int Hello::show( const char *str )
{
  L4::Cap<void> srv = L4Re::Env::env()->get_cap<void>("hello_server");
  if( !srv.is_valid() )
  {
    printf( "Could not get server capability!\n");
    return 1;
  }
  L4::Ipc::Iostream io( l4_utcb() );
  unsigned long stringsize = strlen( str );
  io << L4::Ipc::Buf_cp_out<const char>( str, stringsize);
  l4_msgtag_t res = io.call( srv.cap(), L4::Meta::Protocol );
  printf( "res: %li\n", res.label() );
  if( l4_ipc_error( res, l4_utcb() ) )
      return 1;

  io.reset();

  io << 0 << 43;
  res = io.call( srv.cap(), L4::Factory::Protocol );
  if( l4_ipc_error( res, l4_utcb() ) )
    return 1;
  return 0; 
}



static L4Re::Util::Video::Goos_fb *fb;
static void* base;


  
void* 
pixel_address(int x, int y , L4Re::Video::View::Info info)
{
//  unsigned bytes_per_line = info.bytes_per_line;
  void *addr =(void*) ((char*) base +
      ( y * info.bytes_per_line ) + 
      ( x * info.pixel_info.bytes_per_pixel()) );
  return addr;
}



void
Info_to_type( L4Re::Video::View::Info *in, l4re_video_view_info_t *out )
{
  out->flags = in->flags;
  out->view_index = in->view_index;
  out->xpos = in->xpos;
  out->ypos = in->ypos;
  out->width = in->width;
  out->height = in->height;
  out->buffer_offset = in->buffer_offset;
  out->bytes_per_line = in->bytes_per_line;
  out->buffer_index = in->buffer_index;
  //get color components
  l4re_video_color_component_t r,g,b,a;
  r.size = in->pixel_info.r().size();
  r.shift = in->pixel_info.r().shift();
  g.size = in->pixel_info.g().size();
  g.shift = in->pixel_info.g().shift();
  b.size = in->pixel_info.b().size();
  b.shift = in->pixel_info.b().shift();
  a.size = in->pixel_info.a().size();
  a.shift = in->pixel_info.a().shift();
  
  //create pixel_info_t
  l4re_video_pixel_info_t pixinfo;
  pixinfo.r = r;
  pixinfo.g = g;
  pixinfo.b = b;
  pixinfo.a = a;
  pixinfo.bytes_per_pixel = in->pixel_info.bytes_per_pixel();
  
  out->pixel_info = pixinfo;
}



static text_tracker_t* trackhead;
static text_tracker_t* tracktail;

void
add_Line( unsigned linenbr, const char* text )
{
  text_tracker_t* track = new text_tracker_t();
  track->linenbr = linenbr;
  track->text =  text;
  if( trackhead == 0 )
  {
    track->next = 0;
    track->prev = 0;
    trackhead = track;
  }
  else
  {
    text_tracker_t* iter = trackhead;
    while( iter->next != 0 && iter->next->linenbr < linenbr )
      iter = iter->next;
    track->next = 0;
    track->prev = iter;
    iter->next = track;
  }
  tracktail = track;
}

void
clear_screen()
{
  L4Re::Video::View::Info info;
  fb->view_info( &info );
  l4re_video_view_info_t info_t;
  Info_to_type( &info, &info_t );
  int x = 0;
  const char* txt = "                                                     ";
  for( int y = 0; y < 768 ; y+=15)
  {

    void *addr = pixel_address(x, y, info);
    gfxbitmap_font_text( addr, 
	&info_t,
	GFXBITMAP_DEFAULT_FONT,
	txt,
	GFXBITMAP_USE_STRLEN,
	0, 0,
	1, 0
	);
  }
}

int
scroll_page_up( unsigned linenbr)
{
  clear_screen();
  unsigned bottomLine = linenbr - LINES_PER_PAGE;
  unsigned topLine = bottomLine - LINES_PER_PAGE;
  if( (int)topLine < 0 )
  {
    topLine = 0;
    bottomLine = topLine + LINES_PER_PAGE;
  }

  printf( "last: %u, first: %u, linenbr: %u\n", bottomLine, topLine, linenbr );
  L4Re::Video::View::Info info;

  if( int r = fb->view_info( &info ) )
  {
    printf( "scrollup: error while obtaining view: %i\n", r );
    return -1;
  }

  l4re_video_view_info_t info_t;
  Info_to_type( &info, &info_t );
  int x = 0;
  
  text_tracker_t* iter = trackhead;

  while( iter->linenbr != topLine )
    iter = iter->next;

  for( int y = 0; y < 768 ; y+=15)
  {

    void *addr = pixel_address(x, y, info);
    gfxbitmap_font_text( addr, 
	&info_t,
	GFXBITMAP_DEFAULT_FONT,
	iter->text,
	GFXBITMAP_USE_STRLEN,
	0, 0,
	1, 16
	);
    iter = iter->next;
  }

  return 0;
}



int
scroll_page_down( unsigned linenbr )
{
  clear_screen();
  unsigned topLine = linenbr; 
  unsigned bottomLine = topLine + LINES_PER_PAGE;
  text_tracker_t* iter = tracktail;
  if( iter->linenbr < bottomLine )
  {
    bottomLine = iter->linenbr;
    topLine = bottomLine - LINES_PER_PAGE;
  }

  printf( "last: %u, first: %u, linenbr: %u\n", bottomLine, topLine, linenbr );
  L4Re::Video::View::Info info;

  if( int r = fb->view_info( &info ) )
  {
    printf( "scrollup: error while obtaining view: %i\n", r );
    return -1;
  }

  l4re_video_view_info_t info_t;
  Info_to_type( &info, &info_t );
  int x = 0;

  while( iter->linenbr != topLine )
    iter = iter->prev;
  
  for( int y = 0; y < 768 ; y+=15)
  {

    void *addr = pixel_address(x, y, info);
    gfxbitmap_font_text( addr, 
	&info_t,
	GFXBITMAP_DEFAULT_FONT,
	iter->text,
	GFXBITMAP_USE_STRLEN,
	0, 0,
	1, 20 
	);
    iter = iter->next;
  }

  return 0;
}


  
void
fb_client()
{
  //enter_kdebug("fbclient");
  printf( "new goos\n" );
  fb =  new L4Re::Util::Video::Goos_fb("fb");
  printf("---------hello fb\n");
  base = fb->attach_buffer();
  printf("goos_fb started, buffer attached: %p\n", base);
  
  L4Re::Video::View::Info info;
  printf("try to obtain view info\n");
  
  int r = fb->view_info( &info );
  if( r != 0) printf( "error while obtaining view_info\n");
  printf("view obtained\n");

  //gfxbitmap_color_t ba = 16;
  //gfxbitmap_color_t fo = 1;
  l4re_video_view_info_t info_t; 
  
  Info_to_type( &info, &info_t);

  if( 0 != gfxbitmap_font_init() )
    printf("gfxbitmap_font_init failed.\n");
  printf("init succeeded\n"); 
  //gfxbitmap_color_pix_t back = gfxbitmap_convert_color( &info_t , ba );
  //gfxbitmap_color_pix_t fore = gfxbitmap_convert_color( &info_t , fo );
  printf("convert_color done\n"); 
  
  //keep track of the printed lines
  int incr = 0;

  //print text with gfxbitmap_font_text
  const char *text = "La Le Lu, der Mond ist doof!";
  int x = 0;
  for( int y = 0; y < 768 ; y+=15)
  {

    void *addr = pixel_address(x, y, info);
    gfxbitmap_font_text( addr, 
	&info_t,
	GFXBITMAP_DEFAULT_FONT,
	text,
	GFXBITMAP_USE_STRLEN,
	0, 0,
	1, 16
	);
    add_Line( incr, text );
    incr++;
  }

  printf("font_text done\n");

  scroll_page_up( incr );
  scroll_page_down( incr );
  // History needs: Line count tailored to the text in the line
  // information about the currently showed lines
  //
}


// lines per page is dependent on the fontsize and y-value of the screen
// maximal scrollback?
// scrolling = 1page -1 ?
// updating line numbers if linenbr reaches over 30k
// scrolling: current line nbr - lines per page
// scrollback -fore command?
//


int main(int , char** )
{
  const char *msg = "Hello Server";
  fprintf( stdout, "Hello World! I am your client!\n" );
  
  Hello* he_srv = new Hello();
  if(  he_srv->show( msg ) )
  {
    printf( "An error occured!\n" );
    return 1;
  }
  delete he_srv;

  fb_client();
  printf( "EOF\n" );

  return 0;
}
