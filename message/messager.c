/*
 * messager.c
 *
 *  Created on: Dec 13, 2017
 *      Author: dmcl216
 */

#include "messager.h"
#include "util.h"
#include "networking.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

void messager_open(struct messager_s *ms) {
	ms->messager_init = m_init;
	ms->messager_send = m_send;
	ms->messager_recv = m_recv;
	ms->messager_close = m_close;
	ms->messager_destroy = m_destroy;
}

/**
struct messager_s * messager_open() {
	struct messager_s *ms = (struct messager_s*)malloc(sizeof(struct messager_s));
	ms->messager_init = m_init;
	ms->messager_send = m_send;
	ms->messager_recv = m_recv;
	ms->messager_destroy = m_destroy;

	return ms;
}
*/

/**
 * 启动连接，连接成功返回0，失败返回-1
 * @srv_ip server ip
 * @port service port
 * @t
 * @return 0 on success, -1 on fail
 */
int m_init(const char *srv_ip, const int port, struct messager_s *t) {
	t->ip = srv_ip;
	t->port = port;
	t->sockfd = tcp_conn(t->ip, t->port);
	if (t->sockfd < 0) {
		return -1;
	}

	return 0;
}

void m_send(struct str_s msg, struct messager_s *t) {
	int written = 0, pending = msg.used;
	// fprintf(stdout, "%d\n", pending);
	while (written < pending) {
		int len = send(t->sockfd, msg.data + written, pending - written, 0);
		if (len < 0) {
			return ;
		}
		written += len;
	}

	fprintf(stdout, "[info]m_send success:%s----%d\n", msg.data, written);
}

void m_recv(struct str_s *ret, int len, struct messager_s *t) {
	while (1) {
		int len = recv(t->sockfd, ret->data, len, 0);
		if (len > 0) {
			ret->used = len;
			ret->data[len] = '\0';
			break;
		}
	}
}

void m_close(struct messager_s *t) {
    struct str_s close = {"close", 5, 5};
    t->messager_send(close, t);
    tcp_close(t->sockfd);
}

void m_destroy(struct messager_s *t) {
	struct str_s close = {"close", 5, 5};
	t->messager_send(close, t);
	tcp_close(t->sockfd);
	free(t);
}
