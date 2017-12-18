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

	if (strlen(raw->read_buf) >= 6 && strncmp("gossip", raw->read_buf, 6) == 0) {
		char *back = "get gossip message";
		fprintf(stdout, "[info]get gossip message\n");
		handle_gossip_msg(g_gossiper, raw);
	} else {
		char *back = "can not recognize the operation.";
		memset(raw->write_buf, 0, MAXBUF);
		strcpy(raw->write_buf, back);
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
