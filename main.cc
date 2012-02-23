/* Maintainer:	Philipp Eppelt
 * Date:	20/02/2012
 * Purpose:	Implementation of the Client side for Hello-Client-Server
 *		example, the framebuffer assignment including scrolling back
 *		and forth in pagesize steps.
 */

#include <cstdio>
#include <cstring>

#include <l4/re/env>
#include <l4/re/util/meta>
#include <l4/sys/capability>
#include <l4/cxx/ipc_stream>
#include <l4/re/util/object_registry>
#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>

#include <l4/hello_srv/Fb_server.h>
#include <l4/hello_srv/keyboard.h>


class Hello 
{
  public:
  int show( char const *str );

  private:
  
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
  l4_msgtag_t res = io.call( srv.cap() );
  if( l4_ipc_error( res, l4_utcb() ) )
      return 1;

  return 0; 
}



int main(int , char** )
{
  char *msg = "Hello Server\n";
  fprintf( stdout, "Hello World! I am your client!\n" );
  
  Hello* he_srv = new Hello();
  if(  he_srv->show( msg ) )
  {
    printf( "An error occured!\n" );
    return 1;
  }
  delete he_srv;
/*
  Fb_server* fbsrv = new Fb_server();
  fbsrv->fb_server();
  fbsrv->printLn( msg );
  fbsrv->printLn( msg );
  fbsrv->printLn( msg );
  fbsrv->printLn( msg );
  fbsrv->printLn( msg );
  fbsrv->printLn( msg );
  fbsrv->printLn( msg );
  fbsrv->scrollPageUp();
  fbsrv->printLn( msg );
  */
  
  KeyboardServer* keyserv = new KeyboardServer();

  printf( "EOF\n" );

  return 0;
}
