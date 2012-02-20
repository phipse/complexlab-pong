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
    track->next = 0;
    track->prev = tracktail;
    tracktail->next = track;
  }
  tracktail = track;
}



int
Fb_server::scroll_page_up( )
{
  clear_screen();
  unsigned bottomLine = tracktail->linenbr - linesPerPage;
  unsigned topLine = bottomLine - linesPerPage;
  if( (int)topLine < 0 )
  {
    topLine = 0;
    bottomLine = topLine + linesPerPage;
  }
  printf( "last: %u, first: %u, linenbr: %u\n", bottomLine, topLine, tracktail->linenbr );

  int x = 0;
  text_tracker_t* iter = trackhead;

  while( iter->linenbr != topLine )
    iter = iter->next;

  for( unsigned y = 0; y < screenHeight; y += defaultFontHeight )
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
Fb_server::scroll_page_down( )
{
  clear_screen();
  unsigned topLine = tracktail->linenbr; 
  unsigned bottomLine = topLine + linesPerPage;
  text_tracker_t* iter = tracktail;
  if( iter->linenbr < bottomLine )
  {
    bottomLine = iter->linenbr;
    topLine = bottomLine - linesPerPage;
  }
  printf( "last: %u, first: %u, linenbr: %u\n", bottomLine, topLine, tracktail->linenbr );
  int x = 0;

  while( iter->linenbr != topLine )
    iter = iter->prev;
  
  for( unsigned y = 0; y < screenHeight; y += defaultFontHeight )
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
  int x = 0;
  const char* txt = "                                                     ";
  for( unsigned y = 0; y < screenHeight; y += defaultFontHeight )
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
  //gfxbitmap_color_t ba = 16;
  //gfxbitmap_color_t fo = 1;

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
  for( unsigned y = 0; y < screenHeight; y += defaultFontHeight )
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

  return 0;
}

void
Fb_server::printLn( const char* txt )
{
  
  unsigned bottomLine = 0;
  unsigned newFirstLine = 0;
  if( tracktail != 0 )
  {
    bottomLine = tracktail->linenbr;
    newFirstLine = bottomLine - linesPerPage + 1;
    if( (int)newFirstLine < 0 )
      newFirstLine = 0;
  }
  else
    newFirstLine = bottomLine;
  
  add_Line( ++bottomLine, txt );
  text_tracker_t* iter = tracktail;
  while( iter->linenbr != newFirstLine )
    iter = iter->prev;

  int x = 0;
  for( unsigned y = 0; y < screenHeight; y += defaultFontHeight )
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
  
  Info_to_type( &info, &info_t);
  
  // init tracking vars
  trackhead = 0;
  tracktail = 0;
    
  screenHeight = info.height;
  defaultFontHeight = gfxbitmap_font_height( GFXBITMAP_DEFAULT_FONT );
  if( defaultFontHeight == 0 )
    defaultFontHeight = 15;
  printf( "ScreenHeigt: %u, fontHeight: %u\n", screenHeight, defaultFontHeight );
  linesPerPage = screenHeight / defaultFontHeight;
}
