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
    int dispatch( l4_umword_t o, L4::Ipc::Iostream &ios)
    {
      l4_msgtag_t tag;
      ios >> tag;

      unsigned long cl = 0;
      char *ms = 0;
      ios >> L4::Ipc::Buf_in<char>( ms, cl );
      printf( "client says: %s, size: %lu \n", ms, cl );
      return L4_EOK;
    }
};

class SessionServer : public L4::Server_object
{
  public: 
    int dispatch( l4_umword_t o, L4::Ipc::Iostream &ios );
};

int SessionServer::dispatch( l4_umword_t , L4::Ipc::Iostream &ios )
{
  printf( "dispatch!\n" );
  l4_msgtag_t tag;
  ios >> tag;

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
	printf( "Bad Protocol\n" );
	return -L4_ENOSYS;
      }
  }
  return 0;
}




int main( void )
{
  static SessionServer sessionSrv;

  if( !reg_server.registry()->register_obj( 
	&sessionSrv, "hello_server").is_valid() )
  {
    printf( "Could not register service, readonly namespace?\n");
    return 1;
  }

  printf( "Hello World! I am your server!\n" );

  reg_server.loop();

  return 0;
}
