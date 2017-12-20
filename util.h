/*
 * util.h
 *
 *  Created on: Dec 11, 2017
 *      Author: dmcl216
 */

#ifndef UTIL_H_
#define UTIL_H_

#include "networking.h"

#include <stdint.h>

extern struct gossiper_s *g_gossiper;
extern struct chash_store_s *ch_store;

struct str_s {
	char *data;
	uint64_t len;
	uint64_t used;
};

struct value_store_s {
	int md5;
	int value;
};

void parse(struct raw_data *data);
int get_rand(int range);


#endif /* UTIL_H_ */
