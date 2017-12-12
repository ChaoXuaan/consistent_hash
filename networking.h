/*
 * networking.h
 *
 *  Created on: Dec 11, 2017
 *      Author: dmcl216
 *     Descrip: 使用简单的libevent实现网络功能
 */

#ifndef NETWORKING_H_
#define NETWORKING_H_

#include <event2/event.h>

#define SRVPORT 35696
#define MAXBUF  4096
#define CLOSEFD "close"

struct raw_data {
	char   read_buf[MAXBUF];
	// size_t r_pending;		// 待写入的长度
	// size_t r_used;			// 已经使用的长度
	// size_t r_written;		// 已经写的长度

	char   write_buf[MAXBUF];
	// size_t w_pending;		// 待写入的长度
	// size_t w_used;			// 已经使用的长度
	// size_t w_written;		// 已经写的长度

	struct event *read_event;
	struct event *write_event;
};

struct raw_data* alloc_raw_data(struct event_base *base, int fd);
void free_raw_data(struct raw_data* data);

int tcp_conn(const char *ip, const int port);
void client_recv_cb(int fd, short event, void *arg);
void socket_read_cb(int fd, short event, void *arg);
void socket_write_cb(int fd, short event, void *arg);
void accept_cb(int fd, short event, void *arg);
int tcp_init(const int port, const int listen_num);

#endif /* NETWORKING_H_ */
