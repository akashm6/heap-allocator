#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "p3Heap.h"

typedef struct blockHeader
{
	int size_status;
} blockHeader;

blockHeader *heap_start = NULL;
int alloc_size;
const int header = sizeof(blockHeader);
const int end = 1;

int isFree(int size)
{
	return ((size % 8 == 0 || size % 8 == 2)) ? 0 : 1;
}

void split(blockHeader *best_fit, int pad_size, blockHeader *next, int min_size)
{
	int next_size = min_size - pad_size;
	next->size_status = next_size;
	int alloc_size = ++pad_size;
	best_fit->size_status = alloc_size;
	best_fit->size_status += 2;
	next->size_status += 2;
	return;
}

void *balloc(int size)
{
	if (size < 1)
	{
		return NULL;
	}
	blockHeader *best_fit = NULL;
	int pad_size = size + header;
	pad_size = (pad_size % 8 == 0) ? pad_size : pad_size + (8 - (pad_size % 8));
	int min_size = alloc_size + 1;
	blockHeader *curr = heap_start;
	while (curr->size_status != end)
	{
		int curr_size = curr->size_status;
		int original = curr_size - (curr_size % 8);
		if (isFree(curr_size) == 0)
		{
			if (original >= pad_size && original <= min_size)
			{
				best_fit = curr;
				min_size = original;
			}
		}
		blockHeader *next_block = (blockHeader *)((char *)curr + original);
		curr = next_block;
	}
	blockHeader *next = (blockHeader *)((char *)best_fit + pad_size);
	if (best_fit == NULL)
	{
		return NULL;
	}
	if (min_size == pad_size)
	{
		next->size_status = (next->size_status == 1) ? next->size_status : next->size_status + 2;
		best_fit->size_status++;
	}
	else
	{
		split(best_fit, pad_size, next, min_size);
		int ftr_increment = min_size - pad_size - 4;
		blockHeader *ftr = (blockHeader *)((char *)next + ftr_increment);
		ftr->size_status = ftr_increment + 4;
	}
	void *payload = (char *)best_fit + 4;
	return payload;
}

int bfree(void *ptr)
{
	blockHeader *bptr = (blockHeader *)((char *)ptr - 4);
	if (ptr == NULL || (int)ptr % 8 != 0)
	{
		return -1;
	}
	blockHeader *end_mark = (blockHeader *)((char *)heap_start + alloc_size);
	if (bptr < heap_start || bptr >= end_mark)
	{
		return -1;
	}
	int block_size = bptr->size_status;
	if (isFree(block_size) == 0)
	{
		return -1;
	}
	block_size = block_size - (block_size % 8);
	bptr->size_status = block_size;
	blockHeader *next = (blockHeader *)((char *)bptr + block_size);
	if (isFree(next->size_status) == 0 && next->size_status != 1)
	{
		int next_size = next->size_status - next->size_status % 8;
		bptr->size_status += next_size;
	}
	else
	{
		next->size_status -= 2;
	}
	blockHeader *prev_ftr = (blockHeader *)((char *)bptr - 4);
	int prev_size = prev_ftr->size_status;
	blockHeader *prev = (blockHeader *)((char *)prev_ftr - (prev_size - 4));
	if (prev_size != 0 && prev_ftr >= heap_start && isFree(prev->size_status) == 0)
	{
		int prev_bits = prev->size_status % 8;
		bptr->size_status += prev->size_status - (prev_bits);
		bptr->size_status += prev_bits;
		prev->size_status = bptr->size_status;
		bptr = prev;
	}
	else
	{
		if (bptr->size_status % 8 != 2)
		{
			bptr->size_status += 2;
		}
	}
	int original = bptr->size_status - (bptr->size_status % 8);
	blockHeader *ftr = (blockHeader *)((char *)bptr + original - 4);
	ftr->size_status = original;
	return 0;
}

int init_heap(int sizeOfRegion)
{
	static int allocated_once = 0;
	int pagesize;
	int padsize;
	void *mmap_ptr;
	int fd;
	blockHeader *end_mark;
	if (0 != allocated_once)
	{
		fprintf(stderr, "Error:mem.c: InitHeap has allocated space during a previous call\n");
		return -1;
	}
	if (sizeOfRegion <= 0)
	{
		fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
		return -1;
	}
	pagesize = getpagesize();
	padsize = sizeOfRegion % pagesize;
	padsize = (pagesize - padsize) % pagesize;
	alloc_size = sizeOfRegion + padsize;
	fd = open("/dev/zero", O_RDWR);
	if (-1 == fd)
	{
		fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
		return -1;
	}
	mmap_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (MAP_FAILED == mmap_ptr)
	{
		fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
		allocated_once = 0;
		return -1;
	}
	allocated_once = 1;
	alloc_size -= 8;
	heap_start = (blockHeader *)mmap_ptr + 1;
	end_mark = (blockHeader *)((void *)heap_start + alloc_size);
	end_mark->size_status = 1;
	heap_start->size_status = alloc_size;
	heap_start->size_status += 2;
	blockHeader *footer = (blockHeader *)((void *)heap_start + alloc_size - 4);
	footer->size_status = alloc_size;
	return 0;
}

void disp_heap()
{
	int counter;
	char status[6];
	char p_status[6];
	char *t_begin = NULL;
	char *t_end = NULL;
	int t_size;
	blockHeader *current = heap_start;
	counter = 1;
	int used_size = 0;
	int free_size = 0;
	int is_used = -1;
	fprintf(stdout, "*********************************** HEAP: Block List ****************************\n");
	fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
	fprintf(stdout, "---------------------------------------------------------------------------------\n");
	while (current->size_status != 1)
	{
		t_begin = (char *)current;
		t_size = current->size_status;
		if (t_size & 1)
		{
			strcpy(status, "alloc");
			is_used = 1;
			t_size = t_size - 1;
		}
		else
		{
			strcpy(status, "FREE ");
			is_used = 0;
		}
		if (t_size & 2)
		{
			strcpy(p_status, "alloc");
			t_size = t_size - 2;
		}
		else
		{
			strcpy(p_status, "FREE ");
		}
		if (is_used)
			used_size += t_size;
		else
			free_size += t_size;
		t_end = t_begin + t_size - 1;
		fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%4i\n", counter, status, p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
		current = (blockHeader *)((char *)current + t_size);
		counter = counter + 1;
	}
	fprintf(stdout, "---------------------------------------------------------------------------------\n");
	fprintf(stdout, "*********************************************************************************\n");
	fprintf(stdout, "Total used size = %4d\n", used_size);
	fprintf(stdout, "Total free size = %4d\n", free_size);
	fprintf(stdout, "Total size      = %4d\n", used_size + free_size);
	fprintf(stdout, "*********************************************************************************\n");
	fflush(stdout);
	return;
}
