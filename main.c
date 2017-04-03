/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef enum {RAND, FIFO, LRU, CUSTOM} mode_t;

// Global Variables
mode_t MODE;

void page_fault_handler( struct page_table *pt, int page )
{
	page_table_set_entry(pt,page,page,(PROT_READ|PROT_WRITE));
	page_table_print(pt);
	// Print the page # of the fault
	printf("page fault on page #%d\n",page);


	//exit(1);
}

int main( int argc, char *argv[] )
{
	// Parse command line arguments
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|lru|custom> <sort|scan|focus>\n");
		return 1;
	}

	// Handles number of pages and frames
	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	if (npages <= 0){
		printf("ERROR: npages must be greater than 0\n");
		return 1;
	}
	else if(nframes <=0){
		printf("ERROR: nframes must be greater than 0\n");
		return 1;
	}

	// Handles page replacement algorithm
	if( strcmp(argv[3], "rand") == 0) {
		MODE = RAND;
	}
	else if( strcmp(argv[3], "fifo") == 0) {
		MODE = FIFO;
	} 
	else if( strcmp(argv[3], "lru") == 0) {
		MODE = LRU;
	}
	else if( strcmp(argv[3], "custom") == 0) {
		MODE = CUSTOM;
	}
	else {
		printf("ERROR: the third argument must be <rand|fifo|lru|custom>\n");
		return 1;
	}

	// Handles which built in program to run
	const char *program = argv[4];

	// Create virtual disk
	struct disk *disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}

	// Create page table
	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	// Pointer to the start of virtual memory associated with the page table
	char *virtmem = page_table_get_virtmem(pt);

	// Pointer to the start of physical memory associated with the page table
	char *physmem = page_table_get_physmem(pt);

	// Start program
	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);
		return 1;
	}

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}