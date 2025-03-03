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
    //for size zero, no need for allocation. So send a null pointer
    if(size==0){
        return NULL;
    }
    //first call to malloc--> heap is empty. Initailie with prologue, epilogue and first free block
    if(sf_mem_start()==sf_mem_end()){
        if(initialize_heap()==-1){
            return NULL;
        }
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
    if(block_to_allocate==NULL){
        //add memory of pages to allocate
        int num_pages = 0;
        size_t prev_alloc = GET_PREV_BLOCK_ALLOCATED(epilogue->header);
        if(allocate%PAGE_SZ==0){
            num_pages = allocate/PAGE_SZ;
        }
        else{
            num_pages = (int)(allocate/PAGE_SZ)+1;
        }
        if(!prev_alloc){
            size_t curr_free = GET_SIZE(epilogue->prev_footer);
            if(curr_free==8144){
                num_pages-=1;
            }
        }
        //sf_show_heap();
        int i;
        for(i = 1; i<=num_pages;i++){
            if(sf_mem_grow()==NULL){//no memory left to extend heap
                break;
            }
        }
        if(--i!=num_pages){
            num_pages=i;
        }
        block_to_allocate = (sf_block*)((void*)epilogue);
        block_to_allocate->header = PACK(PAGE_SZ*num_pages,prev_alloc,0,0);
        sf_footer* block_footer = (sf_footer*)((void*)block_to_allocate+PAGE_SZ*num_pages);
        *block_footer = block_to_allocate->header;
        epilogue = (sf_block*)((void*)sf_mem_end()-16);
        epilogue->header = PACK(0,0,THIS_BLOCK_ALLOCATED,0);
        insert_into_free_list_heads(&sf_free_list_heads[get_free_list_index(PAGE_SZ)],block_to_allocate);
        block_to_allocate = coalesce(block_to_allocate);
        size_t s = GET_SIZE(block_to_allocate->header);
        if(s<allocate){
            sf_errno = ENOMEM;
            return NULL;
        }
    }
    current_aggregated_payload += (allocate-sizeof(sf_header));
    if(current_aggregated_payload>max_aggregated_payload){
        max_aggregated_payload = current_aggregated_payload;
    }
    //Allocate the block found 
    return &allocate_free_block(allocate,block_to_allocate)->body.payload;
}

void sf_free(void *pp) {
    //check if pointer is valid, if not, program is aborted
    if(!is_valid_pointer(pp)){
        abort();
    }
    //check block size and decide is free list is to be added in quicklist or free_list_heads
    sf_block* free_block = (sf_block*)((char*)pp-sizeof(sf_header)-sizeof(sf_footer));
    size_t size = GET_SIZE(free_block->header);
    if(size<=172){//add it to quicklist
        insert_into_quick_list(free_block,size);
    }
    else{//add it to free_list_heads
        //to free a block, update the header of the current block
        //have the new header point to the footer with the header info
        //update the next block with prev_block_allocated set to 0
        free_block->header = PACK(size,GET_PREV_BLOCK_ALLOCATED(free_block->header),0,0);
        sf_footer* free_block_footer = (sf_footer*)((void*)free_block+size);
        *free_block_footer = free_block->header;
        sf_block* next_block = (sf_block*)((void*)free_block+size);
        next_block->header = (((next_block->header)^MAGIC)&(~PREV_BLOCK_ALLOCATED))^MAGIC;
        insert_into_free_list_heads(&sf_free_list_heads[get_free_list_index(size)],free_block);
        coalesce(free_block); 
    }
    current_aggregated_payload-=(size-sizeof(sf_header));
    //if quicklist consider 2 situtation: 1. list for that size has capacity, 2. doesn't have capacity. 
    //for, 1: simply insert it in quicklist 
    //for 2: need to flush that list which contains coalescing 
    //for large blocks, add it to free_list_heads and coalesce
}

void *sf_realloc(void *pp, size_t rsize) {
    if(!is_valid_pointer(pp)){
        sf_errno = EINVAL;
        return NULL;
    }
    if(rsize==0){
        sf_free(pp);
        return NULL;
    }
    sf_block* r_block = (sf_block*)((void*)pp-16);
    size_t block_size = GET_SIZE(r_block->header);
    size_t allocate = size_to_allocate(rsize);
    if(allocate<=block_size){//smaller block realloc
        size_t remainder = block_size-allocate;
        if(remainder<MIN_BLOCK_SIZE){//do not create splinters
            return pp;
        }
        else{
            r_block = split_to_allocate(allocate,r_block);
            return r_block->body.payload;
        }
    }
    else{//larger block realloc
        void* block = sf_malloc(rsize);
        if(block==NULL){
            return NULL;
        }
        memcpy(block,pp,rsize);
        sf_free(pp);
        return block;
    }

    //abort();
}

double sf_fragmentation() {
    // To be implemented.
    abort();
}

double sf_utilization() {
    if(sf_mem_start()==sf_mem_end()){
        return 0.0;
    }
    double current_heap_size = (double)(sf_mem_end()-sf_mem_start());
    double peak_utilization = (double)max_aggregated_payload/current_heap_size;
    return peak_utilization;
}
