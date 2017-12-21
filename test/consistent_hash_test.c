/*
 *  consistent_hash_test.c
 *
 *  Created on: Dec 21, 2017
 *      Author: dmcl216
 */

#include "chash/chash.h"
#include "config.h"
#include "util.h"
#include "gossip.h"
#include "message/messager.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

struct chash_store_s *ch_store;
struct gossiper_s *g_gossiper;

void client() {
    ch_store->node_insert(ch_store);
}

void server() {
    int listener = tcp_init(35696, 16);
    fprintf(stdout, "listener: %d\n", listener);

    struct event_base *base = event_base_new();
    struct event *listen_event = event_new(base, listener, EV_READ|EV_PERSIST,
                                    accept_cb, (void*)base);
    event_add(listen_event, NULL);
    event_base_dispatch(base);
    return ;
}

int main(int argc, char **argv) {
    g_gossiper = malloc(sizeof(struct gossiper_s));
    gossiper_open(g_gossiper);
    g_gossiper->gossiper_init(g_gossiper);

    ch_store = malloc(sizeof(struct chash_store_s));
    chash_store_open(ch_store);
    ch_store->chash_store_init(g_gossiper, ch_store);

    if (argc > 1 && !strcmp(argv[1], "-c")) {
        fprintf(stdout, "client\n");
        client();
    } else if (argc > 1 && !strcmp(argv[1], "-s")) {
        fprintf(stdout, "server\n");
        server();
    }

    return 0;
}
