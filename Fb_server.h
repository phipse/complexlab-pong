#ifndef FB_SERVER_H
#define FB_SERVER_H


#include <l4/re/util/video/goos_fb>
#include <l4/re/video/goos>
#include <l4/re/video/colors>
#include <l4/re/video/view>
#include <l4/libgfxbitmap/bitmap.h>
#include <l4/libgfxbitmap/font.h>

#define LINES_PER_PAGE 51

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

    void add_Line( unsigned linenbr, const char* text );
    int scroll_page_up( unsigned linenbr );
    int scroll_page_down( unsigned linenbr );

  private:

    void* pixel_address( int x, int y, L4Re::Video::View::Info info );
    void Info_to_type( L4Re::Video::View::Info *in, 
	    l4re_video_view_info_t *out );
    
    L4Re::Util::Video::Goos_fb* fb;
    void* base;
    L4Re::Video::View::Info info;

    // variables for the history / text tracking
    text_tracker_t* trackhead;
    text_tracker_t* tracktail;
    l4re_video_view_info_t info_t;
};

#endif //FB_SERVER_H
