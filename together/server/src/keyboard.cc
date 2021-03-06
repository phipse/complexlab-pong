#include <stdio.h>
#include <assert.h>

#include <x86/l4/util/port_io.h>
#include <contrib/libio-io/l4/io/io.h>
#include <l4/re/cap_alloc>
#include <l4/re/env>
#include <l4/sys/irq>
#include <pthread-l4.h>
#include <l4/re/util/object_registry>
#include <l4/cxx/ipc_server>
#include <l4/cxx/ipc_stream> 
#include <l4/re/util/meta>
#include <l4/util/util.h>

#include <l4/together/debug.h>
#include <l4/together/keyboard.h>



int
KeyboardServer::dispatch( l4_umword_t, L4::Ipc::Iostream &ios )
{
  l4_umword_t op;
  ios >> op;
  switch( op )
  {
    case 1: 
      {
	_clnt.push_back( rcv_cap );
//	printf( "client connected: %lu\n", rcv_cap.cap());
	return L4_EOK;
      }
    default:
      printf( "Unrecognised opcode: %lu \n", op );
      return -L4_ENOENT;
  }
}



void 
pushAll( const char ch, const bool release )
{ // sends the keystroke to all subscribers 
  
  L4::Ipc::Ostream out( l4_utcb() );
  out << ch << release;
  std::list< L4::Cap<void> >::iterator it;
  for( it =_clnt.begin(); it != _clnt.end(); it++ )
    out.send( it->cap() );
  out.reset();
}



void
KeyboardIrq::receive()
{ 
  printf( " ===== recieve ====== \n" );
  //subscribe to 0x1
  L4::Cap<L4::Irq> irqCap = L4Re::Util::cap_alloc.alloc<L4::Irq>();
  if(!irqCap.is_valid())
  {
    printf( "irqCap not valid!\n" );
    return ;
  }

  // bind irqCap to icu
  long res = l4io_request_irq( 0x1, irqCap.cap() );
  if( res )
  {
    printf( "l4io_request_irq failed: %li \n", res );
    return ;
  }

  L4::Cap<L4::Thread> threadCap( pthread_getl4cap( pthread_self() ) ) ;

  // attach thread to irq
  l4_msgtag_t tag = irqCap->attach(0x1 , threadCap);   
  if( tag.has_error() )
  {
    printf( "attach msgtag has error: %u \n", tag.has_error() ); 
    printf( "attach tag label: %li \n", tag.label() );
    return ;
  }

  char key = ' ';
  
  while( 1 )
  {
    tag = irqCap->receive();
    if( tag.has_error() ) 
      printf( "error: %lu \n", l4_utcb_tcr()->error );
    key = readScanCode( );
    
    buffer.push_back( key );
    rel.push_back( release );
  }
}



void* irq( void* )
{
//  printf( "hullo!\n" );
  KeyboardIrq* irq = new KeyboardIrq();
  irq->receive();
  
  printf( "There must be an recieve error in the Keyboard thread!\n" );
  return 0;
}

void* send( void* )
{
  printf( "hello send\n" );
  while( true )
    if( !buffer.empty() )
    {
      pushAll( buffer.front(), rel.front() );
      buffer.pop_front();
      rel.pop_front();
    }
    else
      l4_sleep( 10 );

  printf( "snd terminating!\n" );
  return 0;
}

int main( void )
{
  SessionServer<KeyboardServer, Keyboard_hooks>* session =
    new SessionServer<KeyboardServer, Keyboard_hooks>();
//  printf( "Hello Main!\n" );

  if( !session->server.registry()->register_obj( 
	session, "key").is_valid() )
  {
    printf( "Could not register service, readonly namespace?\n");
    return 1;
  } 

  pthread_t rec, snd;
  pthread_create( &rec, NULL, irq, NULL );
  pthread_create( &snd, NULL, send, NULL );

  session->server.loop();

  return 0;
}




char
KeyboardIrq::readScanCode( )
{
  //read from port
  l4_uint8_t scancode = l4util_in8(0x60);
  // release key will be pressed key plus 0x80
  if( scancode >= 0x80 )
  {
    release = true;
    scancode -= 0x80;
  }
  else
    release = false;


  char keycode;
  static bool shift = false;


  if( scancode == 0xff ) 
  {
    printf( "Keyboard failure! \n");
    return -1;
  }

  switch( scancode )
  {
    case 0x36:
    case 0x2a: shift = !release; break; 
  }

  if( !shift )
    switch( scancode )
    {
      case 0x02: keycode = '1'; break; 
      case 0x03: keycode = '2'; break;
      case 0x04: keycode = '3'; break;
      case 0x05: keycode = '4'; break;
      case 0x06: keycode = '5'; break;
      case 0x07: keycode = '6'; break;
      case 0x08: keycode = '7'; break;
      case 0x09: keycode = '8'; break;
      case 0x0a: keycode = '9'; break;
      case 0x0b: keycode = '0'; break;
      case 0x0c: keycode = '-'; break;
      case 0x0d: keycode = '`'; break;
//      case 0x0e: keycode = "bksp"; break; //Backspace
//      case 0x0f: keycode = "Tap"; break; //tab
      case 0x10: keycode = 'q'; break;
      case 0x11: keycode = 'w'; break;
      case 0x12: keycode = 'e'; break;
      case 0x13: keycode = 'r'; break;
      case 0x14: keycode = 't'; break;
      case 0x15: keycode = 'y'; break;
      case 0x16: keycode = 'u'; break;
      case 0x17: keycode = 'i'; break;
      case 0x18: keycode = 'o'; break;
      case 0x19: keycode = 'p'; break;
      case 0x1a: keycode = '['; break;
      case 0x1b: keycode = ']'; break;
      case 0x1c: keycode = '\n'; break;//enter
//      case 0x1d: keycode = "LCTRL"; break; //lctrl
      case 0x1e: keycode = 'a'; break;
      case 0x1f: keycode = 's'; break;
      case 0x20: keycode = 'd'; break;
      case 0x21: keycode = 'f'; break;
      case 0x22: keycode = 'g'; break;
      case 0x23: keycode = 'h'; break;
      case 0x24: keycode = 'j'; break;
      case 0x25: keycode = 'k'; break;
      case 0x26: keycode = 'l'; break;
      case 0x27: keycode = ';'; break;
      case 0x28: keycode = '\''; break;
      case 0x29: keycode = '`'; break;
//      case 0x2a: keycode = "LSHIFT"; break;	 
      case 0x2b: keycode = '\\'; break; 
      case 0x2c: keycode = 'z'; break;
      case 0x2d: keycode = 'x'; break;
      case 0x2e: keycode = 'c'; break;
      case 0x2f: keycode = 'v'; break;
      case 0x30: keycode = 'b'; break;
      case 0x31: keycode = 'n'; break;
      case 0x32: keycode = 'm'; break;
      case 0x33: keycode = ','; break;
      case 0x34: keycode = '.'; break;
      case 0x35: keycode = '/'; break;
//      case 0x36: keycode = "RSHIFT"; break;
//      case 0x38: keycode = "LALT"; break;
      case 0x39: keycode = ' '; break;
    }
  else
    switch( scancode )
    {
      case 0x02: keycode = '!'; break; 
      case 0x03: keycode = '"'; break;
      case 0x04: keycode = '3'; break;
      case 0x05: keycode = '$'; break;
      case 0x06: keycode = '%'; break;
      case 0x07: keycode = '&'; break;
      case 0x08: keycode = '/'; break;
      case 0x09: keycode = '('; break;
      case 0x0a: keycode = ')'; break;
      case 0x0b: keycode = '='; break;
      case 0x0c: keycode = '_'; break;
      case 0x0d: keycode = '`'; break;
//      case 0x0e: keycode = "Bksp"; break; //Backspace
//      case 0x0f: keycode = "TAB"; break; //tab
      case 0x10: keycode = 'Q'; break;
      case 0x11: keycode = 'W'; break;
      case 0x12: keycode = 'E'; break;
      case 0x13: keycode = 'R'; break;
      case 0x14: keycode = 'T'; break;
      case 0x15: keycode = 'Y'; break;
      case 0x16: keycode = 'U'; break;
      case 0x17: keycode = 'I'; break;
      case 0x18: keycode = 'O'; break;
      case 0x19: keycode = 'P'; break;
      case 0x1a: keycode = '{'; break;
      case 0x1b: keycode = '}'; break;
      case 0x1c: keycode = '\n'; break;//enter
//      case 0x1d: keycode = "LCTRL"; break; //lctrl"
      case 0x1e: keycode = 'A'; break;
      case 0x1f: keycode = 'S'; break;
      case 0x20: keycode = 'D'; break;
      case 0x21: keycode = 'F'; break;
      case 0x22: keycode = 'G'; break;
      case 0x23: keycode = 'H'; break;
      case 0x24: keycode = 'J'; break;
      case 0x25: keycode = 'K'; break;
      case 0x26: keycode = 'L'; break;
      case 0x27: keycode = ':'; break;
      case 0x28: keycode = '\"'; break;
      case 0x29: keycode = '~'; break; 
//      case 0x2a: keycode = "LSHIFT"; break; 
      case 0x2b: keycode = '|'; break; 
      case 0x2c: keycode = 'Z'; break;
      case 0x2d: keycode = 'X'; break;
      case 0x2e: keycode = 'C'; break;
      case 0x2f: keycode = 'V'; break;
      case 0x30: keycode = 'B'; break;
      case 0x31: keycode = 'N'; break;
      case 0x32: keycode = 'M'; break;
      case 0x33: keycode = '<'; break;
      case 0x34: keycode = '>'; break;
      case 0x35: keycode = '?'; break;
//      case 0x36: keycode = "RSHIFT"; break;
//      case 0x38: keycode = "LALT"; break;
      case 0x39: keycode = ' '; break;
    }
  
//  printf( "Shift: %i, keycode: %c, scancode: %x\n", shift, keycode, scancode );
  
  return keycode;
}
