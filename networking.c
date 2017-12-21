/*
 * networking.c
 *
 *  Created on: Dec 11, 2017
 *      Author: dmcl216
 */

#include "networking.h"
#include "util.h"

#include <event2/util.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>


/**
 * 连接服务器
 * @ip server ip
 * @port application port
 * @return -1 on failure, socket fd on success
 */
int tcp_conn(const char *ip, const int port) {
	int sockfd, status, save_errno;
	struct sockaddr_in srv_addr;

	memset(&srv_addr, 0, sizeof(srv_addr));

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(port);
	status = inet_pton(AF_INET, ip, &srv_addr.sin_addr); //inet_aton(ip, &srv_addr.sin_addr);
	if (status == 0) {
		fprintf(stderr, "[error]tcp_conn: inet_aton(%s)\n", ip);
		return -1;
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		fprintf(stderr, "[error]tcp_conn: socket\n");
		return -1;
	}

	status = connect(sockfd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	if (status < 0) {
		fprintf(stderr, "[error]tcp_conn: connect\n");
		return -1;
	}

	evutil_make_socket_nonblocking(sockfd);
	return sockfd;
}

/**
 * 关闭连接
 */
int tcp_close(int fd) {
	assert (fd >= 0);
	int status = close(fd);
	if (status < 0) {
		fprintf(stderr, "[error]tcp_close: close-%d\n", errno);
	}

	return status;
}

/**
 * 客户端接收回调函数
 */
void client_recv_cb(int fd, short event, void *arg) {
	char buf[MAXBUF];
	int len = recv(fd, buf, sizeof(buf), 0);
	if (len < 0) {
		fprintf(stderr, "[error]client_recv_cb: recv.\n");
		exit(1);
	} else if (len == 0) {
		return ;
	}

	buf[len] = '\0';
	fprintf(stdout, "recv from server: %s\n", buf);
	return ;
}

/**
 * tcp服务端初始化，返回服务端socket fd
 * @port application port
 * @listen_num
 * @return socket fd
 */
int tcp_init(const int port, const int listen_num) {
	int save_errno;
	evutil_socket_t listener;

	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		fprintf(stderr, "[error]tcp_init: socket\n");
		return -1;
	}

	evutil_make_listen_socket_reuseable(listener);

	struct sockaddr_in srv_addr;
	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(port);
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ((bind(listener, (struct sockaddr*)&srv_addr, sizeof(srv_addr))) < 0) {
		fprintf(stderr, "[error]tcp_init: bind\n");
		goto error;
	}

	if ((listen(listener, listen_num)) < 0) {
		fprintf(stderr, "[error]tcp_init: listen\n");
		goto error;
	}

	evutil_make_socket_nonblocking(listener);
	return listener;

error:
	save_errno = errno;
	evutil_closesocket(listener);
	errno = save_errno;
	return -1;
}

/**
 * socket读取回调函数
 * @fd
 * @event
 * @arg argument
 * @return 0 on success, -1 on failure
 */
void socket_read_cb(int fd, short event, void *arg) {
	struct raw_data *data = (struct raw_data*)arg;
	char *buf = data->read_buf;
	int i = 0;
	int len = 0;

	while (1) {
		memset(data->read_buf, 0, MAXBUF);
		len = recv(fd, data->read_buf, sizeof(data->read_buf), 0);
		if (len <= 0)
			break;

		data->read_buf[len] = '\0';
		data->r_used = len;
		if (!strcmp(data->read_buf, "close")) {
			fprintf(stdout, "recv close: %d\n", fd);
			tcp_close(fd);
			len = 0;
			break;
		}

		// fprintf(stdout, "recv: %s\n", data->read_buf);
		parse(data);
		// event_add(data->write_event, NULL);

		return ;
	}

	if (len == 0 && data) {
		free_raw_data(data);
	}
	else if (len < 0) {
		if (errno == EAGAIN)
			return ;
		close(fd);
		fprintf(stderr, "[error]socket_read_cb: read length less than zero\n");
		free_raw_data(data);
	}

	return ;
}

/**
 * socket写回调函数
 */
void socket_write_cb(int fd, short event, void *arg) {
	struct raw_data *data = arg;
	assert (data->write_buf);
	int written = 0, pending = data->w_used;

	// fprintf(stdout, "write_buf: %s\n", data->write_buf);

	while (written < pending) {
		// fprintf(stdout, "written: %d\n", written);
		// fprintf(stdout, "pending: %d\n", pending);
		ssize_t len = send(fd, data->write_buf + written,
				         pending - written, 0);
		if (len < 0) {
			if (errno == EAGAIN)
				return ;
			free_raw_data(data);
			return ;
		}
		else if (len == 0)
			break;

		written += len;
	}

	/*
	if (data->written == data->used)
		data->written = data->pending = data->used = 1;
	 */
	event_del(data->write_event);
}

/**
 * 接收连接回调函数
 * @fd
 * @event
 * @arg
 * @return void
 */
void accept_cb(int fd, short event, void *arg) {
	evutil_socket_t sockfd;
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);

	sockfd = accept(fd, (struct sockaddr*)&client_addr, &len);
	fprintf(stdout, "accept sockfd: %d\n", sockfd);
	if (sockfd < 0) {
		fprintf(stderr, "[error]accept_cb: accept error\n");
		return ;
	}
	evutil_make_socket_nonblocking(sockfd);

	struct event_base *base = arg;
	struct raw_data *data;
	data = alloc_raw_data(base, sockfd);
	data->cli_addr = client_addr;
	fprintf(stdout, "[info]client request: %s\n", inet_ntoa(client_addr.sin_addr));
	assert(data);
	event_add(data->read_event, 0);
}

struct raw_data* alloc_raw_data(struct event_base *base, int fd) {
	struct raw_data *p_data = (struct raw_data*)malloc(sizeof(struct raw_data));
	if (!p_data) {
		fprintf(stderr, "[error]alloc_raw_data: malloc\n");
		return NULL;
	}

	p_data->r_pending = MAXBUF;
	p_data->w_pending = MAXBUF;

	p_data->read_event = event_new(base, fd, EV_READ|EV_PERSIST,
							socket_read_cb, (void*)p_data);
	if (!p_data->read_event) {
		fprintf(stderr, "[error]alloc_raw_data: event_new for read_event\n");
		free(p_data);
		return NULL;
	}

	p_data->write_event = event_new(base, fd, EV_WRITE|EV_PERSIST,
							socket_write_cb, (void*)p_data);
	if (!p_data->write_event) {
		fprintf(stderr, "[error]alloc_raw_data: event_new for write_event\n");
		free(p_data);
		return NULL;
	}

	// p_data->r_pending = p_data->r_used = p_data->r_written = 0;
	// p_data->w_pending = p_data->w_used = p_data->w_written = 0;
	return p_data;
}

void free_raw_data(struct raw_data *data) {
	event_free(data->read_event);
	event_free(data->write_event);
	free(data);
	data = NULL;
}

