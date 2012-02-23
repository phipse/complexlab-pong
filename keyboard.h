#ifndef KEYBOARD_SERVER_H
#define KEYBOARD_SERVER_H

class KeyboardServer : public L4::Server_object
{
  public:
    KeyboardServer();
    int dispatch( l4_umword_t obj, L4::Ipc::Iostream &ios );
    bool init();
  private:
    char readScanCode( l4_msgtag_t res );
    void Action( char key );
    void Ack();

    bool release;
    Fb_server* fb;
};

#endif //KEYBOARD_SERVER_H
