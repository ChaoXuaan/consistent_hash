/*
 * chash.h
 *
 *  Created on: Dec 19, 2017
 *      Author: dmcl216
 */

#ifndef CHASH_H_
#define CHASH_H_

#define NODE_MAX_NUM  1024
#define VALUE_MAX_NUM (1 << 30)

#include "util.h"

#include <stdint.h>

struct chash_host {
	char *ipv4;
	char name[64];
	unsigned long hash;
};

struct chash_store_s {
    struct gossiper_s *gossiper;
	struct chash_host hosts[NODE_MAX_NUM];
	uint32_t n_host;

    struct value_store_s *value_store;
    uint32_t n_value;
    uint32_t space;

    /* 初始化 */
	void (*chash_store_init) (struct gossiper_s *gs, struct chash_store_s *t);
	/* 第一次启动，加入集群 */
	int (*node_insert) (struct chash_store_s *t);
	/* 退出集群 */
	int  (*node_delete) (struct chash_store_s *t);
	/* 节点根据hash排序，暂未实现，因为目前的节点插入是有序插入 */
	void (*node_sort) (struct chash_store_s *t);
	/* hosts添加节点数据 */
	int  (*host_push) (char *ip, struct chash_store_s *t);
	/* hosts删除节点数据 */
	int  (*host_delete) (char *ip, struct chash_store_s *t);
	/* 查找节点是否存在，存在返回下标，不存在返回-1 */
	int  (*host_find) (struct chash_host ch, struct chash_store_s *t);
	/* 查找前驱节点 */
	char* (*get_pre_host) (char *ip, struct chash_store_s *t);
	/* 计算host的hash */
	unsigned long (*host_hash) (char *ip, struct chash_store_s *t);
	/* value_store扩容 */
	int  (*value_store_realloc)(struct chash_store_s *t);
	/* 插入数据 */
	int  (*value_put)(int v, struct chash_store_s *t);
	/* 获得数据，暂未实现 */
	int  (*value_get)(int v, struct chash_store_s *t);
	/* 更新数据，由于不是k-v结构，暂未实现 */
    int  (*value_update)(int oldV, int newV, struct chash_store_s *t);
    /* 删除数据 */
	int  (*value_delete)(int v, struct chash_store_s *t);
	/* 根据value的hash值排序，暂未实现 */
	void (*value_sort) (struct chash_store_s *t);
	/* 计算value的hash */
	unsigned long (*value_hash) (int v, struct chash_store_s *t);
	/* 析构函数 */
	void (*chash_store_destructor) (struct chash_store_s *t);
	/* 返回当前节点的数据属于host的个数 */
	int  (*cnt_belongs2) (char *host, struct chash_store_s *t);
	/* 返回当前节点的数据属于host的数据 */
	int* (*data_belongs2) (char *host, struct chash_store_s *t);
	/* 打印数据 */
	void (*value_print) (struct chash_store_s *t);
};


void chash_store_open(struct chash_store_s *t);
void chash_store_init(struct gossiper_s *gs, struct chash_store_s *t);
int  chash_node_insert(struct chash_store_s *t);
int  chash_node_delete(struct chash_store_s *t);
void chash_node_sort(struct chash_store_s *t);
int  chash_host_push(char *ip, struct chash_store_s *t);
int  chash_host_delete(char *ip, struct chash_store_s *t);
int  chash_host_find(struct chash_host ch, struct chash_store_s *t);
char* chash_pre_host(char *ip, struct chash_store_s *t);
unsigned long chash_host_hash(char *ip, struct chash_store_s *t);
int  chash_value_realloc(struct chash_store_s *t);
int  chash_value_put(int v, struct chash_store_s *t);
int  chash_value_get(int v, struct chash_store_s *t);
int  chash_value_update(int oldV, int newV, struct chash_store_s *t);
int  chash_value_delete(int v, struct chash_store_s *t);
void chash_value_sort(struct chash_store_s *t);
unsigned long chash_value_hash(int v, struct chash_store_s *t);
void chash_destructor(struct chash_store_s *t);
int  chash_cnt_belongs2(char *host, struct chash_store_s *t);
int* chash_data_belongs2(char *host, struct chash_store_s *t);
void chash_value_print(struct chash_store_s *t);

int  v_belongs2(int v, struct chash_store_s *cs);
unsigned long get_md5(char *v);
int  host_handler(struct chash_store_s *t, char *s);

#endif /* CHASH_H_ */
