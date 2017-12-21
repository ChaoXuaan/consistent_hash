/*
 * messager_test.c
 *
 *  Created on: Dec 14, 2017
 *      Author: dmcl216
 */

#include "messager.h"
#include "util.h"
#include "networking.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <event2/event.h>

struct gossiper_s *g_gossiper;

void client() {
	struct event_base *base = event_base_new();

	struct messager_s *ms = malloc(sizeof(struct messager_s));
	messager_open(ms);
	char back[1024];

	ms->messager_init(TESTIP, SRVPORT, ms);
	struct str_s msg = {"abcdef", 6, 6};
	ms->messager_send(msg, ms);

	struct str_s *ba = malloc(sizeof(struct str_s));
	ba->data = malloc(sizeof(char) * 1024);
	ba->len = 1024;
	ba->used = 0;
	ms->messager_recv(ba, 1024, ms);
	fprintf(stdout, "srv back->%s\n", ba->data);
	ms->messager_destroy(ms);
	free(ba->data);
	free(ba);
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
	int i = 0;

	if (argc > 1 && !strcmp(argv[1], "-c")) {
		fprintf(stdout, "client\n");
		client();
	}
	else if (argc > 1 && !strcmp(argv[1], "-s")) {
		fprintf(stdout, "server\n");
		server();
	}

	return 0;
}
