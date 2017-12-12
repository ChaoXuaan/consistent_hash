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
