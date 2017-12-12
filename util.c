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
#include <assert.h>

void parse(struct raw_data *raw) {
	assert (raw);

	if (strlen(raw->read_buf) >= 6 && strncmp("gossip", raw->read_buf, 6) == 0) {
		handle_gossip_msg(raw);
	}
}
