#include <stdio.h>
#include "sfmm_helper.h"

int initialize_heap(){
    void *new_page_start = sf_mem_grow();
    if(new_page_start==NULL){//grow the heap by one page 
        sf_errno = ENOMEM;
        return -1;
    }

    //initialize prologue block
    sf_block* prologue = (sf_block*)((void*)sf_mem_start());
    prologue->header = PACK(MIN_BLOCK_SIZE,PREV_BLOCK_ALLOCATED,THIS_BLOCK_ALLOCATED,0);
    //initialize Epilogue block
    sf_block* epilogue = (sf_block*)((void*)sf_mem_start()+PAGE_SZ-16);
    epilogue->header = PACK(0,PREV_BLOCK_ALLOCATED,THIS_BLOCK_ALLOCATED,0);
    //initialize the free_list_heads array with sentinels
    init_free_list_heads();
    //initialize the first block
    sf_block* first_free_block = (sf_block*) (sf_mem_start()+MIN_BLOCK_SIZE);
    size_t first_free_block_size = PAGE_SZ-MEM_ROW-MIN_BLOCK_SIZE-MEM_ROW; //page_size - alignment_row - prologue_size - epilogue_size;
    first_free_block->header = PACK(first_free_block_size,PREV_BLOCK_ALLOCATED,0,0);
    sf_footer* first_block_footer = (sf_footer*)((void*)first_free_block+first_free_block_size);
    *first_block_footer = first_free_block->header;//footer of the first block 
    insert_into_free_list_heads(&sf_free_list_heads[get_free_list_index(first_free_block_size)],first_free_block);
    //initialize quicklist
    init_qklst();
    return 0;
}

void init_free_list_heads(){
    //create sentinel nodes for each index
    for(int i = 0; i<NUM_FREE_LISTS; i++){
        sf_free_list_heads[i].header = 0;
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
    }
}

void init_qklst(){
    for(int i = 0; i<NUM_QUICK_LISTS; i++){
        sf_quick_lists[i].length = 0;
        sf_quick_lists[i].first = NULL;
    }
}

int get_free_list_index(size_t size){
    if(size==MIN_BLOCK_SIZE){
        return 0;
    }
    else if(size > MIN_BLOCK_SIZE && size <= 2*MIN_BLOCK_SIZE){
        return 1;
    }
    else if(size > 2*MIN_BLOCK_SIZE && size <= 4*MIN_BLOCK_SIZE){
        return 2;
    }
    else if(size > 4*MIN_BLOCK_SIZE && size <= 8*MIN_BLOCK_SIZE){
        return 3;
    }
    else if(size > 8*MIN_BLOCK_SIZE && size <= 16*MIN_BLOCK_SIZE){
        return 4;
    }
    else if(size > 16*MIN_BLOCK_SIZE && size <= 32*MIN_BLOCK_SIZE){
        return 5;
    }
    else if(size > 32*MIN_BLOCK_SIZE && size <= 64*MIN_BLOCK_SIZE){
        return 6;
    }
    else if(size > 64*MIN_BLOCK_SIZE && size <= 128*MIN_BLOCK_SIZE){
        return 7;
    }
    else if(size > 128*MIN_BLOCK_SIZE && size <= 256*MIN_BLOCK_SIZE){
        return 8;
    }
    else if(size > 256*MIN_BLOCK_SIZE){
        return 9;
    }
    else{
        return -1;//size less than minimum block size 
    }

}

void insert_into_free_list_heads(sf_block* sentinel, sf_block* newBlock){
    newBlock->body.links.next = sentinel->body.links.next;
    newBlock->body.links.prev = sentinel;
    sentinel->body.links.next->body.links.prev = newBlock;
    sentinel->body.links.next = newBlock;
}

void insert_into_quick_list(sf_block* free_block, size_t size){
    int qklst_index = (size-MIN_BLOCK_SIZE)/ALIGN_SIZE;
    if(sf_quick_lists[qklst_index].length<5){//No overflow, enter the block in front of the list
        //Set alloc and in_qklist block to 1 
        free_block->header = PACK(size,GET_PREV_BLOCK_ALLOCATED(free_block->header),THIS_BLOCK_ALLOCATED,IN_QUICK_LIST);
        //printf("%lu\n",(free_block->header^MAGIC));
        //Insert free_block in the quicklist
        free_block->body.links.next = sf_quick_lists[qklst_index].first;
        sf_quick_lists[qklst_index].first = free_block;
        sf_quick_lists[qklst_index].length++;
    }
    //else{//This quicklist doesn't have anymore spcae
    //flush the current blocks of this list to free_list_heads[coalesce if needed]

    //insert the free_block in the empty list 

    //}
}

size_t size_to_allocate(size_t size){
    if((size+sizeof(sf_header)<=MIN_BLOCK_SIZE)){
        return MIN_BLOCK_SIZE;
    }
    else{
        if((size+sizeof(sf_header))%ALIGN_SIZE==0){
            return size+sizeof(sf_header);
        }
        else{
            return size+sizeof(sf_header)+(ALIGN_SIZE-(size+sizeof(sf_header))%ALIGN_SIZE);
        }
    }
}

sf_block* search_qklst(size_t size){
    if(sf_quick_lists[(size-MIN_BLOCK_SIZE)/ALIGN_SIZE].length==0){
        return NULL;
    }
    sf_block* block = sf_quick_lists[(size-MIN_BLOCK_SIZE)/ALIGN_SIZE].first;
    sf_quick_lists[(size-MIN_BLOCK_SIZE)/ALIGN_SIZE].first = block->body.links.next;
    sf_quick_lists[(size-MIN_BLOCK_SIZE)/ALIGN_SIZE].length--;
    //clear dangling pointers 
    block->body.links.next = NULL;
    block->header = ((block->header^MAGIC)&(~IN_QUICK_LIST))^MAGIC;//clear in_qklst bit 
    return block;
}

sf_block* search_free_list_heads(size_t size){
    int i = get_free_list_index(size); 
    for(;i<NUM_FREE_LISTS;i++){
        sf_block* current = sf_free_list_heads[i].body.links.next;
        while(current!=&sf_free_list_heads[i]){
            if(GET_SIZE(current->header)>=size){
                //remove the block from the free list 
                current->body.links.prev->body.links.next = current->body.links.next;
                current->body.links.next->body.links.prev = current->body.links.prev;
                //clear dangling pointers
                current->body.links.next=NULL;
                current->body.links.prev=NULL;
                return current;
            }
            current = current->body.links.next;
        }
    }
    return NULL;
}

sf_block* allocate_free_block(size_t size, sf_block* block_to_allocate){
    //split or not??
    if(GET_SIZE(block_to_allocate->header)<(size+MIN_BLOCK_SIZE)){//no need to split
        //update allocated block header
        block_to_allocate->header = (((block_to_allocate->header)^MAGIC)|THIS_BLOCK_ALLOCATED)^MAGIC;
        //remove allocated block from free list
        block_to_allocate->body.links.prev->body.links.next = block_to_allocate->body.links.next;
        block_to_allocate->body.links.next->body.links.prev = block_to_allocate->body.links.prev;
        //update then next block's header on prev block's allocation
        sf_block* next_block = (sf_block*)(((char*)block_to_allocate)+GET_SIZE(block_to_allocate->header));
        next_block->header = (((next_block->header)^MAGIC)|PREV_BLOCK_ALLOCATED)^MAGIC;
        return block_to_allocate;
    }
    else{//split the block, insert upper part of the splitted block in the main free list, then allocate the lower part
        return split_to_allocate(size,block_to_allocate);
    }
    return NULL;
}

sf_block* split_to_allocate(size_t size, sf_block* free_block){
    //1. set the remaining size as new_block_size
    size_t new_free_block_size = GET_SIZE(free_block->header)-size;
    //2. update header for the new free block
    free_block->header = PACK(new_free_block_size,GET_PREV_BLOCK_ALLOCATED(free_block->header),0,0);
    //3. create new footer for the new free block 
    sf_footer* new_free_block_footer = (sf_footer*)((char*)free_block+new_free_block_size);
    *new_free_block_footer = free_block->header;

    //create the new block to be allocated:

    //1. set allocated block pointer where free block ends
    sf_block* allocated_block = (sf_block*)((char*)free_block+new_free_block_size);
    //2. set header size, and bits
    allocated_block->header = PACK(size,0,THIS_BLOCK_ALLOCATED,0);

    //update the following block of free_block with PREV_BLOCK_ALLOCATED[did it in the beginning, as i'm gonna lose the pointer]
    sf_block* next_block = (sf_block*)(((char*)allocated_block)+size);
    next_block->header = ((next_block->header^MAGIC)|PREV_BLOCK_ALLOCATED)^MAGIC;

    //insert the new_free_block in free_list_heads
    insert_into_free_list_heads(&sf_free_list_heads[get_free_list_index(new_free_block_size)],free_block);

    return allocated_block;
}

void is_valid_pointer(void *pp){
    //Pointer is null or the memory address isn't multiple of 16 
    if(pp==NULL||(uintptr_t)pp%16!=0){
        abort();
    }
    //get pointer to the starting of the block
    sf_block *ptr = (sf_block*)((char*)pp-sizeof(sf_header)-sizeof(sf_footer));
    sf_header ptr_header = ptr->header^MAGIC;
    size_t size = GET_SIZE(ptr->header);

    //Size of the block is less than 32 or the size of the block isn't 16 byte aligned 
    if((size<32) || (size%16!=0)){
        abort();
    }
    //Header of the block is before the start of the first block of the heap 
    //or, Footer of the block is after the end of the last block of the heap
    void* first_block_start = sf_mem_start()+MIN_BLOCK_SIZE+MEM_ROW;
    void* last_block_end = sf_mem_end()-MEM_ROW;//start of epilogue
    if(((char*)ptr+MEM_ROW)<(char*)first_block_start||(char*)ptr+size>=(char*)last_block_end){
        abort();
    }
    //alloc bit is 0 or in_qklst bit is 1
    if(!(ptr_header&THIS_BLOCK_ALLOCATED) || (ptr_header&IN_QUICK_LIST)){
        abort();
    }
    //prev_alloc is 0, while alloc of prev_footer is 1
    if(!(ptr_header&PREV_BLOCK_ALLOCATED)&&((ptr->prev_footer^MAGIC)&THIS_BLOCK_ALLOCATED)){
        abort();
    }
}

void remove_from_free_list(sf_block* block){
    if(block->body.links.next==NULL||block->body.links.prev==NULL){
        return;
    }
    block->body.links.prev->body.links.next = block->body.links.next;
    block->body.links.next->body.links.prev=block->body.links.prev;
    block->body.links.next=NULL;
    block->body.links.prev=NULL;
}

void *coalesce(sf_block* block){
    size_t size = GET_SIZE(block->header);
    size_t prev_alloc = GET_PREV_BLOCK_ALLOCATED(block->header);
    sf_block* next_block = (sf_block*)((void*)block+size);
    size_t next_alloc = GET_THIS_BLOCK_ALLOCATED(next_block->header);
    if(prev_alloc&&next_alloc){
        return block;
    }
    else if(prev_alloc&&!next_alloc){
        sf_block* new_block = block;
        size+=GET_SIZE(next_block->header);
        sf_header new_header = PACK(size,PREV_BLOCK_ALLOCATED,0,0);
        remove_from_free_list(block);
        remove_from_free_list(next_block);
        new_block->header = new_header;
        sf_footer* new_footer = (sf_footer*)((void*)new_block+size);
        *new_footer = new_block->header;
        insert_into_free_list_heads(&sf_free_list_heads[get_free_list_index(size)],new_block);
        return new_block;
    }
    else if(!prev_alloc&&next_alloc){
        size_t prev_block_size = GET_SIZE(block->prev_footer);
        sf_block* prev_block = (sf_block*)((void*)block-prev_block_size);
        sf_block* new_block = prev_block;
        size+=prev_block_size;
        sf_header new_header = PACK(size,GET_PREV_BLOCK_ALLOCATED(prev_block->header),0,0);
        remove_from_free_list(prev_block);
        remove_from_free_list(block);
        new_block->header = new_header;
        sf_footer* new_footer = (sf_footer*)((void*)new_block+size);
        *new_footer = new_block->header;
        insert_into_free_list_heads(&sf_free_list_heads[get_free_list_index(size)],new_block);
        return new_block;
    }
    else{
        return NULL;
    }
    
}

// void *coalesce(void *bp){
//     sf_block* free_block = (sf_block*)((void*)bp);
//     size_t size = GET_SIZE(free_block->header);
//     size_t prev_alloc = GET_PREV_BLOCK_ALLOCATED(free_block->header);
//     sf_block* next_block = (sf_block*)((void*)free_block+size);
//     size_t next_alloc = GET_THIS_BLOCK_ALLOCATED(next_block->header);

//     if(!next_alloc){
//         size += GET_SIZE(next_block->header);
//         remove_from_free_list(next_block);
//         //sf_show_block(next_block);
//     }
//     if(!prev_alloc){
//         size_t prev_block_size = GET_SIZE(free_block->prev_footer);
//         sf_block* prev_block = (sf_block*)((void*)free_block-prev_block_size);
//         //sf_show_block(prev_block);
//         size += prev_block_size;
//         free_block = prev_block;
//         //sf_show_block(free_block);
//         remove_from_free_list(prev_block);
//         //sf_show_block(prev_block);
//     }
//     free_block->header = PACK(size,GET_PREV_BLOCK_ALLOCATED(free_block->header),0,0);
//     sf_footer* new_footer = (sf_footer*)((void*)free_block+size);
//     *new_footer = free_block->header;
//     insert_into_free_list_heads(&sf_free_list_heads[get_free_list_index(size)],free_block);
//     //sf_show_block(free_block);
//     remove_from_free_list((sf_block*)bp);
//     return free_block;
// }
// void flush_quick_list(sf_block* first_block){

// }

