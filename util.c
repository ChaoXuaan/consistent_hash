/*
 * util.c
 *
 *  Created on: Dec 12, 2017
 *      Author: dmcl216
 */



#include "util.h"
#include "config.h"
#include "gossip.h"

#include <event2/event.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>

void parse(struct raw_data *raw) {
	assert (raw);

	if (raw->r_used >= G_HEADER_SIZE && strncmp(GOSSIP_HEADER, raw->read_buf, 6) == 0) {
		/* gossip消息: gossip+gossip
		 */
		char *back = "get gossip message";
		fprintf(stdout, "[info]get gossip message\n");
		handle_gossip_msg(g_gossiper, raw);

		memset(raw->write_buf, 0, raw->w_used + 1);
		strncpy(raw->write_buf, back, strlen(back));
		raw->w_used = strlen(back);
		event_add(raw->write_event, NULL);

	} else if (raw->r_used > strlen(NODE_INSERT)
	        && strncmp(NODE_INSERT, raw->read_buf, strlen(NODE_INSERT)) == 0) {
		/* 启动消息: node_insert+gossip
		 * 处理流程：
		 * 1. 更新本地gossip和host信息
		 * 2. 返回gossip信息
		 */
	    int skip = strlen(NODE_INSERT);
		char *str = raw->read_buf + skip;

		/* 处理gossip、host更新 */
		if (host_handler(ch_store, str) < 0) {
		    char *err = "NODE_INSERT ERROR HOST HANDLER";
		    memset(raw->write_buf, 0, raw->w_used + 1);
		    strncpy(raw->write_buf, err, strlen(err));
		    raw->w_used = strlen(err);
		    event_add(raw->write_event, NULL);
		    return ;
		}

		/* 返回gossip */
		struct str_s *ss = ch_store->gossiper->gossiper_cur_msg(ch_store->gossiper);

		memset(raw->write_buf, 0, raw->w_used + 1);
		strncpy(raw->write_buf, ss->data, ss->used);
		raw->w_used = ss->used;
		free(ss->data);
		free(ss);
		event_add(raw->write_event, NULL);

	} else if (raw->r_used > strlen(INSERT_MIGRATE) &&
	        !strncmp(INSERT_MIGRATE, raw->read_buf, strlen(INSERT_MIGRATE))) {
	    /*
	     * insert_migrate消息：inssert_migrate+gossip
	     * 迁移处理：
	     * 1. 更新gossip、host
	     * 2. 返回需要迁移的数据
	     */
	    int skip = strlen(INSERT_MIGRATE);
	    char *str = raw->read_buf + skip;
	    /* 处理gossip、host更新 */
        if (host_handler(ch_store, str) < 0) {
            memset(raw->write_buf, 0, raw->w_used + 1);
            char *err = "INSERT_MIGRATE ERROR HOST HANDLER";
            strncpy(raw->write_buf, err, strlen(err));
            raw->w_used = strlen(err);
            event_add(raw->write_event, NULL);
            return ;
        }

        /* 返回迁移数据 */
        char *cli_addr = inet_ntoa(raw->cli_addr.sin_addr);
        int cnt = ch_store->cnt_belongs2(cli_addr, ch_store);
        int *data = ch_store->data_belongs2(cli_addr, ch_store);
        assert (data);
        memset(raw->write_buf, 0, raw->w_used + 1);
        memcpy(raw->write_buf, data, cnt * sizeof(int));
        raw->w_used = sizeof(int) * cnt / sizeof(char);
        free(data);
        event_add(raw->write_event, NULL);

	} else if (raw->r_used > strlen(DATA_INSERT) &&
	            !strncmp(DATA_INSERT, raw->read_buf, strlen(DATA_INSERT))) {
	    /*
	     * 数据插入:data_insert+data(int)
	     */
	    int skip = strlen(DATA_INSERT);
	    char *s = raw->read_buf + skip;
	    int v;
	    memcpy(&v, s, sizeof(int) / sizeof(char));

	    char back[256];
	    if (ch_store->value_put(v, ch_store) < 0) {
	        back = "insert value fail";

	    } else {
	        back = "insert value succeed";
	    }
        memset(raw->write_buf, 0, raw->w_used + 1);
        strcpy(raw->write_buf, back);
        raw->w_used = strlen(back);
        event_add(raw->write_event, NULL);
	}
	else {
	    /* 未定义消息 */
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
