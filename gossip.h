/*
 * gossip.h
 *
 *  Created on: Dec 12, 2017
 *      Author: dmcl216
 */

#ifndef GOSSIP_H_
#define GOSSIP_H_

#include <string.h>
#include <stdint.h>

#include "networking.h"
#include "util.h"

#define GOSSIP_HEADER "gossip"
#define G_HEADER_SIZE  6     // strlen(HEADER)


enum HOST_STATE {
	ONLINE  = 0x01,
	OFFLINE = 0x10,
};

/**
 * 记录对应host的状态信息
 * @host 主机ip
 * @status 状态，ONLINE or OFFLINE
 * @generation 每次重新启动后自增1
 * @version 每一轮gossip加1
 */
struct host_state_s {
	char *host;
	int status;
	uint32_t generation;
	uint32_t version;
};

/**
 * 记录所有host的状态，更新时比较顺序
 * generation->version
 * @states 所有host的状态
 * @size states的大小
 * @n_host states保存的host的个数，当n_host >= size时调用realloc扩容
 */
struct gossiper_s {
	struct host_state_s *states;
	uint32_t size;
	uint32_t n_host;

	/* 初始化gossiper */
	void (*gossiper_init) (struct gossiper_s *this);
	/* states扩容，每次扩大之前size的1/2 */
	int  (*gossiper_realloc) (struct gossiper_s *this);
	/* 更新host信息 */
	void (*gossiper_update) (struct host_state_s hs, int idx, struct gossiper_s *this);
	/* 新的节点信息插入 */
	int  (*gossiper_push) (struct host_state_s hs, struct gossiper_s *this);
	/* 发起新一轮gossip,每次选三个节点 */
	void (*gossiper_start) (struct gossiper_s *this);
	/* 当前gossip信息 */
	struct str_s* (*gossiper_cur_msg) (struct gossiper_s *this);
	/* 比较gossip */
	int  (*gossiper_compare_update) (struct host_state_s cur, struct gossiper_s *this);
	/* 打印gossip信息 */
	void (*gossiper_print) (struct gossiper_s *this);
	/* 析构函数 */
	void (*gossiper_destructor) (struct gossiper_s *this);
};

void gossiper_open(struct gossiper_s *this);
void g_init(struct gossiper_s *this);
int  g_realloc(struct gossiper_s *this);
void g_update(struct host_state_s hs, int idx, struct gossiper_s *this);
int  g_push(struct host_state_s hs, struct gossiper_s *this);
void g_start(struct gossiper_s *this);
struct str_s* g_cur_msg(struct gossiper_s *this);
int  g_compare_update(struct host_state_s cur, struct gossiper_s *this);
void g_print(struct gossiper_s *this);
void g_destructor(struct gossiper_s *this);

void handle_gossip_msg(struct gossiper_s *gs, struct raw_data *msg);
// handle_gossip_msg(struct gossiper_s *gs, char *msg)


#endif /* GOSSIP_H_ */
