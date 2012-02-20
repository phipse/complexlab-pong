/* Maintainer:	Philipp Eppelt
 * Date:	20/02/2012
 * Purpose:	Framebuffer Server
 */

#include <cstdio>

#include <l4/hello_srv/Fb_server.h>
#include <l4/sys/kdebug.h>



void
Fb_server::add_Line( unsigned linenbr, const char* text )
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



int
Fb_server::scroll_page_up( unsigned linenbr )
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

  if( int r = fb->view_info( &info ) )
  {
    printf( "scrollup: error while obtaining view: %i\n", r );
    return -1;
  }

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
Fb_server::scroll_page_down( unsigned linenbr )
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

  if( int r = fb->view_info( &info ) )
  {
    printf( "scrollup: error while obtaining view: %i\n", r );
    return -1;
  }

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



void* 
Fb_server::pixel_address(int x, int y , L4Re::Video::View::Info info)
{
//  unsigned bytes_per_line = info.bytes_per_line;
  void *addr =(void*) ((char*) base +
      ( y * info.bytes_per_line ) + 
      ( x * info.pixel_info.bytes_per_pixel()) );
  return addr;
}



void
Fb_server::Info_to_type( L4Re::Video::View::Info *in, l4re_video_view_info_t *out )
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



void
Fb_server::clear_screen()
{
  fb->view_info( &info );
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
Fb_server::fb_server()
{
  //enter_kdebug("fbclient");
  printf( "new goos\n" );

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

  return 0;
}



Fb_server::Fb_server()
{
  fb =  new L4Re::Util::Video::Goos_fb("fb");
  printf("---------hello fb\n");
  base = fb->attach_buffer();
  printf("goos_fb started, buffer attached: %p\n", base);
  
  printf("try to obtain view info\n");
  int r = fb->view_info( &info );
  if( r != 0) printf( "error while obtaining view_info\n");
  printf("view obtained\n");
  
  // init tracking vars
  trackhead = 0;
  tracktail = 0;
}
