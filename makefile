CC=gcc
INCLUDE=.
CFLAGS=-I$(INCLUDE)
LIBS=-levent -lcrypto
OBJS=networking.o gossip.o util.o message/messager.o chash/chash.o
SRC=networking.c gossip.c util.c message/messager.c chash/chash.c

all: networking_test

networking.o: networking.c networking.h util.h
	$(CC) $(CFLAGS) -c networking.c -levent -g
	
gossip.o: gossip.c gossip.h
	$(CC) $(CFLAGS) -c gossip.c -levent -g
	
util.o: util.c util.h gossip.h
	$(CC) $(CFLAGS) -c util.c -g
	
messager.o: message/messager.c message/messager.h networking.h
	$(CC) $(CFLAGS) -c -o message/messager.o message/messager.c 
	
chash.o: chash/chash.c
	$(CC) $(CFLAGS) -c -o chash/chash.o chash/chash.c
	
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
	
range_test.o: test/range_test.c util.h
	$(CC) $(CFLAGS) -c -o test/range_test.o test/range_test.c
	
range_test: test/range_test.o $(OBJS)
	$(CC) $(CFLAGS) -o test/range_test $(OBJS) test/range_test.o -levent
	
gossip_test.o: test/gossip_test.c
	$(CC) $(CFLAGS) -c -o test/gossip_test.o test/gossip_test.c
	
gossip_test: test/gossip_test.o $(OBJS)
	$(CC) $(CFLAGS) -o test/gossip_test test/gossip_test.o $(OBJS) $(LIBS)
	
gdb_gossip_test: test/gossip_test.c
	$(CC) $(CFLAGS) -g -o test/gdb_gossip_test test/gossip_test.c message/messager.c   \
	networking.c util.c gossip.c -levent
	
chash_test.o: test/consistent_hash_test.c
	$(CC) $(CFLAGS) -c -o test/chash_test.o test/consistent_hash_test.c
	
chash_test: chash_test.o $(OBJS)
	$(CC) $(CFLAGS) -o test/chash_test test/chash_test.o $(OBJS) $(LIBS)
	
gdb_chash_test: test/consistent_hash_test.c
	$(CC) $(CFLAGS) -g -o test/gdb_chash_test test/consistent_hash_test.c $(SRC) $(LIBS)
	
clean:
	rm $(OBJS) test/networking_test test/*.o test/messager_test test/messager_test_g test/gossip_test \
	   test/gdb_gossip_test test/range_test test/chash_test
	
.PHONY: clean