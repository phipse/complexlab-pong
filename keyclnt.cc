#include <cstdio>

#include <l4/re/env>
#include <l4/sys/capability>
#include <l4/cxx/ipc_stream> 



class Keyclnt 
{
  public:
  void recieve()
  {
    L4::Ipc::Istream in( l4_utcb() );
    printf(" == receiving! == \n" );
    for(;;)
    {
      l4_umword_t sender = 0;
      in.wait(&sender);
      printf( "msg recieved!\n");
      char rec;
      in.get(rec);
      printf( "%c\n", rec);
      in.reset();
    }
  }

  int connect()
  {
    L4::Cap<void> srv = L4Re::Env::env()->get_cap<void>("keybo");
    if( !srv.is_valid() )
    {
      printf( "Could not get server capability!\n");
      return -1;
    }
    L4::Ipc::Iostream io( l4_utcb() );
    io << 1 << L4Re::Env::env()->main_thread(); 
    l4_msgtag_t res = io.call( srv.cap() );
    if( l4_ipc_error( res, l4_utcb() ) )
      return -1;
    return 0;
  }
};

int 
main( void )
{
  printf( "Hello keyclnt\n" ); 
  Keyclnt* abc = new Keyclnt();
  abc->connect(); 
  printf("connected!\n");
  abc->recieve();

  return 0; 
}
