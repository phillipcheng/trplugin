CC	= gcc
TR_INSTALL_DIR	= /opt/ats/libexec/trafficserver
TR_SRC_DIR = /home/ec2-user/trplugin/trplugin
CFLAGS	= -g -Wall -I/opt/local/include -I/opt/ats/include -I/opt/fd/include -I../include -mcx16 -fPIC -c
LDFLAGS_APP	= -g -L/opt/ats/lib -L/opt/fd/lib -ltsconfig -ltsmgmt -ltsutil -lfdcore -lfdproto -bundle -flat_namespace -undefined suppress
LDFLAGS	= -g -L/opt/ats/lib -L/opt/fd/lib -ltsconfig -ltsmgmt -ltsutil -lfdcore -lfdproto -lpthread -shared

SRC_PLUGIN	= trplugin.c trconfig.c ta_cli.c ta_dict.c definition.c srv_resource.c timeout_thread.c

OBJS_PLUGIN	= $(SRC_PLUGIN:.c=.o)

TR_EXE	= trplugin.so

all: $(TR_EXE)

$(TR_EXE): $(OBJS_PLUGIN)
	$(CC) $(LDFLAGS) $(OBJS_PLUGIN) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o *.so *.fdx

install_app:
	install_name_tool -change libfdproto.6.dylib /opt/fd/lib/libfdproto.6.dylib /opt/fd/lib/libfdcore.6.dylib

	install_name_tool -change libfdcore.6.dylib /opt/fd/lib/libfdcore.6.dylib $(TR_EXE)
	install_name_tool -change libfdproto.6.dylib /opt/fd/lib/libfdproto.6.dylib $(TR_EXE)

install:
	cp $(TR_EXE) $(TR_INSTALL_DIR)/$(TR_EXE)
	chmod 0755 $(TR_INSTALL_DIR)/$(TR_EXE)

uninstall:
	rm $(TR_INSTALL_DIR)/$(TR_EXE)
