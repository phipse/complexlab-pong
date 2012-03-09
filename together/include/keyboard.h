#ifndef KEYBOARD_SERVER_H
#define KEYBOARD_SERVER_H

#include <l4/together/SessionServer.h>
#include <list>


L4::Cap<void> rcv_cap;
std::list< L4::Cap<void> > _clnt;
std::list<char> buffer;

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
};



class KeyboardIrq : public L4::Server_object
{
  public:
    int dispatch( l4_umword_t o, L4::Ipc::Iostream &io )
    {
      (void)o; (void)io;
      return 0;
    }
    void receive();
    char readScanCode();
    
  private:
    bool release;
};


#endif //KEYBOARD_SERVER_H
