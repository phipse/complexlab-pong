PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= keyboard_svr console_svr clnt debug

# list your .c or .cc files here
SRC_CC_keyboard_svr	= keyboard.cc
SRC_CC_console_svr	= console_svr.cc
SRC_CC_clnt		= Clnt.cc
SRC_CC_debug		= Fb_server.cc

# list requirements of your program here
REQUIRES_LIBS   += libc_be_mem libpthread libgfxbitmap libstdc++ libio-io 

include $(L4DIR)/mk/prog.mk
