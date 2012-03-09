#include <cstdio>
#include <vector>

#include <l4/cxx/ipc_stream>
#include <l4/cxx/ipc_server>
#include <l4/sys/capability>
#include <l4/re/util/video/goos_fb>
#include <l4/re/util/video/goos_svr>
#include <l4/re/util/dataspace_svr>
#include <l4/re/util/meta>
#include <l4/re/util/object_registry>
#include <l4/re/env>

#include <l4/Console/SessionServer.h>

L4::Cap<void> rcv_cap;

class Cap_hooks :
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



class VDS :
   public L4::Server_object,
   public L4Re::Util::Dataspace_svr
{
  public:
    VDS( l4_addr_t addr, l4_size_t sz);
    ~VDS() throw() {}
  
    int changeFb();
    void unmap();
    void dsswitch();
    void copy( l4_addr_t oldstart );
    int dispatch( l4_umword_t o, L4::Ipc::Iostream &io )
    {
      o = o | L4_FPAGE_X;
      return Dataspace_svr::dispatch( o, io );
    }

    l4_addr_t dummyStart;
    l4_addr_t realStart;

  private: 
    L4::Cap<L4Re::Dataspace> myDummyDs; 
}; 


class VFB : 
  public L4::Server_object, 
  public L4Re::Util::Video::Goos_svr
{ // Framebuffer server object for every client. Needs to be created with the 
  // information of the goos framebuffer.

  public:
    VFB();

    void changeDs();
    void init( L4::Cap<void> cap, 
	L4Re::Video::Goos::Info gi, 
	L4Re::Video::View::Info vi);

    int dispatch( l4_umword_t o, L4::Ipc::Iostream &io )
    {// just forward
      return Goos_svr::dispatch( o, io );
    }


  private:
    VDS* _vds;
};

L4Re::Util::Video::Goos_fb* _fb;
void* _fbuffer;
L4Re::Video::Goos::Info _gooInfo;
L4Re::Video::View::Info _info;

#if 0
int
Wrapper_so::dispatch( l4_umword_t obj , L4::Ipc::Iostream &io )
{
}
#endif
