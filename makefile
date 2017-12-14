CC=gcc
INCLUDE=.
CFLAGS=-I$(INCLUDE)
OBJS=networking.o gossip.o util.o message/messager.o

all: networking_test

networking.o: networking.c networking.h util.h
	$(CC) -c networking.c -levent -g
	
gossip.o: gossip.c gossip.h
	$(CC) -c gossip.c -levent -g
	
util.o: util.c util.h gossip.h
	$(CC) -c util.c -g
	
messager.o: message/messager.c message/messager.h networking.h
	$(CC) $(CFLAGS) -c -o message/messager.o message/messager.c 
	
networking_test.o: test/networking_test.c
	$(CC) $(CFLAGS) -c -o test/networking_test.o test/networking_test.c -levent

networking_test: $(OBJS) test/networking_test.o
	$(CC) -o test/networking_test  $(OBJS) test/networking_test.o -levent
	
messager_test.o: test/messager_test.c
	$(CC) $(CFLAGS) -c -o test/messager_test.o test/messager_test.c -levent

messager_test: test/messager_test.o $(OBJS)
	$(CC) $(CFLAGS) -o test/messager_test $(OBJS) test/messager_test.o -levent
	
messager_test_g: test/messager_test.c
	$(CC) $(CFLAGS) -g -o test/messager_test_g test/messager_test.c message/messager.c   \
	networking.c util.c gossip.c -levent
	
clean:
	rm $(OBJS) test/networking_test test/*.o test/messager_test test/messager_test_g 
	
.PHONY: clean