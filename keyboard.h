#ifndef KEYBOARD_SERVER_H
#define KEYBOARD_SERVER_H

L4::Cap<void> rcv_cap;
L4::Cap<void> _clnt[3];
static int current = 0;

class Keyboard_hooks :
  public L4::Ipc_svr::Ignore_errors,
  public L4::Ipc_svr::Default_timeout,
  public L4::Ipc_svr::Compound_reply
{
public:
  static void setup_wait(L4::Ipc::Istream &istr, bool)
  {
    rcv_cap = L4Re::Util::cap_alloc.alloc<void>();
    istr.reset();
    istr << L4::Ipc::Small_buf(rcv_cap.cap(), L4_RCV_ITEM_LOCAL_ID);
    l4_utcb_br()->bdr = 0;
  }

};



class KeyboardServer : public L4::Server_object
{
  public:
    int dispatch( l4_umword_t obj, L4::Ipc::Iostream &ios );

  private:
    int connected;
};



class KeyboardIrq : public L4::Server_object
{
  public:
    int dispatch( l4_umword_t o, L4::Ipc::Iostream &io )
    {
      return 0;
    }
    void receive();
    char readScanCode();
    void pushAll( const char ch );
    
  private:
    bool release;
};



class SessionServer : public L4::Server_object
{

  public: 
    static L4Re::Util::Registry_server<Keyboard_hooks> server;
    int dispatch( l4_umword_t , L4::Ipc::Iostream &ios )
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

	    KeyboardServer *server_obj = new KeyboardServer();
	    server.registry()->register_obj( server_obj );
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
    }
};

L4Re::Util::Registry_server<Keyboard_hooks> SessionServer::server;

#endif //KEYBOARD_SERVER_H
