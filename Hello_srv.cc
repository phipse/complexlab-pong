#include <cstdio>
#include <cstring>

#include <l4/re/env>
#include <l4/sys/capability>
#include <l4/cxx/ipc_stream>
#include <l4/cxx/ipc_server>
#include <l4/re/util/object_registry>
#include <l4/re/util/meta>

#include "shared.h"

static L4Re::Util::Registry_server<> reg_server;

class Hello_srv : public L4::Server_object
{
  public: 
    int dispatch( l4_umword_t o, L4::Ipc::Iostream &ios );
};

int Hello_srv::dispatch( l4_umword_t o, L4::Ipc::Iostream &ios )
{
  printf( "dispatch!\n" );
  l4_msgtag_t tag;
  ios >> tag;
//  printf( "tag.label: %li\n", tag.label() );
  // check protocol
  switch( tag.label() )
  {
    case L4::Meta::Protocol:
      {
	printf( "Meta Protocol\n");
	return L4::Util::handle_meta_request<L4::Factory>(ios);
      }
    case L4::Factory::Protocol:
      {
	unsigned op;
	ios >> op;
	if( op != 0 ) return -L4_EINVAL;
	
	Hello_srv *server_obj = new Hello_srv();
	reg_server.registry()->register_obj( server_obj );
	ios << server_obj->obj_cap();
	printf( "Factory protocol\n");
	return L4_EOK;
      }
    default:
      {
	int cl = 0;
	ios >> cl;
	printf( "clinet says: %i \n", cl );
	return -L4_ENOSYS;
      }
  }
/*    printf( "output msg recieved\n" );
    unsigned long strsize = 0;
    char *ms = 0;
    ios >> L4::Ipc::Buf_in<char>( ms, strsize );
    printf( "strsize: %lu\n", strsize );
    printf( "MS: %s\n", ms );
  } 
  else 
    printf( "Msg recieved. Unknown opcode!\n" );
*/
  return 0;
}




int main( void )
{
  static Hello_srv he_srv;

  if( !reg_server.registry()->register_obj( 
	&he_srv, "hello_server").is_valid() )
  {
    printf( "Could not register service, readonly namespace?\n");
    return 1;
  }

  printf( "Hello World! I am your server!\n" );

  reg_server.loop();

  return 0;
}
