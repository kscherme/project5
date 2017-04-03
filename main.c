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
int *FRAME_ARRAY;
struct disk *DISK;

void page_fault_handler( struct page_table *pt, int page )
{
	//page_table_set_entry(pt,page,page,(PROT_READ|PROT_WRITE));
	// page_table_print(pt);

	// Print the page # of the fault
	printf("page fault on page #%d\n",page);

	// Check if page is already loaded
	int frame;
	int bits;
	page_table_get_entry( pt, page, &frame, &bits );
	// printf("frame: %d\n", frame);
	// printf("bits: %d\n", bits);

	// If page only has read permission, set write permission and continue
	if (bits == 1) {
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
		return;
	}

	// Check for an open frame
	int nframes = page_table_get_nframes(pt);
	int i;
	int open_frame = -1;
	int found_frame = 0;
	for (i=0; i < nframes; i++) {
		if (FRAME_ARRAY[i] == -1) {
			open_frame = i;
			found_frame = 1;
			break;
		}
	}

	// If no open frame
	if (found_frame == 0) {
		if (MODE == RAND) {
			break;
		}
	}

	// Read in page from disk
	char *physmem = page_table_get_physmem(pt);
	//disk_read(DISK, page, open_frame*PAGE_SIZE + physmem);
	disk_read(DISK, page, &physmem[open_frame*FRAME_SIZE]);

	// Set page table entry
	page_table_set_entry(pt, page, open_frame, PROT_READ);
	FRAME_ARRAY[open_frame] = page;

	// Print frame array
	int x;
	for (x=0; x< nframes; x++){
		printf("%d\n", FRAME_ARRAY[x]);
	}



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

	// Make frame table array
	int temp_array[nframes];
	FRAME_ARRAY = temp_array;

	int i;
	for(i=0; i < nframes; i++){
		FRAME_ARRAY[i] = -1;
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
	DISK = disk_open("myvirtualdisk",npages);
	if(!DISK) {
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
	disk_close(DISK);

	return 0;
}