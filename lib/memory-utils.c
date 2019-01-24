/* 
 * Memory-related functions for creating a buffer and getting its physical address
 *
 * Copyright (c) 2019, Alireza Farshin, KTH Royal Institute of Technology - All Rights Reserved
 */

#define _GNU_SOURCE
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

/*
 * Definitions + mmap Flags
 */

#define USE_HUGEPAGE	/* Should be defined for allocating hugepages; comment it for 4KB-pages */

#define SIZE (8*1024UL*1024*1024)	/* Buffer Size -> 8*1GB */

#define PROTECTION (PROT_READ | PROT_WRITE)	/* Protection of the mapping: page may be read and written */

#ifndef MAP_HUGETLB	/* Use hugepages */
#define MAP_HUGETLB 0x40000 /* arch. specific */
#endif

/* Only ia64 requires this */
#ifdef __ia64__
#define ADDR (void *)(0x8000000000000000UL)	/* the kernel takes it as a hint about where to place the mapping */
/* Flags: Can use [MAP_HUGE_2MB|MAP_HUGE_1GB] instead of MAP_HUGE_1GB */
#define FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_FIXED | MAP_HUGE_1GB)
#else
#define ADDR (void *)(0x0UL)
#define FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB)
#endif


/*
 * Create buffer backed by a hugepage
 */

void* create_buffer(void) {

	#ifdef USE_HUGEPAGE
	/* Allocate some memory using mmap (hugepage 1GB/2MB) based on the input FLAGS */
	void *buffer = mmap(ADDR, SIZE, PROTECTION, FLAGS, 0, 0);
	if (buffer == MAP_FAILED) {
		fprintf(stderr, "Failed to allocate memory for buffer\n");
		exit(1);
	}
	#else
	/* Allocate some memory using malloc (4KB-page) */
	void *buffer = malloc(buf_size);
	if(buffer == NULL) {
		fprintf(stderr, "Failed to allocate memory for buffer\n");
		exit(1);
	}
	#endif
	/* 
	 * Lock the page in the memory
	 * Do this before writing data to the buffer so that any copy-on-write
	 * mechanisms will give us our own page locked in memory
	 */
	if(mlock(buffer, SIZE) == -1) {
		fprintf(stderr, "Failed to lock page in memory: %s\n", strerror(errno));
		exit(1);
	}
	return buffer;
}


/*
 * Free buffer 
 */ 

void free_buffer(void* buffer) {
	/* munmap() length of MAP_HUGETLB memory must be hugepage aligned */
	if (munmap(buffer, SIZE)) {
		perror("munmap");
		exit(EXIT_FAILURE);
	}
}

/*
 * Virtual Address to Physical Address Translation by using /proc/self/pagemap
 * Inspired by http://fivelinesofcode.blogspot.com/2014/03/how-to-translate-virtual-to-physical.html
 * and https://shanetully.com/2014/12/translating-virtual-addresses-to-physcial-addresses-in-user-space/
 *
 *
 *
 * The page frame shifted left by PAGE_SHIFT will give us the physcial address of the frame
 * Note that this number is architecture dependent. For x86_64 with 4096 page sizes,
 * it is defined as 12. If you're running something different, check the kernel source
 * for what it is defined as.
 */

#define PAGE_SHIFT 12
#define PAGEMAP_LENGTH 8

/* 
 * Get the page frame number
 */

uint64_t get_page_frame_number_of_address(void *address) {

	/* Open the pagemap file for the current process */
	FILE *pagemap = fopen("/proc/self/pagemap", "rb");

	/* Seek to the page that the buffer is on it */ 
	uint64_t offset = (uint64_t)((uint64_t)address >> PAGE_SHIFT) * (uint64_t)PAGEMAP_LENGTH;
	if(fseek(pagemap, (uint64_t)offset, SEEK_SET) != 0) {
		fprintf(stderr, "Failed to seek pagemap to proper location\n");
		exit(1);
	}

	/* The page frame number is in bits 0-54 so read the first 7 bytes and clear the 55th bit */
	uint64_t page_frame_number = 0;
	fread(&page_frame_number, 1, PAGEMAP_LENGTH-1, pagemap);
	page_frame_number &= 0x7FFFFFFFFFFFFF;
	fclose(pagemap);
	return page_frame_number;
}


/*
 * Get the physical address of a page: 
 */

uint64_t get_physical_address(void* address) {

	/* Get page frame number */
	unsigned int page_frame_number = get_page_frame_number_of_address(address);

	/* Find the difference from the buffer to the page boundary */
	uint64_t distance_from_page_boundary = (uint64_t)address % getpagesize();

	/* Determine how far to seek into memory to find the buffer */
	uint64_t physical_address = (uint64_t)((uint64_t)page_frame_number << PAGE_SHIFT) + (uint64_t)distance_from_page_boundary;

	return physical_address;
}