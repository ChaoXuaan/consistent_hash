/*
 * messager.h
 *
 *  Created on: Dec 13, 2017
 *      Author: dmcl216
 */

#ifndef MESSAGE_MESSAGER_H_
#define MESSAGE_MESSAGER_H_

struct messager_s {
	const char *ip;
	int port;
	int sockfd;

	void (*messager_init) (const char *srv_ip, const int port, struct messager_s * t);
	void (*messager_send) (const char *msg, struct messager_s *t);
	void (*messager_recv) (char *ret, int ret_len, struct messager_s *t);
	void (*messager_destroy) (struct messager_s *t);
};

struct messager_s * messager_open();
void m_init(const char *srv_ip, const int port, struct messager_s *t);
void m_send(const char *msg, struct messager_s *t);
void m_recv(char *ret, int ret_len, struct messager_s *t);
void m_destroy(struct messager_s *t);

#endif /* MESSAGE_MESSAGER_H_ */
