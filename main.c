/*
 * main.c
 *
 *  Created on: Dec 12, 2017
 *      Author: dmcl216
 */


struct gossiper_s *g_gossiper;
struct chash_store_s *ch_store;

void on_start();
void timeout_cb(int fd, short ev, void *arg);
