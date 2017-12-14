/*
 * gossip.c
 *
 *  Created on: Dec 12, 2017
 *      Author: dmcl216
 */



#include "networking.h"
#include "gossip.h"

#include <event2/event.h>

#include <stdio.h>
#include <string.h>

void handle_gossip_msg(struct raw_data *msg) {
	char *m = msg->read_buf;
	char back[1024] = "back:";

	strcat(back + strlen(back), m);
	memset(m, 0, sizeof(msg->write_buf));
	strcpy(msg->write_buf, back);
	fprintf(stdout, "back:%s\n", msg->write_buf);
	event_add(msg->write_event, NULL);
}

void gossiper_open(struct gossiper_s *this) {
	this->gossiper_init = g_init;
	this->gossiper_realloc = g_realloc;
	this->gossiper_push = g_push;
	this->gossiper_update = g_update;
	this->gossiper_start = g_start;
}

/**
 * 初始化this，
 * size=64，n_host=1,version=1
 */
void g_init(struct gossiper_s *this) {
	struct host_state_s self;

	self.generation = 1;
	self.version = 1;
	self.status = ONLINE;
	self.host = "192.168.0.105";		// 将来用配置文件解析

	this->states = (struct host_state_s*)malloc(sizeof(struct host_state_s) * 64);
	this->states[0] = self;
	this->n_host = 1;
	this->size = 64;
}

/**
 * 扩容，容量增大原来的一半，不检查新的的size是否越界
 */
void g_realloc(struct gossiper_s *this) {
	//
	int new_size = this->size + this->size / 2;
	if (new_size < 0) {
		return ;
	}
	struct host_state_s *new_store = (struct host_state_s*)
												malloc(sizeof(struct host_state_s) * new_size);
	if (!new_store) {
		return ;
	}

	this->size = new_size;
	int i;
	for (i = 0; i < this->n_host; i++) {

	}

}

void g_update(struct host_state_s hs, struct gossiper_s *this);
void g_push(char *back, struct gossiper_s *this);
void g_start(struct gossiper_s *this);
