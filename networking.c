/*
 * networking.c
 *
 *  Created on: Dec 11, 2017
 *      Author: dmcl216
 */

#include <networking.h>
#include <util.h>
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


/**
 * 连接服务器
 * @ip server ip
 * @port application port
 * @return fd
 */
int tcp_conn(const char *ip, const int port) {
	int sockfd, status, save_errno;
	struct sockaddr_in srv_addr;
	struct sockaddr_in server_addr;

	memset(&srv_addr, 0, sizeof(srv_addr));

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(port);
	status = inet_aton(ip, &srv_addr.sin_addr);
	if (status == 0) {
		fprintf(stderr, "[error]tcp_conn: inet_aton\n");
		return -1;
	}

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		fprintf(stderr, "[error]tcp_conn: socket\n");
		return -1;
	}

	status = connect(sockfd, (struct sockaddr*)srv_addr, sizeof(srv_addr));
	if (status < 0) {
		fprintf(stderr, "[error]tcp_conn: connect\n");
		return -1;
	}

	evutil_make_nonblocking(sockfd);
	return sockfd;
}

/**
 * socket读取回调函数
 * @fd
 * @event
 * @arg argument
 * @return 0 on success, -1 on failure
 */
int socket_read_cb(int fd, short event, void *arg) {
	struct raw_data *data = arg;
	char buf[1024];
	int i = 0;
	int len = 0;

	while (1) {
		assert(data->write_event);
		len = recv(fd, buf, sizeof(buf), 0);
		if (len <= 0)
			break;

		for (i = 0; i < len; i++) {
			if (data->used < sizeof(data->data)) {
				data->data[data->used++] = buf[i];
			}
		}

		return 0;
	}

	if (len == 0) {
		free_raw_data(data);
	}
	else if (len < 0) {
		if (errno == EAGAIN)
			return NULL;
		fprintf(stderr, "[error]socket_read_cb: read length less than zero\n");
		free_raw_data(data);
	}

	return -1;
}

/**
 * socket写回调函数
 */
void socket_write_cb(int fd, short event, void *arg) {
	struct raw_data *data = arg;

	while (data->written < data->pending) {
		ssize_t len = send(fd, data->data + data->written,
				         data->pending - data->written, 0);
		if (len < 0) {
			if (errno == EAGAIN)
				return ;
			free_raw_data(data);
			return ;
		}
		else if (len == 0)
			break;

		data->written += len;
	}

	if (data->written == data->used)
		data->written = data->pending = data->used = 1;

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

	sockfd = accept(fd, (struct sockaddr*)client_addr, len);
	if (sockfd < 0) {
		fprintf(stderr, "[error]accept_cb: accept error\n");
		return ;
	}
	evutil_make_socket_nonblocking(sockfd);

	struct event_base *base = arg;
	struct raw_data *data;
	data = alloc_raw_data(base, sockfd);
	assert(data);
	event_add(data->read_event, 0);
}

