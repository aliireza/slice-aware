/* 
 * This program will find cycles required to access a specific slice from one core
 *
 * Copyright (c) 2019, Alireza Farshin, KTH Royal Institute of Technology - All Rights Reserved
 */

#include "../lib/memory-utils.c"
#include "../lib/cache-utils.c"
#include <sched.h>
#include <inttypes.h>
#include <stdlib.h>

#define NUMBER_CORES 8
#define READ_TIMES 1000

/*
 * Pin program to the input core
 */

void CorePin(int coreID)
{
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(coreID,&set);
	if(sched_setaffinity(0, sizeof(cpu_set_t), &set) < 0) {
		printf("\nUnable to Set Affinity\n");
		exit(EXIT_FAILURE);
	}
}


int main(int argc, char **argv) {

	/*
	 * Check arguments: should contain coreID and slice number
	 */

	if(argc!=3){
		printf("Wrong Input! Enter desired slice and coreID!\n");
		printf("Enter: %s <coreID> <sliceNumber>\n", argv[0]);
		exit(1);
	}

	int coreID;
	sscanf (argv[1],"%d",&coreID);
	if(coreID > NUMBER_CORES*2-1 || coreID < 0){
		printf("Wrong Core! CoreID should be less than %d and more than 0!\n", NUMBER_CORES);
		exit(1);   
	}

	int desiredSlice;
	sscanf (argv[2],"%d",&desiredSlice);
	if(desiredSlice > NUMBER_SLICES-1 || desiredSlice < 0){
		printf("Wrong slice! Slice should be less than %d and more than 0!\n", NUMBER_SLICES);
		exit(1);   
	}

	/* 
	 * Ping program to core-0 for finding chunks
	 * Later the program will be pinned to the desired coreID
	 */
	CorePin(0);

	/* Get a 1GB-hugepage */
	void *buffer = create_buffer();

	/* Calculate the physical address of the buffer */
	uint64_t bufPhyAddr = get_physical_address(buffer);

	#ifdef SKYLAKE
	/* Memory Chunks -> Fit in LLC */
	unsigned long long nTotalChunks=(unsigned long long)LLC_WAYS/2+L2_WAYS;
	/* Memory Chunks -> Fit in L2 */
	unsigned long long nL2Chunks=(unsigned long long)LLC_WAYS/2;
	#else
	/* Memory Chunks -> Fit in LLC */
	unsigned long long nTotalChunks=2*L2_WAYS;
	/* Memory Chunks -> Fit in L2 */
	unsigned long long nL2Chunks=L2_WAYS;
	#endif

	/* Memory Chunks -> Fit in L1 */
	unsigned long long nL1Chunks=L1_WAYS;

	/* Stride: Can be used to avoid prefetching */
	unsigned long long stride=1;

	/* Variables for calculating cache indexes */
	uint64_t index3, index2, index1;

	/* Address to different chunks that are mapped to the desired slice - each 64 Byte (Virtual Address) */
	void ** totalChunks=malloc(nTotalChunks*sizeof(*totalChunks));

	/* Physical Address of chunks */
	unsigned long long *totalChunksPhysical=malloc(nTotalChunks*sizeof(*totalChunksPhysical));

	unsigned long long  i=0;
	int j=0,k=0;

	/* Find first chunk */
	unsigned long long offset = sliceFinder_uncore(buffer,desiredSlice);

	totalChunks[0]=buffer+offset;
	totalChunksPhysical[0]= bufPhyAddr+offset;

	/* Find the Indexes (Set number in cache hierarychy) */
	index3=indexCalculator(totalChunksPhysical[0],3);
	index2=indexCalculator(totalChunksPhysical[0],2);
	index1=indexCalculator(totalChunksPhysical[0],1);

	/* Find next chunks which are residing in the desired slice and the same sets in L3/L2/L1*/
	for(i=1;i<nTotalChunks; i++) {
		offset=L3_INDEX_STRIDE;
		while(desiredSlice!=calculateSlice_uncore(totalChunks[i-1]+offset) || index1!=indexCalculator(totalChunksPhysical[i-1]+offset,1) || index2!=indexCalculator(totalChunksPhysical[i-1]+offset,2) || index3!=indexCalculator(totalChunksPhysical[i-1]+offset,3)) {
			offset+=L3_INDEX_STRIDE;
		}
		totalChunks[i]=totalChunks[i-1]+offset;
		totalChunksPhysical[i]=totalChunksPhysical[i-1]+offset;
	}

	/* validate chunks: whether they are on the desired slice or not */
	for(i=0;i<nTotalChunks;i++) {
		if(desiredSlice!=calculateSlice_uncore(totalChunks[i])) {
			printf("Error!");
			exit(EXIT_FAILURE);
		}
	}
        
	/* Ping program to coreID */
    CorePin(coreID);

	unsigned char *slice;

	for(k=0;k<READ_TIMES;k++) {
		/* Fill Arrays */
		for(i=0; i<nTotalChunks;i++) {
			slice=totalChunks[i];
			for(j=0;j<64;j++) {
				slice[j]=10+20;
			}
		}

		/* Flush Array */
		for(i=0; i<nTotalChunks;i++) {
			slice=totalChunks[i];
			for(j=0;j<64;j++) {
				_mm_clflush(&slice[j]);
			}
		}

		register uint64_t time1, time2;
		unsigned cycles_high, cycles_low, cycles_high1, cycles_low1;
		unsigned int val=0;

		/* Read Array: Gives Memory Access Time*/
		for(i=0; i<nTotalChunks;i=i+stride) {
			asm volatile ("CPUID\n\t"
				"RDTSC\n\t"
				"mov %%edx, %0\n\t"
				"mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low):: "rax", "rbx", "rcx", "rdx");
			/* Measured operation */
			val=*(unsigned int*)totalChunks[i];

			asm volatile ("RDTSCP\n\t"
				"mov %%edx, %0\n\t"
				"mov %%eax, %1\n\t"
				"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1):: "rax", "rbx", "rcx", "rdx");
			time1= (((uint64_t)cycles_high << 32) | cycles_low);
			time2= (((uint64_t)cycles_high1 << 32) | cycles_low1);
			/* Print Memory Access Time */
			//printf("%lu\n", time2-time1);
		}

		/* Gives LLC Access Time*/
		for(i=0; i<nL2Chunks;i=i+stride) {
			slice=totalChunks[i];
			asm volatile ("CPUID\n\t"
				"RDTSC\n\t"
				"mov %%edx, %0\n\t"
				"mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low):: "rax", "rbx", "rcx", "rdx");

			/* Measured operation */
			val=*slice;

			asm volatile ("RDTSCP\n\t"
				"mov %%edx, %0\n\t"
				"mov %%eax, %1\n\t"
				"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1):: "rax", "rbx", "rcx", "rdx");
			time1= (((uint64_t)cycles_high << 32) | cycles_low);
			time2= (((uint64_t)cycles_high1 << 32) | cycles_low1);
			/* Print LLC Access Time */
			printf("%lu\n", time2-time1);
		}

	}

	/* Free the buffers */
	free_buffer(buffer);
	free(totalChunks);
	free(totalChunksPhysical);

	return 0;
}
