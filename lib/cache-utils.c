/* 
 * LLC-slice-related functions for calculating the slice number and finding the appropriate offset
 *
 * Copyright (c) 2019, Alireza Farshin, KTH Royal Institute of Technology - All Rights Reserved
 */

#include <inttypes.h>
#include "msr-utils.c"

/* 
 * Architecture dependent values for LLC hash function
 *
 * Xeon-E5-2667 v3 (Haswell)
 * 8 cores in each socket -> 8 slices (2.5MB)
 * Each physical address will be mapped to a slice by XOR-ing the selected bits
 * The bits in physical_addr will be selected by using a hash_x
 * x=0,1,2
 * Then we will XOR all of those bits
 * Bitx = XOR( and(physical_addr,hash_x))
 * Output slice is the decimal value of: (Bit2.Bit1.Bit0)
 * 
 * Xeon-Gold-6134 (SkyLake Server)
 * 8 cores in each socket and 18 slices per socket (1.375MB)
 * Mapping is not known yet
*/

/*
 * Haswell hash
 */

/* Number of hash functions/Bits for slices */
#define bitNum 3
/* Bit0 hash */
#define hash_0 0x1B5F575440
/* Bit1 hash */
#define hash_1 0x2EB5FAA880
/* Bit2 hash */
#define hash_2 0x3CCCC93100


/*
 * Cache hierarchy characteristics
 */

/* TODO: Detect and define the values automatically by using data in: "/sys/devices/system/cpu/cpu[n]/cache/..." */

#define LINE  64
#define L1_SIZE (32UL*1024)
#define L1_WAYS 8
#define L1_SETS (L1_SIZE/LINE)/(L1_WAYS)

#define SKYLAKE_LLC_SIZE (24.75*1024*1024UL)
#define SKYLAKE_LLC_WAYS 11
#define SKYLAKE_LLC_SETS (LLC_SIZE/LINE)/(LLC_WAYS*NUMBER_SLICES)
#define SKYLAKE_SLICE_SIZE (LLC_SIZE/(NUMBER_SLICES))
#define SKYLAKE_L2_SIZE (1UL*1024*1024)
#define SKYLAKE_L2_WAYS 16
#define SKYLAKE_L2_SETS (L2_SIZE/LINE)/(L2_WAYS)

#define HASWELL_LLC_SIZE (20UL*1024*1024)
#define HASWELL_LLC_WAYS 20
#define HASWELL_LLC_SETS (LLC_SIZE/LINE)/(LLC_WAYS*NUMBER_SLICES)
#define HASWELL_SLICE_SIZE (LLC_SIZE/(NUMBER_SLICES))
#define HASWELL_L2_SIZE (256UL*1024)
#define HASWELL_L2_WAYS 8
#define HASWELL_L2_SETS (L2_SIZE/LINE)/(L2_WAYS)

/* Set indexes */

#define SKYLAKE_L3_INDEX_PER_SLICE 0x1FFC0 /* 11 bits - [16-6] - 2048 sets per slice + 11 way for each slice (1.375MB) */
#define SKYLAKE_L2_INDEX 0xFFC0 /* 10 bits - [15-6] - 1024 sets + 16 way for each core  */
#define SKYLAKE_L1_INDEX 0xFC0 /* 6 bits - [11-6] - 64 sets + 8 way for each core  */
#define SKYLAKE_L3_INDEX_STRIDE 0x20000 /* Offset required to get the same indexes bit 17 = bit 16 (MSB bit of L3_INDEX_PER_SLICE) + 1 */
#define SKYLAKE_L2_INDEX_STRIDE 0x10000 /* Offset required to get the same indexes bit 16 = bit 15 (MSB bit of L2_INDEX) + 1 */

#define HASWELL_L3_INDEX_PER_SLICE 0x1FFC0 /* 11 bits - [16-6] - 2048 sets per slice + 20 way for each slice (2.5MB) */
#define HASWELL_L2_INDEX 0x7FC0 /* 9 bits - [14-6] - 512 sets + 8 way for each core  */
#define HASWELL_L1_INDEX 0xFC0 /* 6 bits - [11-6] - 64 sets + 8 way for each core  */
#define HASWELL_L3_INDEX_STRIDE 0x20000 /* Offset required to get the same indexes bit 17 = bit 16 (MSB bit of L3_INDEX_PER_SLICE) + 1 */
#define HASWELL_L2_INDEX_STRIDE 0x8000 /* Offset required to get the same indexes bit 15 = bit 14 (MSB bit of L2_INDEX) + 1 */

#ifdef SKYLAKE 
#define L3_INDEX_PER_SLICE SKYLAKE_L3_INDEX_PER_SLICE
#define L2_INDEX SKYLAKE_L2_INDEX
#define L1_INDEX SKYLAKE_L1_INDEX
#define LLC_SIZE SKYLAKE_LLC_SIZE
#define LLC_WAYS SKYLAKE_LLC_WAYS
#define LLC_SETS SKYLAKE_LLC_SETS
#define SLICE_SIZE SKYLAKE_SLICE_SIZE
#define L2_SIZE SKYLAKE_L2_SIZE
#define L2_WAYS SKYLAKE_L2_WAYS
#define L2_SETS SKYLAKE_L2_SETS
#define L3_INDEX_STRIDE SKYLAKE_L3_INDEX_STRIDE
#define L2_INDEX_STRIDE SKYLAKE_L2_INDEX_STRIDE
#else
#define L3_INDEX_PER_SLICE HASWELL_L3_INDEX_PER_SLICE
#define L2_INDEX HASWELL_L2_INDEX
#define L1_INDEX HASWELL_L1_INDEX
#define LLC_SIZE HASWELL_LLC_SIZE
#define LLC_WAYS HASWELL_LLC_WAYS
#define LLC_SETS HASWELL_LLC_SETS
#define SLICE_SIZE HASWELL_SLICE_SIZE
#define L2_SIZE HASWELL_L2_SIZE
#define L2_WAYS HASWELL_L2_WAYS
#define L2_SETS HASWELL_L2_SETS
#define L3_INDEX_STRIDE HASWELL_L3_INDEX_STRIDE
#define L2_INDEX_STRIDE HASWELL_L2_INDEX_STRIDE
#endif

/* 
 * Function for XOR-ing all bits
 * ma: masked physical address
 */

uint64_t
rte_xorall64(uint64_t ma) {
	return __builtin_parityll(ma);
}

/* Calculate slice based on the physical address - Haswell */

uint8_t
calculateSlice_HF_haswell(uint64_t pa) {
	uint8_t sliceNum=0;
	sliceNum= (sliceNum << 1) | (rte_xorall64(pa&hash_2));
	sliceNum= (sliceNum << 1) | (rte_xorall64(pa&hash_1));
	sliceNum= (sliceNum << 1) | (rte_xorall64(pa&hash_0));
	return sliceNum;
}

/* Calculate the slice based on a given virtual address - Haswell and SkyLake */

uint8_t
calculateSlice_uncore(void* va) {
	/*
	 * The registers' address and their values would be selected in msr-utils.c
	 * check_cpu.sh will automatically define the proper architecture (i.e., #define HASWELL or #define SKYLAKE).
	 */
	uncore_init();
	polling(va);
	return find_CHA_CBO();
}


/* 
 * Calculate virtual slice based on the virtual address - SkyLake 
 * Since SkyLake has 18 slices and 8 cores, we tag every slice with a number between 0 to 7, which represent the virtual slice
 * The mapping with virtual slice number and number of cores is similar to Haswell architecture, i.e., slice ith is the closest to core ith.
 * The mapping for virtual slices is as follows:
 * VS0 -> S0/S2/S6
 * VS1 -> S4/S1
 * VS2 -> S8/S11
 * VS3 -> S12/S13
 * VS4 -> S10/S7/S9
 * VS5 -> S14/S16
 * VS6 -> S3/S5
 * VS7 -> S15/S17
 */

uint8_t
calculateVirtualSlice_uncore(void* va) {
	uint8_t slice = calculateSlice_uncore(va);
	uint8_t virtualSlice=0;

	if (slice==0 || slice ==2 || slice==6){
		virtualSlice=0;
	} else if (slice==4 || slice==1){
		virtualSlice=1;
	} else if (slice==8 || slice==11){
		virtualSlice=2;
	} else if (slice==12 || slice==13){
		virtualSlice=3;
	} else if (slice==10 || slice==7 || slice==9){
		virtualSlice=4;
	} else if (slice==14 || slice==16){
		virtualSlice=5;
	} else if (slice==3 || slice==5){
		virtualSlice=6;
	} else if (slice==15 || slice==17){
		virtualSlice=7;
	}

	return virtualSlice;
}

/* Find the next chunk that is mapped to the input slice number - with Haswell hash function */

uint64_t
sliceFinder_HF_haswell(uint64_t pa, uint8_t desiredSlice) {
	uint64_t offset=0;
	while(desiredSlice!=calculateSlice_HF_haswell(pa+offset)) {
		/* Slice mapping will change for each cacheline which is 64 Bytes */
		offset+=LINE;
	}
	return offset;
}

/* Find the next chunk that is mapped to the input slice number - with Haswell/Skylake uncore performance counters */

uint64_t
sliceFinder_uncore(void* va, uint8_t desiredSlice) {
	uint64_t offset=0;
	while(desiredSlice!=calculateSlice_uncore(va+offset)) {
		/* Slice mapping will change for each cacheline which is 64 Bytes */
		offset+=LINE;
	}
	return offset;
}


/* 
 * Find the next virtual slice for the input virtual slice (core number) based on the mapping found previously
 * The mapping is as follows:
 * C0 -> VS0 -> S0/S2/S6
 * C1 -> VS1 -> S4/S1
 * C2 -> VS2 -> S8/S11
 * C3 -> VS3 -> S12/S13
 * C4 -> VS4 -> S10/S7/S9
 * C5 -> VS5 -> S14/S16
 * C6 -> VS6 -> S3/S5
 * C7 -> VS7 -> S15/S17
 */

uint64_t
virtualSliceFinder_uncore(void* va, uint8_t desiredVirtualSlice) {
	uint64_t offset=0;

	while(desiredVirtualSlice!=calculateVirtualSlice_uncore(va+offset)) {
		/* Slice mapping will change for each cacheline which is 64 Bytes */
		offset+=LINE;
	}
	return offset;
}


/* Calculate the index of set for a given physical address */

uint64_t 
indexCalculator(uint64_t addr_in, int cacheLevel) {

	uint64_t index;

	if (cacheLevel==1)
	{
		index=(addr_in)&L1_INDEX;
		index = index >> 6;
	}
	else if (cacheLevel==2)
	{
		index=(addr_in)&L2_INDEX;
		index = index >> 6;
	}
	else if (cacheLevel==3)
	{
		index=(addr_in)&L3_INDEX_PER_SLICE;
		index = index >> 6;
	}
	else
	{
		exit(EXIT_FAILURE);
	}
	return index;
}
