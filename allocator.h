#ifndef __p3Heap_h
#define __p3Heap_h

#include <stddef.h>

int init_heap(int sizeOfRegion);
void disp_heap();

void *balloc(int size);
int bfree(void *ptr);

void *malloc(size_t size)
{
    return NULL;
}

#endif