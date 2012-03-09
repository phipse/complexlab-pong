#ifndef FB_SERVER_H
#define FB_SERVER_H


#include <l4/re/util/video/goos_fb>
#include <l4/re/video/goos>
#include <l4/re/video/colors>
#include <l4/re/video/view>
#include <l4/libgfxbitmap/bitmap.h>
#include <l4/libgfxbitmap/font.h>

typedef struct text_tracker_t
{
  unsigned linenbr;
  char* text;
  struct text_tracker_t* next;
  struct text_tracker_t* prev;
} text_tracker_t;



class Fb_server
{
  public: 
    Fb_server();
    void clear_screen();
    void printChar( char ch );
    void printLn( char* txt );
    void printPage( unsigned startnbr );
    void addLine( text_tracker_t* track );
    void scrollPageUp( );
    void scrollPageDown( );
    void regKeyboard();
    void run();

  private:

    void* pixel_address( int x, int y, L4Re::Video::View::Info info );
    void Info_to_type( L4Re::Video::View::Info *in, 
	    l4re_video_view_info_t *out );
    
    l4_addr_t ds_start;
    l4_size_t ds_size;
    L4Re::Video::View::Info info;
    l4re_video_view_info_t info_t;

    // variables for the history / text tracking
    text_tracker_t* trackhead;
    text_tracker_t* tracktail;
    unsigned currentLine;    

    // often used
    unsigned screenHeight;
    unsigned defaultFontHeight;
    unsigned linesPerPage;
};

#endif //FB_SERVER_H
