#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"
#define TEST_TIMEOUT 15

/*
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 */
void assert_free_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_FREE_LISTS; i++) {
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	while(bp != &sf_free_list_heads[i]) {
	    if(size == 0 || size == ((bp->header ^ MAGIC) & ~0xf))
		cnt++;
	    bp = bp->body.links.next;
	}
    }
    if(size == 0) {
	cr_assert_eq(cnt, count, "Wrong number of free blocks (exp=%d, found=%d)",
		     count, cnt);
    } else {
	cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    }
}

/*
 * Assert that the free list with a specified index has the specified number of
 * blocks in it.
 */
void assert_free_list_size(int index, int size) {
    int cnt = 0;
    sf_block *bp = sf_free_list_heads[index].body.links.next;
    while(bp != &sf_free_list_heads[index]) {
	cnt++;
	bp = bp->body.links.next;
    }
    cr_assert_eq(cnt, size, "Free list %d has wrong number of free blocks (exp=%d, found=%d)",
		 index, size, cnt);
}

/*
 * Assert the total number of quick list blocks of a specified size.
 * If size == 0, then assert the total number of all quick list blocks.
 */
/*
 * Assert the total number of quick list blocks of a specified size.
 * If size == 0, then assert the total number of all quick list blocks.
 */
void assert_quick_list_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_QUICK_LISTS; i++) {
	sf_block *bp = sf_quick_lists[i].first;
	while(bp != NULL) {
	    if(size == 0 || size == ((bp->header ^ MAGIC) & ~0xf))
		cnt++;
	    bp = bp->body.links.next;
	}
    }
    if(size == 0) {
	cr_assert_eq(cnt, count, "Wrong number of quick list blocks (exp=%d, found=%d)",
		     count, cnt);
    } else {
	cr_assert_eq(cnt, count, "Wrong number of quick list blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    }
}

Test(sfmm_basecode_suite, malloc_an_int, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz = sizeof(int);
	int *x = sf_malloc(sz);

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(8112, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + 8192 == sf_mem_end(), "Allocated more than necessary!");
}

Test(sfmm_basecode_suite, malloc_four_pages, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;

	// We want to allocate up to exactly four pages, so there has to be space
	// for the header and the link pointers.
	void *x = sf_malloc(32704);
	cr_assert_not_null(x, "x is NULL!");
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sfmm_basecode_suite, malloc_too_large, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(163840);

	cr_assert_null(x, "x is not NULL!");
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(163792, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sfmm_basecode_suite, free_quick, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_x = 8, sz_y = 32, sz_z = 1;
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);

	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(48, 1);
	assert_free_block_count(0, 1);
	assert_free_block_count(8032, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, free_no_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_x = 8, sz_y = 200, sz_z = 1;
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 2);
	assert_free_block_count(208, 1);
	assert_free_block_count(7872, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, free_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_w = 8, sz_x = 200, sz_y = 300, sz_z = 4;
	/* void *w = */ sf_malloc(sz_w);
	void *x = sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);
	sf_free(x);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 2);
	assert_free_block_count(528, 1);
	assert_free_block_count(7552, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, freelist, .timeout = TEST_TIMEOUT) {
        size_t sz_u = 200, sz_v = 300, sz_w = 200, sz_x = 500, sz_y = 200, sz_z = 700;
	void *u = sf_malloc(sz_u);
	/* void *v = */ sf_malloc(sz_v);
	void *w = sf_malloc(sz_w);
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(u);
	sf_free(w);
	sf_free(y);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 4);
	assert_free_block_count(208, 3);
	assert_free_block_count(5968, 1);

	// First block in list should be the most recently freed block.
	int i = 3;
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	cr_assert_eq(bp, (char *)y - 16,
		     "Wrong first block in free list %d: (found=%p, exp=%p)",
                     i, bp, (char *)y - 16);
}

Test(sfmm_basecode_suite, realloc_larger_block, .timeout = TEST_TIMEOUT) {
    size_t sz_x = sizeof(int), sz_y = 10, sz_x1 = sizeof(int) * 20;
	void *x = sf_malloc(sz_x);
	/* void *y = */ sf_malloc(sz_y);
	x = sf_realloc(x, sz_x1);

	cr_assert_not_null(x, "x is NULL!");
	sf_block *bp = (sf_block *)((char *)x - 16);
	cr_assert((bp->header ^ MAGIC) & 0x2, "Allocated bit is not set!");
	cr_assert(((bp->header ^ MAGIC) & ~0xf) == 96,
		  "Realloc'ed block size (0x%ld) not what was expected (0x%ld)!",
		  (bp->header ^ MAGIC) & ~0xf, 96);

	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(32, 1);
	assert_free_block_count(0, 1);
	assert_free_block_count(7984, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_splinter, .timeout = TEST_TIMEOUT) {
        size_t sz_x = sizeof(int) * 20, sz_y = sizeof(int) * 16;
	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_block *bp = (sf_block *)((char *)y - 16);
	cr_assert((bp->header ^ MAGIC) & 0x2, "Allocated bit is not set!");
	cr_assert(((bp->header ^ MAGIC) & ~0xf) == 96,
		  "Block size (0x%ld) not what was expected (0x%ld)!",
		  (bp->header ^ MAGIC) & ~0xf, 96);

	// There should be only one free block.
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(8048, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_free_block, .timeout = TEST_TIMEOUT) {
    size_t sz_x = sizeof(double) * 8, sz_y = sizeof(int);
	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(y, "y is NULL!");

	sf_block *bp = (sf_block *)((char *)y - 16);
	cr_assert((bp->header ^ MAGIC) & 0x2, "Allocated bit is not set!");
	cr_assert(((bp->header ^ MAGIC) & ~0xf) == 32,
		  "Realloc'ed block size (0x%ld) not what was expected (0x%ld)!",
		  (bp->header ^ MAGIC) & ~0xf, 32);

	// After realloc'ing x, we can return a block of size ADJUSTED_BLOCK_SIZE(sz_x) - ADJUSTED_BLOCK_SIZE(sz_y)
	// to the freelist.  This block will go into the main freelist and be coalesced.
	// Note that we don't put split blocks into the quick lists because their sizes are not sizes
	// that were requested by the client, so they are not very likely to satisfy a new request.
	assert_quick_list_block_count(0, 0);	
	assert_free_block_count(0, 1);
	assert_free_block_count(8112, 1);
}

//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE OR OTHERWISE MANGLE THESE COMMENTS
//############################################

//Test(sfmm_student_suite, student_test_1, .timeout = TEST_TIMEOUT) {
//}

Test(sfmm_student_suite, flush_quicklist, .timeout=TEST_TIMEOUT){
	sf_errno = 0;
	size_t sz_u = 10, sz_v = 3, sz_w = 5, sz_x = 8, sz_y = 20, sz_z = 1;

	void *u = sf_malloc(sz_u);
	void *v = sf_malloc(sz_v);
	void *w = sf_malloc(sz_w);
	void *x = sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	void *z = sf_malloc(sz_z);

	sf_free(u);
	sf_free(v);
	sf_free(w);
	sf_free(x);
	sf_free(y);
	sf_free(z);

	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(32, 1);
	assert_free_block_count(0, 2);
	assert_free_block_count(160, 1);
	assert_free_block_count(7952, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_student_suite, coalesce_prev_and_next, .timeout=TEST_TIMEOUT){
	sf_errno = 0;
	size_t sz_x = 2000, sz_y = 1000, sz_z = 500;
	void *x = sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
    void* z = sf_malloc(sz_z);
    
    sf_free(x);
    sf_free(z);
    sf_free(y);

	assert_quick_list_block_count(0,0);
	assert_free_block_count(0,1);
	assert_free_block_count(8144,1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_student_suite, malloc_edge_case, .timeout=TEST_TIMEOUT){
	sf_errno = 0; 
	size_t sz_x = 0;
	cr_assert(sf_malloc(sz_x)==NULL,"malloc should return NULL on size 0");
}

Test(sfmm_student_suite, free_invalid_pointer, .signal=SIGABRT){
	size_t sz_x = 2000;
	void *x = sf_malloc(sz_x);
	void *y = ((void*)x-900);
    sf_free(y);
}

Test(sfmm_student_suite, realloc_invalid_pointer, .timeout=TEST_TIMEOUT){
	size_t sz_x = 2000;
	void *x = sf_malloc(sz_x);
	void *y = ((void*)x-900);
	void *z = sf_realloc(y,100);
    
	cr_assert_not_null(z==NULL,"z is not NULL!");
	cr_assert(sf_errno == EINVAL, "sf_errno not set to EINVAL");
}

