# Makefile for LIXY (LISP on Linux for Vyatta)

CC = gcc -Wall -g

MODULES = error.o instance.o map.o maptable.o control.o
PATRICIA = patricia
LIST = list
PROGNAME = lixyd lixyctl

.PHONY: all
all: $(PROGNAME)

.c.o:
	$(CC) -c $< -o $@

modules: $(MODULES)

patricia: $(PATRICIA)/patricia.h $(PATRICIA)/patricia.h
	cd $(PATRICIA)
	gcc -c patricia.c -o patricia.o

list: $(LIST)/list.h $(LIST)/list.c
	cd $(LIST)
	gcc -c list.c list.o

lixyd: main.c $(MODULES) $(PATRICIA)/patricia.o $(LIST)/list.o
	$(CC) main.c -lcrypto -lpthread $(MODULES) \
	$(PATRICIA)/patricia.o $(LIST)/list.o -o lixyd

lixyctl: lixyctl.c common.h control.h error.o
	$(CC) lixyctl.c error.o -o lixyctl

clean : 
	rm $(MODULES) $(PATRICIA)/patricia.o $(LIST)/list.o $(PROGNAME)


error.c		: error.h
instance.h	: common.h
instance.c	: instance.h map.h maptable.h sockaddrmacro.h
map.h		: common.h
map.c		: common.h lisp.h error.h map.h sockaddrmacro.h
maptable.h	: common.h $(PATRICIA)/patricia.h
maptable.c	: maptable.h sockaddrmacro.h
common.h	: lisp.h list/list.h patricia/patricia.h
control.h	: list/list.h
control.c	: control.h instance.h sockaddrmacro.h error.h



RCDST = /etc/init.d
INSTALLDST = /usr/local/sbin
UPDATERCD = /usr/sbin/update-rc.d

install: $(PROGNAME)
	install lixyd $(INSTALLDST)
	install lixyctl $(INSTALLDST)
	install lixy $(RCDST)
	$(UPDATERCD) lixy defaults

uninstall:
	rm $(INSTALLDST)/lixyd
	rm $(INSTALLDST)/lixyctl
	$(UPDATERCD) -f lixy remove
	rm $(RCDST)/(INITSCRIPT)

VYATTA_DST = /opt/vyatta/share
install-vyatta: install
	cp -r vyatta/vyatta-cfg $(VYATTA_DST)/
#	cp -r vyatta/vyatta-op $(VYATTA_DST)/

uninstall-vyatta:  uninstall
	rm -r $(VYATTA_DST)/vyatta-cfg/templates/protocols/lisp
