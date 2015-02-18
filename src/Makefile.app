CC	= gcc
TR_INSTALL_DIR	= /opt/ats/libexec/trafficserver
DS_INSTALL_DIR	= /opt/fd/lib/freeDiameter
TR_SRC_DIR = /home/ec2-user/trplugin/trplugin
CFLAGS	= -g -Wall -I/opt/local/include -I/opt/ats/include -I/opt/fd/include -I../include -mcx16 -fPIC -c
LDFLAGS	= -g -L/opt/ats/lib -L/opt/fd/lib -ltsconfig -ltsmgmt -ltsutil -lfdcore -lfdproto -bundle -flat_namespace -undefined suppress
DS_LDFLAGS=-g -L/opt/fd/lib -lfdcore -lfdproto -lpthread -shared

SRC_PLUGIN	= trplugin.c dc.c ta_cli.c test_app.c ta_dict.c ta_serv_dummy.c definition.c
SRC_DS	= ta_serv.c test_app.c ta_dict.c ta_cli_dummy.c definition.c

OBJS_PLUGIN	= $(SRC_PLUGIN:.c=.o)
OBJS_DS	= $(SRC_DS:.c=.o)

TR_EXE	= trplugin.so
DS_EXE	= ds.fdx

all: $(TR_EXE) $(DS_EXE)

$(TR_EXE): $(OBJS_PLUGIN)
	$(CC) $(LDFLAGS) $(OBJS_PLUGIN) -o $@

$(DS_EXE): $(OBJS_DS)
	$(CC) $(DS_LDFLAGS) $(OBJS_DS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o *.so *.fdx

install:
	install_name_tool -change libfdproto.6.dylib /opt/fd/lib/libfdproto.6.dylib /opt/fd/lib/libfdcore.6.dylib

	install_name_tool -change libfdcore.6.dylib /opt/fd/lib/libfdcore.6.dylib $(TR_EXE)
	install_name_tool -change libfdproto.6.dylib /opt/fd/lib/libfdproto.6.dylib $(TR_EXE)

	install_name_tool -change libfdcore.6.dylib /opt/fd/lib/libfdcore.6.dylib $(DS_EXE)
	install_name_tool -change libfdproto.6.dylib /opt/fd/lib/libfdproto.6.dylib $(DS_EXE)
	
	cp $(TR_EXE) $(TR_INSTALL_DIR)/$(TR_EXE)
	chmod 0755 $(TR_INSTALL_DIR)/$(TR_EXE)
	cp $(DS_EXE) $(DS_INSTALL_DIR)/$(DS_EXE)
	chmod 0755 $(DS_INSTALL_DIR)/$(DS_EXE)

uninstall:
	rm $(TR_INSTALL_DIR)/$(TR_EXE)
	rm $(DS_INSTALL_DIR)/$(DS_EXE)