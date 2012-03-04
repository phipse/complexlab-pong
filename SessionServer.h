#pragma once

#define DEBUG 0


#if DEBUG
#include <cstdio>
#endif

#include <l4/cxx/ipc_server>
#include <l4/cxx/ipc_stream>
#include <l4/re/util/meta>



template <class TYPE, typename HOOKS = L4::Ipc_svr::Default_loop_hooks>
class SessionServer : public L4::Server_object
{

  public: 
    std::vector<TYPE> sessions;
    typename std::vector<TYPE>::iterator iter;
    static L4Re::Util::Registry_server<HOOKS> server;

    int dispatch( l4_umword_t , L4::Ipc::Iostream &ios )
    {
#if DEBUG
      printf("SessionServer hello!\n" );
#endif
      l4_msgtag_t tag;
      ios >> tag;

      // check protocol
      switch( tag.label() )
      {
	case L4::Meta::Protocol:
	  {
#if DEBUG
	    printf( "Meta Protocol\n");
#endif
	    return L4::Util::handle_meta_request<L4::Factory>(ios);
	  }
	case L4::Factory::Protocol:
	  {
	    unsigned op;
	    ios >> op;
	    if( op != 0 ) 
	      return -L4_EINVAL;

	    TYPE* server_obj = new TYPE();
	    sessions.push_back( *server_obj );
	    server.registry()->register_obj( server_obj );
	    ios << server_obj->obj_cap();
#if DEBUG
	    printf( "Factory protocol\n");
#endif
	    return L4_EOK;
	  }
	default:
	  {
	    printf( "Bad Protocol\n" );
	    return -L4_ENOSYS;
	  }
      }
    }
};

template <class TYPE, typename HOOKS>
L4Re::Util::Registry_server<HOOKS> SessionServer<TYPE, HOOKS>::server;
