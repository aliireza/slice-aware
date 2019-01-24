/* 
 * This program will find mapping between physical address and slices
 *
 * Copyright (c) 2019, Alireza Farshin, KTH Royal Institute of Technology - All Rights Reserved
 */

#include "../lib/memory-utils.c"
#include "../lib/cache-utils.c"
#include <sched.h>
#include <inttypes.h>

#define HUGEPAGE_NUM 1 /* Number of used hugepages */

/* Convert decimal to binary */
char* convert(int dec, char* bits, int nBits) {
	int i=0;
	for (i=0;i<nBits;i++) {
		bits[nBits-1-i]=dec%2+'0';
		dec/=2;
	}
	bits[nBits] = '\0';
	return bits;
}

int main(int argc, char **argv) {
	
	/* Pin to core 0 */
	int mask = 0;
	cpu_set_t my_set;
	CPU_ZERO(&my_set);
	CPU_SET(mask, &my_set);
	sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	/* Create a buffer with some data in it */
	void **bufferList=malloc(HUGEPAGE_NUM*sizeof(*bufferList));
	int k=0;
	for(k=0;k<HUGEPAGE_NUM;k++) {
		bufferList[k] = create_buffer();
	}

	for(k=0;k<HUGEPAGE_NUM;k++) {
		/* Get physical address of the buffer */
		uint64_t physical_address=get_physical_address(bufferList[k]);

		/* Iterate through the 1GB-page */
		uint64_t offset=0;
		uint8_t slice_number=0;
		char bits[5+1];
		while(offset<1024*1024*1024) {

			/* Print Physical Address */
			printf("%lx\t",physical_address+offset);

			/* Find the slice number */
			slice_number=calculateSlice_uncore(bufferList[k]+offset);

			/* Print the slice number */
			printf("%d\t",slice_number);
			printf("%s\n",convert(slice_number,bits,5));
			offset+=64;
		}
	}
	return 0;
}

