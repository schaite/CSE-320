#include "sfmm_helper.h"

void initialize_heap(){
    if(sf_mem_grow()==NULL){//grow the heap by one page 
        sf_errno = ENOMEM;
        return;
    }

    char* heap_start = sf_mem_start()+MEM_ROW;  //heap alignment by adding 8byte at the beginning 
    char* heap_end = sf_mem_end()-MEM_ROW;   //heap ends before epilogue

    //initialize prologue block
    sf_block* prologue = (sf_block*) heap_start;
    prologue->header = PACK(MIN_BLOCK_SIZE,PREV_BLOCK_ALLOCATED,THIS_BLOCK_ALLOCATED,0);

    //initialize the epilogue block 
    sf_block* epilogue = (sf_block*) heap_end;
    epilogue->header = PACK(0,0,THIS_BLOCK_ALLOCATED,0);

    //initialize the free_list_heads array with sentinels
    init_free_list_heads();
    //initialize the first block
    sf_block* first_free_block = (sf_block*) (heap_start+MIN_BLOCK_SIZE);
    size_t first_free_block_size = PAGE_SZ-MEM_ROW-MIN_BLOCK_SIZE-MEM_ROW; //page_size - alignment_row - prologue_size - epilogue_size;
    first_free_block->header = PACK(first_free_block_size,PREV_BLOCK_ALLOCATED,0,0);
    sf_footer* first_block_footer = (sf_footer*)((void*)first_free_block+first_free_block_size-sizeof(sf_footer));
    *first_block_footer = first_free_block->header;//footer of the first block 
    insert_into_free_list_heads(&sf_free_list_heads[get_free_list_index(first_free_block_size)],first_free_block);
    //initialize quicklist
    init_qklst();
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

size_t size_to_allocate(size_t size){
    if(size<=MIN_BLOCK_SIZE){
        return MIN_BLOCK_SIZE;
    }
    else{
        if(size%ALIGN_SIZE==0){
            return size;
        }
        else{
            return size+(size%ALIGN_SIZE);
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
    block->header &= (~IN_QUICK_LIST);//clear in_qklst bit 
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
        block_to_allocate->header |= THIS_BLOCK_ALLOCATED;
        sf_block* next_block = (sf_block*)(((void*)block_to_allocate)+GET_SIZE(block_to_allocate->header));
        next_block->header |= PREV_BLOCK_ALLOCATED;
        return block_to_allocate;
    }
    else{//split the block, insert upper part of the splitted block in the main free list, then allocate the lower part
        return split_to_allocate(size,block_to_allocate);
    }
    return NULL;
}

sf_block* split_to_allocate(size_t size, sf_block* free_block){
    //update the following block of free_block with PREV_BLOCK_ALLOCATED
    sf_block* next_block = (sf_block*)(((void*)free_block)+GET_SIZE(free_block->header));
    next_block->header |= PREV_BLOCK_ALLOCATED;

    //configer the higher block as new free block: 

    //1. set the remaining size as new_block_size
    size_t new_free_block_size = GET_SIZE(free_block->header)-size;
    //2. update header for the new free block
    free_block->header = PACK(new_free_block_size,GET_PREV_BLOCK_ALLOCATED(free_block->header),0,0);
    //3. create new footer for the new free block 
    sf_footer* new_free_block_footer = (sf_footer*)((void*)free_block+new_free_block_size-sizeof(sf_footer));
    *new_free_block_footer = free_block->header;

    //create the new block to be allocated:

    //1. set allocated block pointer where free block ends
    sf_block* allocated_block = (sf_block*)((void*)free_block+new_free_block_size);
    //2. set header size, and bits
    allocated_block->header = PACK(size,0,1,0);

    //insert the new_free_block in free_list_heads
    insert_into_free_list_heads(&sf_free_list_heads[get_free_list_index(new_free_block_size)],free_block);

    return allocated_block;
}