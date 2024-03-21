/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include "sfmm_helper.h"

void *sf_malloc(size_t size) {
    // To be implemented.
    //for size zero, no need for allocation. So send a null pointer
    if(size==0){
        return NULL;
    }
    //first call to malloc--> heap is empty. Initailie with prologue, epilogue and first free block
    if(sf_mem_start()==sf_mem_end()){
        initialize_heap();
    }
    //calculate the size to allocate
    size_t allocate = size_to_allocate(size);
    //Search for block to allocate 
    sf_block* block_to_allocate = NULL;
    //first, search in quicklist

    if(allocate<=176){
        block_to_allocate = search_qklst(allocate);
    }
    //If block isn't found in quicklist, search in main_free_list
    if(block_to_allocate==NULL){
        block_to_allocate = search_free_list_heads(allocate);
    }
    /*
    If block_to_allocate is still null, means the heap doesn't have enough memory to allocate. 
    So we have add a page to the heap
    */
    /*if(block_to_allocate==NULL){
        //add a page
    }*/
    //Allocate the block found 
    return allocate_free_block(allocate,block_to_allocate);
    //abort();
}

void sf_free(void *pp) {
    // To be implemented.
    abort();
}

void *sf_realloc(void *pp, size_t rsize) {
    // To be implemented.
    abort();
}

double sf_fragmentation() {
    // To be implemented.
    abort();
}

double sf_utilization() {
    // To be implemented.
    abort();
}
