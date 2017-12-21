/*
 * networking_test.c
 *
 *  Created on: Dec 12, 2017
 *      Author: dmcl216
 */

#include "networking.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void cmd_cb(int fd, short event, void *arg) {
	char msg[1024];
	int len = read(fd, msg, 1024);

	msg[len] = '\0';
	int sockfd = *((int *)arg);
	int nwrite = send(sockfd, msg, len, 0);
}

void client() {
	const char *ip = "127.0.0.1";
	const int port = 35696;
	const char *msg = "hello world.";

	int sockfd = tcp_conn(ip, port);
	fprintf(stdout, "sockfd: %d\n", sockfd);


	struct event_base *base = event_base_new();
	struct event *read_event = event_new(base, sockfd, EV_READ|EV_PERSIST,
									client_recv_cb, NULL);
	struct event *cmd_event = event_new(base, STDIN_FILENO, EV_READ|EV_PERSIST,
									cmd_cb, (void*)&sockfd);

	event_add(read_event, NULL);
	event_add(cmd_event, NULL);
	event_base_dispatch(base);

	return ;
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

	if (argc > 1 && !strcmp(argv[1], "-s")) {
		fprintf(stdout, "server\n");
		server();
	}

	return 0;
}
