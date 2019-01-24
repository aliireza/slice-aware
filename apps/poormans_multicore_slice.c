/* 
 * This program initialize 8 threads, one per core, and read/write from/to memory regions that are mapped to appropriate LLC slices.
 * The reading/writing operations will be done according to the input pattern (e.g., Uniform or Zipf)
 *
 * Copyright (c) 2019, Alireza Farshin, KTH Royal Institute of Technology - All Rights Reserved
 */


#include "../lib/memory-utils.c"
#include "../lib/cache-utils.c"
#include <sched.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define NUMBER_CORES 8
#define READ_TIMES 10000
#define PRINT_TIMES 10


/* Thread argument */
struct arg_struct {
	void **totalChunks;			/* Pointer to the allocated memory region */
	int coreID;					/* Core ID */
	unsigned long long size;	/* Size of the the allocated memory region */
	const char* access_pattern;	/* Name of the file which contains the access pattern */
};

/* Printf mutex */
static pthread_mutex_t printf_mutex;

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

/* 
 * Function to be called by each thread
 */

void* Run_Exp(void *arguments) {

	struct arg_struct *args = (struct arg_struct*) arguments;
	int coreID = args -> coreID;
	unsigned long long size = args -> size;
	void **totalChunks = args -> totalChunks;
	const char * access_pattern = args -> access_pattern;
	#ifdef HASWELL
		coreID*=2; /* This is related to core numbering of our system, i.e., cores 0,2,4,6,8,10,12,14 are located on socket 0 */
	#endif
	unsigned long long  i=0,k=0;
	int j=0;
	unsigned char read_var=0;
	/* Stride: To avoid prefetching */
	unsigned long long stride=1;
	
	//printf("Hi from thread %d, size: %llu, filename: %s\n", coreID, size, access_pattern);
	
	/* 
	 * Pin program to the input coreID
	 */
	CorePin(coreID);

	/* Read the access pattern from a file */
	unsigned long long *pattern=malloc(size*sizeof(unsigned long long));

	/* Open the file */
	FILE *fileptr;
	fileptr = fopen(access_pattern,"r");
	if(fileptr == NULL) {
		printf("Error!");
		exit(1);
	}

	/* Fill the pattern array */
	for(i=0;i<size;i++) {
		fscanf(fileptr, "%llu", &pattern[i]);
	}

	/* Print the pattern */
	// for(i=0;i<size;i++)	{
	// 	printf("%llu\n", pattern[i]);
	// }

	//printf("Ready! Press Enter!\n");
	//getchar();

	unsigned char *slice;

	/* Fill Arrays */
	for(i=0; i<size;i++) {
		slice=totalChunks[i];
		for(j=0;j<64;j++) {
			slice[j]=10;
		}
	}

	/* Flush Array */
	for(i=0; i<size;i++) {
		slice=totalChunks[i];
		for(j=0;j<64;j++) {
			_mm_clflush(&slice[j]);
		}
	}

	clock_t start;
	clock_t end;
	float seconds;
	float TPS;

	for(j=0;j<PRINT_TIMES;j++) {
		start = clock();
		for(k=0;k<READ_TIMES;k++) {

			for(i=0; i<size;i=i+stride) {
				//__builtin_prefetch(totalChunks[i+1], 0, 0); /* Uncomment for SW Prefetching */
				slice=totalChunks[pattern[i]];
				read_var=slice[0];	/* Read Latency */
				//slice[0]=30;		/* Write Latency */
			}
		}
		end = clock();
		seconds = (float)(end-start)/CLOCKS_PER_SEC;
		TPS=size*READ_TIMES/seconds;
		pthread_mutex_lock(&printf_mutex);
		printf("%llu\n", (unsigned long long)TPS);
		pthread_mutex_unlock(&printf_mutex);
	}
		
	/* Free the buffers */
	// free_buffer(buffer);
	// free(totalChunks);
	// free(totalChunksPhysical);

	pthread_exit(NULL);
}

int main(int argc, char **argv) {

	/*
	 * Check arguments: should contain size and access pattern filename
	 */

	if(argc!=3){
		printf("Wrong Input! Size and access pattern filename should be passed as input!\n");
		printf("Enter: %s <size> <access_pattern_file>\n", argv[0]);
		exit(1);
	}

	unsigned long long nTotalChunks;
	sscanf (argv[1],"%llu",&nTotalChunks);
	if(nTotalChunks < 0){
		printf("Wrong size! Size should be more than 0!\n");
		exit(1);   
	}

	const char * input_file=argv[2];

    struct arg_struct args[NUMBER_CORES];
	pthread_t threads[NUMBER_CORES];
	int t,rc;
	/* Pin the program to core 0 for initialization (polling) */
	CorePin(0);

	pthread_mutex_init(&printf_mutex, NULL);

	/* Initialize arrays for different cores */
	int i=0,c=0;
	for(c=0;c<NUMBER_CORES;c++) {
		int desiredSlice=c;

		/* Get a 1GB-hugepage */
		void *buffer = create_buffer();
		/* Calculate the physical address of the buffer */
		uint64_t bufPhyAddr = get_physical_address(buffer);
		/* Stride: To avoid prefetching */
		unsigned long long stride=1;
		/* Address to different chunks being mapped to the desired slice - Each 64 Byte (Virtual Address) */
		void ** totalChunks=malloc(nTotalChunks*sizeof(*totalChunks));
		/* Physical Address of chunks */
		unsigned long long *totalChunksPhysical=malloc(nTotalChunks*sizeof(*totalChunksPhysical));

		/* Find first chunk */
		#ifdef SKYLAKE
			unsigned long long offset = virtualSliceFinder_uncore(buffer,desiredSlice);
		#else
			unsigned long long offset = sliceFinder_HF_haswell(bufPhyAddr,desiredSlice);
		#endif

		totalChunks[0]=buffer+offset;
		totalChunksPhysical[0]= bufPhyAddr+offset;


		/* Find next chunks which are residing in the desired slice*/
		for(i=1;i<nTotalChunks; i++) {
			offset=64;
			#ifdef SKYLAKE
				offset+=virtualSliceFinder_uncore(totalChunks[i-1]+offset,desiredSlice);
			#else
				offset+=sliceFinder_HF_haswell(totalChunksPhysical[i-1]+offset,desiredSlice);
			#endif
			totalChunks[i]=totalChunks[i-1]+offset;
			totalChunksPhysical[i]=totalChunksPhysical[i-1]+offset;
			//if(i%1000==0) printf("FoundChunks:%llu/%llu\n",i,nTotalChunks);
		}
		args[c].totalChunks=totalChunks;
		//printf("Array %d initialized!\n",c);
	}

	/* Create threads */
	for(t=0; t<NUMBER_CORES; t++){
       //printf("In main: creating thread %d\n", t);
       args[t].coreID = t;
       args[t].size = nTotalChunks;
       args[t].access_pattern = input_file;
       rc = pthread_create(&threads[t], NULL, Run_Exp, (void *)&args[t]);
       if (rc){
          printf("ERROR; return code from pthread_create() is %d\n", rc);
          exit(0);
       }
    }

    pthread_exit(NULL);

	return 0;
}
