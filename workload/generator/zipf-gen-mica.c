/* 
 * This program generates random number with zipf distribution
 *
 * Copyright (c) 2019, Alireza Farshin, KTH Royal Institute of Technology - All Rights Reserved
 */

#include <stdio.h>
#include "zipf.h"

int main(int argc, char **argv)
{

	if(argc!=2){
		printf("Wrong Input! Enter Range>0!\n");
		printf("Enter: %s <Range>\n", argv[0]);
		exit(1);
	}

	unsigned long long range;
	sscanf (argv[1],"%llu",&range);
	if(range < 0){
		printf("Wrong Range! Range should be more than 0!\n");
		exit(1);   
	}

	double theta = 0.99;
	const uint64_t n = range;
	uint64_t i;
	struct zipf_gen_state state;
	unsigned long long tmp;
	mehcached_zipf_init(&state, n, theta, 0);
	for (i = 0; i < n; i++) {
		tmp = (unsigned long long)mehcached_zipf_next(&state);
		printf("%llu\n", tmp);
		if(tmp >n)
			printf("Error!\n");
	}

	return 0;
}
