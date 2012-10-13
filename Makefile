# Makefile for LIXY (LISP on Linux for Vyatta)

CC = gcc -Wall

MODULES = error.o instance.o map.o maptable.o
PATRICIA = patricia
LIST = list
PROGNAME = lixy

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

lixy: $(MODULES) $(PATRICIA)/patricia.o $(LIST)/list.o
	$(CC) $(MODULES) $(PATRICIA)/patricia.o $(LIST)/list.o -o lixy

clean : 
	$(MODULES) $(PATRICIA)/patricia.o $(LIST)/list.o $(PROGNAME)


error.c		: error.h
instance.h	: common.h
instance.c	: instance.h map.h maptable.h sockaddrmacro.h
map.h		: common.h
map.c		: common.h lisp.h error.h map.h sockaddrmacro.h
maptable.h	: common.h $(PATRICIA)/patricia.h
maptable.c	: maptable.h sockaddrmacro.h
common.h	: lisp.h list/list.h patricia/patricia.h

