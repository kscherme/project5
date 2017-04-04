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

typedef enum {RAND, FIFO, CUSTOM} mode_t;

// Global Variables
mode_t MODE;
int *FRAME_ARRAY;
struct disk *DISK;
int FIRST_IN_FRAME;
int *WRITTEN_TO;

int PAGE_FAULTS;
int DISK_READS;
int DISK_WRITES;

void page_fault_handler( struct page_table *pt, int page )
{
	// Print the page # of the fault
	//printf("page fault on page #%d\n",page);

	// Add to number of page faults
	PAGE_FAULTS++;

	// Check if page is already loaded
	int frame;
	int bits;
	page_table_get_entry( pt, page, &frame, &bits );
	// printf("frame: %d\n", frame);
	printf("bits: %d\n", bits);

	// If page only has read permission, set write permission and continue
	printf("%d\n", (bits & PROT_WRITE));
	if (bits != 0) {
		page_table_set_entry(pt, page, frame, bits|PROT_WRITE);
		if (MODE == CUSTOM) WRITTEN_TO[frame] = 0;
		return;
	}

	// Check for an open frame
	int nframes = page_table_get_nframes(pt);
	int i;
	int open_frame = -1;
	for (i=0; i < nframes; i++) {
		if (FRAME_ARRAY[i] == -1) {
			open_frame = i;
			break;
		}
	}

	// If no open frame
	char *physmem = page_table_get_physmem(pt);
	if (open_frame == -1) {
		switch(MODE) {
			case RAND: 
				// Replace a random frame
				open_frame = lrand48() % nframes;
				break;
			case FIFO:
				// Replace first in frame
				open_frame = FIRST_IN_FRAME;
				FIRST_IN_FRAME = (FIRST_IN_FRAME + 1) % nframes;
				break;
			case CUSTOM:
				// Use fifo but skip if it was recently written to
				open_frame = FIRST_IN_FRAME;

				while (WRITTEN_TO[open_frame] == 0) {
					WRITTEN_TO[open_frame] = -1;
					FIRST_IN_FRAME = (FIRST_IN_FRAME + 1) % nframes;
					open_frame = FIRST_IN_FRAME;					
				}

				FIRST_IN_FRAME = (FIRST_IN_FRAME + 1) % nframes;
				break;
		}


		// Check if the old page is dirty
		page_table_get_entry(pt, FRAME_ARRAY[open_frame], &frame, &bits);

		// If it is dirty, write back onto the disk
		if (bits & PROT_WRITE) {
			disk_write(DISK, FRAME_ARRAY[open_frame], &physmem[open_frame*PAGE_SIZE]);
			DISK_WRITES++;
		}

		// Change the old page table entry
		page_table_set_entry(pt, FRAME_ARRAY[open_frame], 0, 0);
	}

	// Read in page from disk
	disk_read(DISK, page, &physmem[open_frame*PAGE_SIZE]);
	DISK_READS++;

	// Set page table entry
	page_table_set_entry(pt, page, open_frame, PROT_READ);
	FRAME_ARRAY[open_frame] = page;

	// Print frame array
	// int x;
	// for (x=0; x< nframes; x++){
	// 	printf("%d\n", FRAME_ARRAY[x]);
	// }
	page_table_print(pt);
	printf("\n\n");
}

int main( int argc, char *argv[] )
{
	// Parse command line arguments
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}

	// Initizalize summary components to 0
	PAGE_FAULTS = 0;
	DISK_READS = 0;
	DISK_WRITES = 0;

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
	int temp_array2[nframes];
	FRAME_ARRAY = temp_array;
	WRITTEN_TO = temp_array2;

	int i;
	for(i=0; i < nframes; i++){
		FRAME_ARRAY[i] = -1;
		WRITTEN_TO[i] = -1;
	}

	// Set first in frame to 0
	FIRST_IN_FRAME = 0;


	// Handles page replacement algorithm
	if( strcmp(argv[3], "rand") == 0) {
		MODE = RAND;
	}
	else if( strcmp(argv[3], "fifo") == 0) {
		MODE = FIFO;
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

	//Print summary
	printf("Number of page faults: %d\nNumber of disk reads: %d\nNumber of disk writes: %d\n", PAGE_FAULTS, DISK_READS, DISK_WRITES);

	return 0;
}