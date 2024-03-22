#ifndef SFMM_HELPER_H
#define SFMM_HELPER_H

#include "sfmm.h"
#include <errno.h>

#define WSIZE ((size_t)2)
#define  MEM_ROW ((size_t)8)
#define MIN_BLOCK_SIZE ((size_t)32)
#define ALIGN_SIZE ((size_t)16)
#define MUSK ((size_t)~0x7)

#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *) (p) = (val))
#define PACK(size,prv_alloc,alloc,in_qklst) (((size)|(prv_alloc)|(alloc)|(in_qklst))^(MAGIC))
#define GET_SIZE(header) (((header)^(MAGIC))&(MUSK))
#define GET_PREV_BLOCK_ALLOCATED(header) (((header)^(MAGIC))&(PREV_BLOCK_ALLOCATED))
#define GET_THIS_BLOCK_ALLOCATED(header) (((header)^(MAGIC))&(THIS_BLOCK_ALLOCATED))
#define GET_IN_QUICK_LIST(header) (((header)^(MAGIC))&(PREV_BLOCK_ALLOCATED))


int initialize_heap();
void init_free_list_heads();
int get_free_list_index(size_t size);
void insert_into_free_list_heads(sf_block* sentinel, sf_block* newBlock);
void insert_into_quick_list(sf_block* free_block,size_t size);
void init_qklst();
size_t size_to_allocate(size_t size);
sf_block*  search_qklst(size_t size);
sf_block* search_free_list_heads(size_t size);
sf_block* allocate_free_block(size_t size, sf_block* block_to_allocate);
sf_block* split_to_allocate(size_t size, sf_block* free_block);
void is_valid_pointer(void *pp);


#endif
