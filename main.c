/*
 * main.c
 *
 *  Created on: Dec 12, 2017
 *      Author: dmcl216
 */

struct chash_store_s *ch_store;
struct gossiper_s *g_gossiper;

#include "chash/chash.h"
#include "networking.h"
#include "config.h"

#include <event2/event.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void on_start();
void timeout_cb(int fd, short ev, void *arg);
void cmd_cb(int fd, short ev, void *arg);

int main(int argc, char **argv) {

	return 0;
}

void on_start() {
    g_gossiper = malloc(sizeof(struct gossiper_s));
    gossiper_open(g_gossiper);
    g_gossiper->gossiper_init(g_gossiper);

    ch_store = malloc(sizeof(struct chash_store_s));
    chash_store_open(ch_store);
    ch_store->chash_store_init(g_gossiper, ch_store);


    int listener = tcp_init(SRVPORT, LISTEN_NUM);
    fprintf(stdout, "[info]main:listen %d\n", listener);

    struct event_base *base = event_base_new();
    struct event *listen_event = event_new(base, listener, EV_READ | EV_PERSIST,
                                                accept_cb, (void*) base);
    struct event *cmd_event =
    struct event *time_event
    event_add(listen_event, NULL);
    event_base_dispatch(base);
    return;
}
