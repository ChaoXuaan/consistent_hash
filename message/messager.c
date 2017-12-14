/*
 * messager.c
 *
 *  Created on: Dec 13, 2017
 *      Author: dmcl216
 */

#include "../message/messager.h"

#include "networking.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

struct messager_s * messager_open() {
	struct messager_s *ms = (struct messager_s*)malloc(sizeof(struct messager_s));
	ms->messager_init = m_init;
	ms->messager_send = m_send;
	ms->messager_recv = m_recv;
	ms->messager_destroy = m_destroy;

	return ms;
}

void m_init(const char *srv_ip, const int port, struct messager_s *t) {
	t->ip = srv_ip;
	t->port = port;
	t->sockfd = tcp_conn(t->ip, t->port);
}

void m_send(const char *msg, struct messager_s *t) {
	int written = 0, pending = strlen(msg);
	while (written < pending) {
		int len = send(t->sockfd, msg + written, pending - written, 0);
		if (len < 0) {
			return ;
		}
		written += len;
	}

	fprintf(stdout, "m_send success\n");
}

void m_recv(char *ret, int ret_len, struct messager_s *t) {
	while (1) {
		int len = recv(t->sockfd, ret, ret_len, 0);
		if (len > 0) {
			ret[len] = '\0';
			break;
		}
	}
}

void m_destroy(struct messager_s *t) {
	t->messager_send("close", t);
	tcp_close(t->sockfd);
	free(t);
}
