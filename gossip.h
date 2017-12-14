/*
 * gossip.h
 *
 *  Created on: Dec 12, 2017
 *      Author: dmcl216
 */

#ifndef GOSSIP_H_
#define GOSSIP_H_

#include <string.h>

static char **hosts;

enum HOST_STATE {
	ONLINE  = 0x01,
	OFFLINE = 0x10,
};

/**
 * 记录对应host的状态信息
 * @host 主机ip
 * @status 状态，ONLINE or OFFLINE
 * @generation 每次重新启动后自增1
 */
struct host_state_s {
	char *host;
	int status;
	int generation;
};

/**
 * 记录所有host的状态，更新时比较顺序
 * generation->version
 * @states 所有host的状态
 * @version 每经历一轮gossip，version自增1
 * @size states的大小
 * @n_host states保存的host的个数，当n_host >= size时调用realloc扩容
 */
struct gossiper_s {
	struct host_state_s *states;
	int version;
	int size;
	int n_host;

	/* 初始化gossiper */
	void (*gossiper_init) (struct gossiper_s *this);
	/* states扩容，每次扩大之前size的1/2 */
	void (*gossiper_realloc) (struct gossiper_s *this);
	/* 添加host信息 */
	void (*gossiper_put) (struct host_state_s hs, struct gossiper_s *this);
};

void g_init(struct gossiper_s *this);
void g_realloc(struct gossiper_s *this);
void g_put(struct host_state_s hs, struct gossiper_s *this);
void handle_gossip_msg(struct raw_data *msg);



#endif /* GOSSIP_H_ */
