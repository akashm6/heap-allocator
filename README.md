# Custom Heap Allocator 

## How it works
* init_heap() sets up the heap by allocating a memory block of a specified size using mmap. This block is divided into headers and footers, marking free and used regions for memory management.
* balloc() uses a Best-Fit malloc strategy to allocate memory blocks, by checking the heap for the smallest free block that fits the request, and coalesces the block if necessary.
* bfree() frees an allocated block and coalesces adjacent free blocks into a larger free block to reduce fragmentation and improve memory utilization/throughput.
* immediate coalescing is implemented so that adjacent free blocks are immediately merged into larger singular free blocks. 
