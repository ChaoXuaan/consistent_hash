/*
 * main.c
 *
 *  Created on: Dec 12, 2017
 *      Author: dmcl216
 */


struct gossiper_s *g_gossiper;

void on_start();
void timeout_cb(int fd, short ev, void *arg);
