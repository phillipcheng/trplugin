CC	= gcc
ATS_DIR=/opt/ats
FD_DIR=/opt/fd
TR_INSTALL_DIR	= /opt/tr
CFLAGS	= -g -Wall -I/opt/local/include -I/opt/ats/include -I/opt/fd/include -I/usr/local/include -I../include -mcx16 -fPIC -c
LDFLAGS	= -g -L/opt/ats/lib -L/opt/fd/lib -L/usr/local/lib -ltsconfig -ltsmgmt -ltsutil -lfdcore -lfdproto -lpthread -lconfuse -shared

SRC_PLUGIN	= trplugin.c trconfig.c ta_cli.c ta_dict.c definition.c srv_resource.c timeout_thread.c

OBJS_PLUGIN	= $(SRC_PLUGIN:.c=.o)

TR_EXE	= trplugin.so
TR_CONF = ../conf/tr.conf ../conf/fd1.conf
TR_PLUGIN_CONF = ../conf/plugin.config ../conf/records.config

all: $(TR_EXE)


$(TR_EXE): $(OBJS_PLUGIN)
	$(CC) $(LDFLAGS) $(OBJS_PLUGIN) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o *.so

install:
	mkdir -p $(TR_INSTALL_DIR)
	mkdir -p $(TR_INSTALL_DIR)/lib
	mkdir -p $(TR_INSTALL_DIR)/conf
	cp $(TR_EXE) $(TR_INSTALL_DIR)/lib/$(TR_EXE)
	chmod 0755 $(TR_INSTALL_DIR)/lib/$(TR_EXE)
	cp $(TR_CONF) $(TR_INSTALL_DIR)/conf
	cp $(TR_PLUGIN_CONF) $(ATS_DIR)/etc/trafficserver/

uninstall:
	rm $(TR_INSTALL_DIR)/lib/$(TR_EXE)
