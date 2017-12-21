/*
 * gossip.c
 *
 *  Created on: Dec 12, 2017
 *      Author: dmcl216
 */

#include "gossip.h"
#include "config.h"
#include "message/messager.h"
#include "util.h"
#include "networking.h"
#include "config.h"

#include <event2/event.h>
#include <event2/util.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void handle_gossip_msg(struct gossiper_s *gs, struct raw_data *msg) {
	char *src;
	char *ss = msg->read_buf;
	int skip = G_HEADER_SIZE;
	int gap = sizeof(struct host_state_s);
	int i = 0;
	while (*(ss+skip) != '\0') {
		struct host_state_s hs;
		memcpy(&hs, ss+skip, gap);
		gs->gossiper_compare_update(hs, gs);
		skip += gap;

		if (i == 0)
			src = hs.host;
		i++;
	}

	fprintf(stdout, "[info]%s send gossip message, contains %d host message.\n", src, i);
	gs->gossiper_print(gs);
}

/**
 * gossiper初始调用函数，函数赋值
 * @this 初始化变量
 * @return void
 */
void gossiper_open(struct gossiper_s *this) {
	this->gossiper_init = g_init;
	this->gossiper_realloc = g_realloc;
	this->gossiper_push = g_push;
	this->gossiper_update = g_update;
	this->gossiper_start = g_start;
	this->gossiper_cur_msg = g_cur_msg;
	this->gossiper_compare_update = g_compare_update;
	this->gossiper_print = g_print;
	this->gossiper_destructor = g_destructor;
}

/*
struct gossiper_s* gossiper_open() {
	struct gossiper_s *this = malloc(sizeof(struct gossiper_s));

	this->gossiper_init = g_init;
	this->gossiper_realloc = g_realloc;
	this->gossiper_push = g_push;
	this->gossiper_update = g_update;
	this->gossiper_start = g_start;
	this->gossiper_cur_msg = g_cur_msg;
	this->gossiper_compare_update = g_compare_update;
	this->gossiper_print = g_print;

	return this;
}*/

/**
 * 初始化this，
 * size=64，n_host=1,version=1
 * @this
 * @return void
 */
void g_init(struct gossiper_s *this) {
	struct host_state_s self;

	self.generation = 1;				// generation应该是++
	self.version = 1;
	self.status = ONLINE;
//	self.host = malloc(sizeof(char) * (strlen(LOCAL_HOST) + 1));
	strcpy(self.host, LOCAL_HOST);		// 将来用配置文件解析

	this->states = (struct host_state_s*)malloc(sizeof(struct host_state_s) * 64);
	this->states[0] = self;
	this->n_host = 1;
	this->size = 64;
}

/**
 * 扩容，容量增大原来的一半，不检查新的的size是否溢出
 * @this
 * @return 0 on success，-1 on failure
 */
int g_realloc(struct gossiper_s *this) {
	//
	uint32_t new_size = this->size + this->size / 2;
	if (new_size < this->size) {
	    fprintf(stderr, "[error]g_realloc: realloc fail.\n");
		return -1;
	}
	struct host_state_s *new_store = (struct host_state_s*)
												malloc(sizeof(struct host_state_s) * new_size);
	if (!new_store) {
		return -1;
	}

	this->size = new_size;
	int i;
	for (i = 0; i < this->n_host; i++) {
		new_store[i] = this->states[i];
	}
	struct host_state_s *pre_hs = this->states;
	this->states = new_store;
	free(pre_hs);
	return 0;
}

/**
 * 更新idx下标的host信息
 * @hs 新的host信息
 * @idx 需要更新的下标
 * @this
 * @return void
 */
void g_update(struct host_state_s hs, int idx, struct gossiper_s *this) {
//	struct host_state_s *p_hs = malloc(sizeof(struct host_state_s));
//	memcpy(p_hs, &hs, sizeof(struct host_state_s));
//	struct host_state_s *pre = this->states[idx];

	this->states[idx] = hs;
}

/**
 * 将hs插入到states的尾部，hs可能是新插入的节点
 * @hs 待插入host变量
 * @this
 * @return 0 on success, -1 on failure
 */
int g_push(struct host_state_s hs, struct gossiper_s *this) {
	if (this->n_host >= this->size) {
		int status = this->gossiper_realloc(this);
		if (status < 0)
			return -1;
	}

	this->states[this->n_host] = hs;
	this->n_host++;

	return 0;
}

/**
 * 生成gossip信息，将每个host_state_s的成员复制保存到字符数组
 * @return gossip msg
 */
struct str_s* g_cur_msg(struct gossiper_s *this) {
//    this->states[0].version++;
	struct str_s *ss = malloc(sizeof(struct str_s));
	char* header = GOSSIP_HEADER;
	const int header_length = G_HEADER_SIZE;
	ss->data = malloc(sizeof(struct host_state_s) * this->n_host + header_length + 1);
	char *cur = ss->data;
	int size = 0;

	memcpy(cur, header, header_length);
	size += header_length;

	int i, hs_len = sizeof(struct host_state_s);
	for (i = 0; i < this->n_host; i++) {
		struct host_state_s hs = this->states[i];
		memcpy(cur + size, &hs, hs_len);
		size += hs_len;
	}
	cur[size] = '\0';
	ss->len = size;
	ss->used = size;
	return ss;
}

/**
 * 发起gossip，自身的version+1，随机选取三个节点，有一个是种子节点
 * @this
 * @return void
 */
void g_start(struct gossiper_s *this) {
    this->states[0].status = ONLINE;
    this->states[0].version++;

	struct messager_s ms[3];
	int i, idx, pre[3] = {0};
	for (i = 0; i < 3;i++) {
		messager_open(&ms[i]);
	}
	/* host个数超过4个，随机选三个，应该有一个是seed,这里还没有实现 */
	if (this->n_host > 4) {
		for (i = 0; i < 3; i++) {
			do {
				idx = get_rand(this->n_host);
			} while (idx != 0 && idx != pre[0] && idx != pre[1] && idx != pre[2]);
			pre[i] = idx;

			fprintf(stdout, "[info]g_start: send gossip to %s\n", this->states[i].host);
			if (ms[i].messager_init(this->states[idx].host, SRVPORT, &ms[i]) < 0) {
				fprintf(stdout, "[error]g_start: mark %s is offline\n", this->states[i].host);
			    this->states[idx].status=OFFLINE;
			    this->states[idx].version++;
			} else {
			    struct str_s *cur_msg = this->gossiper_cur_msg(this);
                ms[i].messager_send(*cur_msg, &ms[i]);
                char buf[1024];
                struct str_s ss = {buf, 1024, 0};
                ms[i].messager_recv(&ss, ss.len, &ms[i]);
                ms[i].messager_close(&ms[i]);
                free(cur_msg);
                fprintf(stdout, "[info]g_start: gossip with %s successful\n", this->states[i].host);
			}

		}
	} else {
		for (i = 1; i < this->n_host; i++) {
			fprintf(stdout, "[info]g_start: send gossip to %s\n", this->states[i].host);
            if (ms[i - 1].messager_init(this->states[i].host, SRVPORT,
                    &ms[i - 1]) < 0) {
            	fprintf(stdout, "[error]g_start: mark %s is offline\n", this->states[i].host);
                this->states[i].status = OFFLINE;
                this->states[i].version++;
			} else {
                struct str_s *cur_msg = this->gossiper_cur_msg(this);
                ms[i - 1].messager_send(*cur_msg, &ms[i - 1]);
                char buf[1024];
                struct str_s ss = { buf, 1024, 0 };
                ms[i].messager_recv(&ss, ss.len, &ms[i]);
                ms[i - 1].messager_close(&ms[i - 1]);
                free(cur_msg);
                fprintf(stdout, "[info]g_start: gossip with %s successful\n", this->states[i].host);
			}
		}
	}
}

/**
 * 比较并更新，如果不存在就插入
 * @cur 待比较host，如果存在并符合以下规则更新
 *    -- cur.generation > this.generation 更新
 *    -- cur.generation == this.generation && cur.version > this.version
 * @this
 * @return 0 on success, -1 on failure
 */
int  g_compare_update(struct host_state_s cur, struct gossiper_s *this) {
	int i, status;

	for (i = 0 ; i < this->n_host; i++) {
		status = strcmp(cur.host, this->states[i].host);
		if (!status && cur.generation > this->states[i].generation) {
			this->gossiper_update(cur, i, this);
			return 0;
		} else if (!status && cur.generation == this->states[i].generation &&
				cur.version > this->states[i].version) {
			this->gossiper_update(cur, i, this);
			return 0;
		} else if (!status){
		    /* 首先更新，有可能不如该节点信息新 */
		    break;
		}
	}

	if (i < this->n_host)
	    return 0;

	status = this->gossiper_push(cur, this);
	if (status < 0)
		return -1;
	return 0;
}

/**
 * 打印保存的gossip信息
 * @this
 * @return void
 */
void g_print(struct gossiper_s *this) {
	int i;

	fprintf(stdout, "n_host = %d, size = %d\n", this->n_host, this->size);
	for (i = 0; i < this->n_host; i++) {
		fprintf(stdout, "[%s,%s,gen=%d,ver=%d]\n", this->states[i].host, this->states[i].status == ONLINE ? "online" : "offline",
									this->states[i].generation, this->states[i].version);
	}
}

void g_destructor(struct gossiper_s *this) {
    free(this->states);
    free(this);
}
