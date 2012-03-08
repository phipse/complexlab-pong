/* Maintainer:	Philipp Eppelt
 * Date:	20/02/2012
 * Purpose:	Framebuffer, history and scrolling
 */

#include <cstdio>
#include <cstring>

#include <l4/together/Fb_server.h>
#include <l4/sys/kdebug.h>
#include <l4/re/env>
#include <l4/cxx/ipc_stream>
#include <l4/re/util/cap_alloc>
#include <pthread-l4.h>


#define DEBUG 0
  
void
Fb_server::addLine( text_tracker_t* track )
{
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



void
Fb_server::printChar( char ch )
{
  /* adds a new character to the current line */
  text_tracker_t* newTrack;
  static bool lastNewLine = true;
  
  if( lastNewLine || !tracktail )
  {
    newTrack = new text_tracker_t();
    newTrack->text = 0;
    newTrack->linenbr = currentLine++;
  }
  else
    newTrack = tracktail;
  
#if DEBUG 
    printf( "text: %p, text: %s, ch: %c\n", &(newTrack->text), 
	newTrack->text, ch);
#endif
  int len = 0;
  char old = ' ';
  if( newTrack->text != 0 )
  {
    len = strlen( newTrack->text );
    if( len == 80 )
    {
      old = ch;
      ch = '\n';
    }
//    printf( "  len: %i \n", len );
    char* tmp = new char[ len + 2 ];
//    printf( "  tmp: %p\n", tmp );
    strcpy( tmp, newTrack->text );
    tmp[len] = ch;
    tmp[len+1] = 0;
//    printf( "  delete: %p\n", &(newTrack->text) );
    delete newTrack->text;
    newTrack->text = tmp;
  }
  else
  {
    newTrack->text = new char[2];
    newTrack->text[0] = ch;
    newTrack->text[1] = 0;
  }

  unsigned topLine = currentLine - linesPerPage + 1;
  if( (int) topLine < 0 ) topLine = 0;
  if( lastNewLine )
    addLine( newTrack );
  printPage( topLine );
  ch == '\n' ? lastNewLine = true : lastNewLine = false;
  if( len == 80 ) printChar( old ); 
}



void
Fb_server::printLn( char* txt )
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

  text_tracker_t* newTrack = new text_tracker_t();
  newTrack->linenbr = ++bottomLine;
  newTrack->text = txt;
  
  addLine( newTrack );
  currentLine = bottomLine;
  printPage( newFirstLine );
}



void
Fb_server::printPage( unsigned startnbr )
{
  text_tracker_t* iter = tracktail;
  while( iter->linenbr != startnbr )
    iter = iter->prev;
  int x = 0;
  for( unsigned y = 0; iter != 0 && y < screenHeight; y += defaultFontHeight )
  {
    void *addr = pixel_address(x, y, info);
    gfxbitmap_font_text( addr, 
	&info_t,
	GFXBITMAP_DEFAULT_FONT,
	iter->text,
	GFXBITMAP_USE_STRLEN,
	0, 0,
	0xffffff, 0
	);
    iter = iter->next;
  }
} 



void
Fb_server::scrollPageUp( )
{
  clear_screen();
  unsigned bottomLine = currentLine - linesPerPage;
  unsigned topLine = bottomLine - linesPerPage;
  if( (int)topLine < 0 )
  {
    topLine = 0;
    bottomLine = topLine + linesPerPage;
  }
  currentLine = bottomLine;
  printPage( topLine );
}



void
Fb_server::scrollPageDown( )
{
  clear_screen();
  unsigned topLine = currentLine; 
  unsigned bottomLine = topLine + linesPerPage;
  text_tracker_t* iter = tracktail;
  if( iter->linenbr < bottomLine )
  {
    bottomLine = iter->linenbr;
    topLine = bottomLine - linesPerPage;
    if( (int) topLine < 0 ) topLine = 0;
  }
  currentLine = bottomLine;

  printPage( topLine );
}



void* 
Fb_server::pixel_address(int x, int y , L4Re::Video::View::Info info)
{
//  unsigned bytes_per_line = info.bytes_per_line;
  void *addr =(void*) ((char*) ds_start +
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
  memset( (void*)ds_start, 0 , ds_size ); 
}



Fb_server::Fb_server()
{
  L4::Cap<L4Re::Video::Goos> fb_cap = 
    L4Re::Env::env()->get_cap<L4Re::Video::Goos>("goosfb");

  L4Re::Util::Video::Goos_fb fb( fb_cap );
  long err;
  ds_size = fb.buffer()->size();

  err = L4Re::Env::env()->rm()->attach( &ds_start, ds_size,
      L4Re::Rm::Search_addr, fb.buffer() );

  if( err )
    printf( "Couldn't attach goos_fb ds: %li\n", err );

  err = fb.view_info( &info );

  if( err )
    printf( "Failed to obtain view info: %li\n", err );
  
  Info_to_type( &info, &info_t);
  
  if( 0 != gfxbitmap_font_init() )
    printf("gfxbitmap_font_init failed.\n");
  
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

void
Fb_server::regKeyboard()
{
  L4::Cap<void> keyb = L4Re::Env::env()->get_cap<void>( "keyb" );
  if( !keyb.is_valid() )
    printf( "Invalid keyboard capability!\n" );
  
  L4::Ipc::Iostream io( l4_utcb() );
  io << 1 << L4Re::Env::env()->main_thread();
  l4_msgtag_t tag = io.call( keyb.cap() );
  
  if( tag.has_error() )
    printf( "Failed to register with keyboard server: %lu\n", 
	l4_utcb_tcr()->error );
}


void
Fb_server::run()
{
  L4::Ipc::Istream in( l4_utcb() );
//  clear_screen();

  while( true ) 
  {
    l4_umword_t sender = 0;
    in.wait( &sender );
    char rec;
    in.get( rec );
//    printf( "drawing: %c\n", rec );
    printChar( rec );
    in.reset();
  }
}

int main( void )
{
  Fb_server* debug = new Fb_server();
  debug->regKeyboard();
  debug->run();

  return 0;
}
