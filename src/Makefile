CC	= clang
INSTALL_DIR	= /opt/ats/libexec/trafficserver
CFLAGS	= -g -Wall -I/opt/local/include -I/opt/ats/include -I/opt/fd/include -I/Users/chengyi/Documents/learn/trafficreport/trplugin/include -mcx16 -fPIC -c
LDFLAGS	= -g -L/opt/ats/lib -L/opt/fd/lib -ltsconfig -ltsmgmt -ltsutil -lfdcore -lfdproto -bundle -flat_namespace -undefined suppress
SRC_PLUGIN	= trplugin.c dc.c ta_cli.c test_app.c ta_dict.c
OBJS	= $(SRC_PLUGIN:.c=.o)
TR_EXE	= trplugin.so

all: $(TR_EXE)

$(TR_EXE): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o *.so

install:
	install_name_tool -change libfdcore.6.dylib /opt/fd/lib/libfdcore.6.dylib $(TR_EXE)
	install_name_tool -change libfdproto.6.dylib /opt/fd/lib/libfdproto.6.dylib $(TR_EXE)
	cp $(TR_EXE) $(INSTALL_DIR)/$(TR_EXE)
	chmod 0755 $(INSTALL_DIR)/$(TR_EXE)
	install_name_tool -change libfdproto.6.dylib /opt/fd/lib/libfdproto.6.dylib /opt/fd/lib/libfdcore.6.dylib

uninstall:
	rm $(INSTALL_DIR)/$(TR_EXE)
