/*
 * messager.h
 *
 *  Created on: Dec 13, 2017
 *      Author: dmcl216
 */

#ifndef MESSAGE_MESSAGER_H_
#define MESSAGE_MESSAGER_H_

#include "util.h"

struct messager_s {
	const char *ip;
	int port;
	int sockfd;

	int (*messager_init) (const char *srv_ip, const int port, struct messager_s * t);
	void (*messager_send) (struct str_s msg, struct messager_s *t);
	void (*messager_recv) (struct str_s *ret, int len, struct messager_s *t);
	void (*messager_close) (struct messager_s *t);
	void (*messager_destroy) (struct messager_s *t);
};

// struct messager_s * messager_open();
void messager_open(struct messager_s *ms);
int m_init(const char *srv_ip, const int port, struct messager_s *t);
void m_send(struct str_s msg, struct messager_s *t);
void m_recv(struct str_s *ret, int len, struct messager_s *t);
void m_close(struct messager_s *t);
void m_destroy(struct messager_s *t);

#endif /* MESSAGE_MESSAGER_H_ */
