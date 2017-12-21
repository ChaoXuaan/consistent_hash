/*
 * gossip_test.c
 *
 *  Created on: Dec 18, 2017
 *      Author: dmcl216
 */


#include "gossip.h"
#include "util.h"
#include "message/messager.h"
#include "chash/chash.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct gossiper_s *g_gossiper;
struct chash_store_s *ch_store;
char *ip[5] = { "192.168.0.111", "192.168.0.112", "192.168.0.113",
			"192.168.0.114", "192.168.0.115" };

void g_test() {
	struct gossiper_s *gs = malloc(sizeof(struct gossiper_s));
	gossiper_open(gs);
	gs->gossiper_init(gs);

	struct host_state_s hs[5];
	int i, gen = 0, ver_rand = 10;

	for (i = 0; i < 5; i++) {
		strcpy(hs[i].host, ip[i]);
		hs[i].generation = gen;
		hs[i].version = get_rand(ver_rand);
		int s = get_rand(2);
		fprintf(stdout, "%d %s\n", s, s == 0 ? "offline" : "online");
		hs[i].status = (s == 1 ? OFFLINE : ONLINE);

		gs->gossiper_compare_update(hs[i], gs);
	}

	gs->gossiper_print(gs);
}

void client() {
	struct messager_s *ms = malloc(sizeof(struct messager_s));
	messager_open(ms);

	char back[1024];
	char *tomsg = malloc(sizeof(struct host_state_s) * 5 + G_HEADER_SIZE + 1);
	int skip = G_HEADER_SIZE;
	memcpy(tomsg, GOSSIP_HEADER, G_HEADER_SIZE);

	struct host_state_s hs[5];
	int hs_len = sizeof(struct host_state_s);
	int i, gen = 0, ver_rand = 10;
	for (i = 0; i < 5; i++) {
		strcpy(hs[i].host, ip[i]);
		hs[i].generation = gen;
		hs[i].version = get_rand(ver_rand);
		int s = get_rand(2);
		// fprintf(stdout, "%d %s\n", s, s == 0 ? "offline" : "online");
		hs[i].status = (s == 1 ? OFFLINE : ONLINE);

		memcpy(tomsg + skip, &hs[i], hs_len);
		skip += hs_len;
	}

	tomsg[skip] = '\0';
	struct str_s ss;
	ss.data = tomsg;
	ss.len = skip;
	ss.used = skip;

	i = 0, skip = G_HEADER_SIZE;
	while (*(tomsg + skip) != '\0') {
		i++;
		skip += hs_len;
	}

	// g_gossiper->gossiper_print(g_gossiper);
	// char *tomsg = g_gossiper->gossiper_cur_msg(g_gossiper);
	fprintf(stdout, "tomsg = %s-%d-%ld\n", tomsg, i, ss.len);

	if (ms->messager_init(SRV_HOST, SRVPORT, ms) < 0) {
		return ;
	}
	ms->messager_send(ss, ms);

	char buf[1024];
	struct str_s ss_back;
	ss_back.data = buf;
	ss_back.len = 1024;
	ss_back.used = 0;
	ms->messager_recv(&ss_back, 1024, ms);
	fprintf(stdout, "srv back->%s\n", ss_back.data);

	ms->messager_destroy(ms);
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
