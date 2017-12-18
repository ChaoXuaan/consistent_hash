/*
 * range_test.c
 *
 *  Created on: Dec 15, 2017
 *      Author: dmcl216
 */

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

int main(int argc, void **argv) {
	int i, ret;

	for (i = 0; i < 20; i++) {
		ret = get_rand(50);
		fprintf(stdout, "%d\n", ret);
	}

	return 0;
}
