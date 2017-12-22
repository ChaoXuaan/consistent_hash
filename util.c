/*
 * util.c
 *
 *  Created on: Dec 12, 2017
 *      Author: dmcl216
 */

#include "util.h"
#include "config.h"
#include "gossip.h"
#include "chash/chash.h"

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

extern struct chash_store_s *ch_store;

void parse(struct raw_data *raw) {
	assert (raw);

	if (raw->r_used >= G_HEADER_SIZE &&
	        strncmp(GOSSIP_HEADER, raw->read_buf, 6) == 0) {
		/* gossip消息: gossip+gossip
		 * push-pull 模式
		 */
	    fprintf(stdout, "[info]get gossip message from %s\n", inet_ntoa(raw->cli_addr.sin_addr));

	    int header = G_HEADER_SIZE;
	    char *str = raw->read_buf + header;
	    host_handler(ch_store, str);
	    /* 返回gossip消息 */
//		struct str_s *ss = ch_store->gossiper->gossiper_cur_msg(ch_store->gossiper);
//        memset(raw->write_buf, 0, raw->w_used + 1);
//        strncpy(raw->write_buf, ss->data, ss->used);
//        raw->w_used = ss->used;
//        free(ss->data);
//        free(ss);
//        event_add(raw->write_event, NULL);

//        handle_gossip_msg(g_gossiper, raw);
	    char *back = "get gossip message";
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
	    fprintf(stdout, "[info]get node_insert message from %s\n",
	                inet_ntoa(raw->cli_addr.sin_addr));
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
	    fprintf(stdout, "[info]get insert_migrate message from %s\n",
	                inet_ntoa(raw->cli_addr.sin_addr));
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
        if (cnt == 0) {
            char *back = "no migrate";
            memset(raw->write_buf, 0, raw->w_used + 1);
            memcpy(raw->write_buf, back, strlen(back));
            raw->w_used = strlen(back);
            event_add(raw->write_event, NULL);
            return ;
        }
        int *data = ch_store->data_belongs2(cli_addr, ch_store);
        assert (data);
        fprintf(stdout, "[info]migrate following data to %s\n", cli_addr);
        int i;
        for (i = 0; i < cnt; i++) {
            fprintf(stdout, "%d ", data[i]);
        }
        fprintf(stdout, "\n");

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
	    fprintf(stdout, "[info]get data_insert message from %s\n",
	                    inet_ntoa(raw->cli_addr.sin_addr));
	    int skip = strlen(DATA_INSERT);
	    char *s = raw->read_buf + skip;
	    int v;
	    memcpy(&v, s, sizeof(int));

	    char back[256];
	    if (ch_store->value_put(v, ch_store) < 0) {
	        strcpy(back, "insert value fail");

	    } else {
	        strcpy(back, "insert value successful");
	    }
	    fprintf(stdout, "[info]%s\n", back);
        memset(raw->write_buf, 0, raw->w_used + 1);
        strcpy(raw->write_buf, back);
        raw->w_used = strlen(back);
        event_add(raw->write_event, NULL);

	} else if(raw->r_used > strlen(DATA_DELETE) &&
	            !strncmp(DATA_DELETE, raw->read_buf, strlen(DATA_DELETE))) {
	    /* 数据删除：data_delete+data(int) */
        fprintf(stdout, "[info]get data_delete message from %s\n",
                inet_ntoa(raw->cli_addr.sin_addr));
	    int skip = strlen(DATA_DELETE);
	    char *s = raw->read_buf + skip;
	    int v;
	    memcpy(&v, s, sizeof(int));

	    char back[256];
	    if (ch_store->value_delete(v, ch_store) < 0) {
	        strcpy(back, "delete value fail");
	    } else {
	        strcpy(back, "delete value successful");
	    }
	    fprintf(stdout, "[info]%s\n", back);
	    memset(raw->write_buf, 0, raw->w_used + 1);
        strcpy(raw->write_buf, back);
        raw->w_used = strlen(back);
        event_add(raw->write_event, NULL);

	} else {
	    /* 未定义消息 */
	    fprintf(stdout, "[info]get none-defined message from %s\n",
	                inet_ntoa(raw->cli_addr.sin_addr));
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

/**
 * 无符号整数转字符串
 */
char* itoa(unsigned int num, char *str, int radix) {
    /*索引表*/
    char index[] = "0123456789ABCDEF";
    /*中间变量*/
    unsigned int unum = num;
    int i = 0, j, k;

    /*转换*/
    do {
        str[i++] = index[unum % (unsigned) radix];
        unum /= radix;
    } while (unum);
    str[i] = '\0';
    /*逆序*/
    k = 0;
    char temp;
    for (j = k; j <= (i - 1) / 2; j++) {
        temp = str[j];
        str[j] = str[i - 1 + k - j];
        str[i - 1 + k - j] = temp;
    }
    return str;
}
