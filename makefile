CC=gcc
INCLUDE=.
OBJS=networking.o gossip.o util.o

all: networking_test

networking.o: networking.c networking.h util.h
	$(CC) -c networking.c -levent -g
	
gossip.o: gossip.c gossip.h
	$(CC) -c gossip.c -levent -g
	
util.o: util.c util.h gossip.h
	$(CC) -c util.c -g
	
networking_test.o: test/networking_test.c
	$(CC) -c test/networking_test.c -levent -I $(INCLUDE) -o test/networking_test.o

networking_test: $(OBJS) networking_test.o
	$(CC) -o test/networking_test  $(OBJS) test/networking_test.o -levent
	
clean:
	rm  $(OBJS) test/networking_test test/networking_test.o
	
.PHONY: clean