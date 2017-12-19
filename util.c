/*
 * util.c
 *
 *  Created on: Dec 12, 2017
 *      Author: dmcl216
 */



#include "util.h"
#include "gossip.h"

#include <event2/event.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>

void parse(struct raw_data *raw) {
	assert (raw);

	if (raw->r_used >= 6 && strncmp("gossip", raw->read_buf, 6) == 0) {
		/* gossip消息 */
		char *back = "get gossip message";
		fprintf(stdout, "[info]get gossip message\n");
		handle_gossip_msg(g_gossiper, raw);

		memset(raw->write_buf, 0, raw->w_used + 1);
		strncpy(raw->write_buf, back, strlen(back));
		raw->w_used = strlen(back);
		event_add(raw->write_event, NULL);
	} else if (raw->r_used > strlen("on-start") && strncmp("on-start", raw->read_buf, strlen("on-start")) == 0) {
		/* 启动消息，
		 * 启动时发送一个只带自身信息的gossip消息，服务端更新之后返回gossip消息 */
		char *str = raw->read_buf;
		int skip = strlen("on-start");
		int gap = sizeof(struct host_state_s);
		struct host_state_s hs;
		memcpy(&hs, str+skip, gap);
		g_gossiper->gossiper_compare_update(hs, g_gossiper);
		struct str_s *ss = g_gossiper->gossiper_cur_msg(g_gossiper);

		memset(raw->write_buf, 0, raw->w_used + 1);
		strncpy(raw->write_buf, ss->data, ss->used);
		raw->w_used = ss->used;
		event_add(raw->write_event, NULL);
	} else {
		char *back = "can not recognize the operation.";
		memset(raw->write_buf, 0, raw->w_used + 1);
		strncpy(raw->write_buf, back, strlen(back));
		raw->w_used = strlen(back);
		event_add(raw->write_event, NULL);
	}
}

/**
 * 产生0 - range-1的随机数
 */
int get_rand(int range) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	// srand((unsigned)time(NULL));
	srand(tv.tv_usec);

	int ret = rand() % range;
	return ret;
}
