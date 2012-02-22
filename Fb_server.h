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
  const char* text;
  struct text_tracker_t* next;
  struct text_tracker_t* prev;
} text_tracker_t;



class Fb_server
{
  public: 
    Fb_server();
    int fb_server();
    void clear_screen();
    void printChar( char ch );
    void printLn( const char* txt );
    void printPage( unsigned startnbr );
    void addLine( unsigned linenbr, const char* text );
    void scrollPageUp( );
    void scrollPageDown( );

  private:

    void* pixel_address( int x, int y, L4Re::Video::View::Info info );
    void Info_to_type( L4Re::Video::View::Info *in, 
	    l4re_video_view_info_t *out );
    
    L4Re::Util::Video::Goos_fb* fb;
    void* base;
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
