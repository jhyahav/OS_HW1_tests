
#define _GNU_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <sys/mman.h>
#include "os.h"

/* 
	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~TEST HEADER FILES~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#include <time.h>
#include <execinfo.h>


void test_suite_1(void);
void test_suite_2(void);

/* 2^20 pages ought to be enough for anybody */
#define NPAGES	(1024*1024)

// For test suite 1
#define VPN_MASK 0x1FFFFFFFFFFF
#define PPN_MASK 0xFFFFFFFFFFFFF


static char* pages[NPAGES];

uint64_t alloc_page_frame(void)
{
	static uint64_t nalloc;
	uint64_t ppn;
	void* va;

	if (nalloc == NPAGES)
		errx(1, "out of physical memory");

	/* OS memory management isn't really this simple */
	ppn = nalloc;
	nalloc++;

	va = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (va == MAP_FAILED)
		err(1, "mmap failed");

	pages[ppn] = va;
	return ppn + 0xbaaaaaad;
}

void* phys_to_virt(uint64_t phys_addr)
{
	uint64_t ppn = (phys_addr >> 12) - 0xbaaaaaad;
	uint64_t off = phys_addr & 0xfff;
	char* va = NULL;

	if (ppn < NPAGES)
		va = pages[ppn] + off;

	return va;
}

int main(int argc, char **argv) 
{
	test_suite_1();
	test_suite_2();
	return 0;
}


/* 
	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~FUNCTIONS FOR SUITE 1~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

void assert_equal(uint64_t received, uint64_t expected) {
	static int counter = 0;

	if (expected != received) {
		printf("\n\033[0;31mFailed test!\nExpected \033[0m\033[0;32m%lx\033[0m\033[0;31m and received \033[0m\033[0;33m%lx\033[0m\033[0;31m.\033[0m\n", expected, received);

		void* callstack[128];
  		int i, frames = backtrace(callstack, 128);
  		char** strs = backtrace_symbols(callstack, frames);
		printf("\033[0;36m(Almost readable) stacktrace\n");
  		for (i = 0; i < frames; ++i) {
    			printf("%s\n", strs[i]);
  		}
		printf("\033[0m\n");
  		free(strs);

		assert(0);
	}

	if (counter % 500 == 0)
		printf("\033[0;32m.\033[0m");
	counter++;
}

uint64_t get_random(uint64_t mask) {
	return rand() & mask;
}


uint64_t power(uint64_t base, uint64_t exponent) {
	int temp;
    if (exponent == 0) {
    	return 1;
	}
    temp = power(base, exponent / 2);
    if (!(exponent % 2)) {
        return temp * temp;
	} else {
        return base * temp * temp;
	}
}

int in_array(uint64_t *arr, int size, uint64_t value) {
	for (int i = 0; i < size; i++)
		if (arr[i] == value)
			return 1;
	return 0;
}


void get_random_list(uint64_t **out, int size, uint64_t mask) {
	*out = calloc(size, sizeof(uint64_t));
	uint64_t *arr = *out;
	int count = 0;
	uint64_t val;

	while (count < size) {
		val = get_random(mask);
		
		if (!in_array(arr, count, val)) {
			arr[count] = val;
			count++;
		}
	}
}

uint64_t get_random_vpn() {
	return get_random(VPN_MASK);
}

uint64_t get_random_ppn() {
	return get_random(VPN_MASK);
}

void update_random_and_check(uint64_t pt) {
	uint64_t vpn = get_random_vpn();
	uint64_t ppn = get_random_ppn();
	
	if (rand() % 10 < 3)
		ppn = NO_MAPPING;

	page_table_update(pt, vpn, ppn);
	assert_equal(page_table_query(pt, vpn), ppn);
}

void update_many_with_prefix(uint64_t pt) {
	int prefix = (rand() % 45) + 1;
	uint64_t mask = power(2, prefix + 1) - 1;
	uint64_t vpn_mask = power(2, (45 - prefix) + 1) - 1;
	int amount = (rand() % 20) + 2;

	if (amount > vpn_mask / 2)
		amount = vpn_mask / 2;

	uint64_t block = get_random(mask) << prefix;
	uint64_t* vpn_arr;
	uint64_t* ppn_arr = malloc(sizeof(uint64_t) * amount);

	get_random_list(&vpn_arr, amount, vpn_mask);
	for (int i = 0; i < amount; i++) {
		vpn_arr[i] = block + vpn_arr[i];
		ppn_arr[i] = get_random_ppn();

		page_table_update(pt, vpn_arr[i], ppn_arr[i]);	
		assert_equal(page_table_query(pt, vpn_arr[i]), ppn_arr[i]);
	}

	for (int i = 0; i < amount; i++) {
		uint64_t value = page_table_query(pt, vpn_arr[i]);
		uint64_t expected = ppn_arr[i];
		if (value != expected) {
			printf("Set values:\n");
			for (int j = 0; j < amount; j++)
				printf("page_table[%lx] = %lx\n", vpn_arr[j], ppn_arr[j]);
			printf("\nFailed on index %d,\ngot value %lx instead of %lx\n", i, value, expected);
			assert(0);
		}
	}

	free(vpn_arr);
	free(ppn_arr);
}

void perform_random_move(uint64_t pt) {
	int option = rand() % 2;

	switch (option) {
		case 0:
			update_random_and_check(pt);
			break;
		case 1:
			update_many_with_prefix(pt);
			break;
	}

}

void test_suite_1(void) {
	srand(time(NULL));
	uint64_t pt = alloc_page_frame();

	assert_equal(page_table_query(pt, 0xcafe), NO_MAPPING);
	page_table_update(pt, 0xcafe, 0xf00d);
	assert_equal(page_table_query(pt, 0xcafe), 0xf00d);
	page_table_update(pt, 0xcafe, NO_MAPPING);
	assert_equal(page_table_query(pt, 0xcafe), NO_MAPPING);

	for (int i = 0; i < power(2, 15); i++) {
		perform_random_move(pt);
	}
	printf("\n\nPASSED SUITE 1\n\n");

}

void test_suite_2(void) {
		uint64_t pt = alloc_page_frame();
	/*0th Test (as included in original file)*/
	assert(page_table_query(pt, 0xcafecafeeee) == NO_MAPPING);
	assert(page_table_query(pt, 0xfffecafeeee) == NO_MAPPING);
	assert(page_table_query(pt, 0xcafecafeeff) == NO_MAPPING);
	page_table_update(pt, 0xcafecafeeee, 0xf00d);
	assert(page_table_query(pt, 0xcafecafeeee) == 0xf00d);
	assert(page_table_query(pt, 0xfffecafeeee) == NO_MAPPING);
	assert(page_table_query(pt, 0xcafecafeeff) == NO_MAPPING);
	page_table_update(pt, 0xcafecafeeee, NO_MAPPING);
	assert(page_table_query(pt, 0xcafecafeeee) == NO_MAPPING);
	assert(page_table_query(pt, 0xfffecafeeee) == NO_MAPPING);
	assert(page_table_query(pt, 0xcafecafeeff) == NO_MAPPING);
	printf("0th Test: PASSED\n");

	uint64_t new_pt = alloc_page_frame();
	uint64_t *tmp;

	/* 1st Test */
	assert(page_table_query(pt, 0xcafe) == NO_MAPPING);
	page_table_update(pt, 0xcafe, 0xf00d);
	page_table_update(pt, 0xbaff, 0xbadd);
	assert(page_table_query(pt, 0xcafe) == 0xf00d);
	assert(page_table_query(pt, 0xbaff) == 0xbadd);
	page_table_update(pt, 0xcafe, NO_MAPPING);
	assert(page_table_query(pt, 0xcafe) == NO_MAPPING);
	assert(page_table_query(pt, 0xbaff) == 0xbadd);
	page_table_update(pt, 0xbaff, NO_MAPPING);
	assert(page_table_query(pt, 0xbaff) == NO_MAPPING);
	printf("1st Test: PASSED\n");
	
	/* 2nd Test*/
	page_table_update(pt, 0xcafe, 0);
	assert(page_table_query(pt, 0xcafe) == 0);
	page_table_update(pt, 0xcafe, NO_MAPPING);
	assert(page_table_query(pt, 0xcafe) == NO_MAPPING);
	printf("2nd Test: PASSED\n");
	
	/* 3rd Test */
	page_table_update(pt, 0x8686, 0xabcd);
	assert(page_table_query(pt, 0x8686) == 0xabcd);
	page_table_update(pt, 0x8686, 0x1234);
	assert(page_table_query(pt, 0x8686) == 0x1234);
	printf("3rd Test: PASSED\n");
	
	/* 4th Test */
	page_table_update(pt, 0xcafe, 0xacdc);
	assert(page_table_query(pt, 0xcafe) == 0xacdc);
	page_table_update(pt, 0xcaff, 0xaaaa);
	assert(page_table_query(pt, 0xcafe) == 0xacdc);
	page_table_update(pt, 0xcafe, NO_MAPPING);
	page_table_update(pt, 0xcaff, NO_MAPPING);
	assert(page_table_query(pt, 0xcafe) == NO_MAPPING);
	assert(page_table_query(pt, 0xcaff) == NO_MAPPING);
	printf("4th Test: PASSED\n");
	
	/* 5th Test */
	page_table_update(new_pt, 0xcafe, 0xabcd);
	assert(page_table_query(new_pt, 0xcafe) == 0xabcd);
	page_table_update(new_pt, 0xcafe, NO_MAPPING);
	assert(page_table_query(new_pt, 0xcafe) == NO_MAPPING);
	printf("5th Test: PASSED\n");
	
	/* 6th Test */
	page_table_update(pt, 0x1ffff8000000, 0x1212);
	assert(page_table_query(pt, 0x1ffff8000000) == 0x1212);
	tmp = phys_to_virt(pt << 12);
	tmp = phys_to_virt((tmp[511] >> 12) << 12);
	tmp = phys_to_virt((tmp[511] >> 12) << 12);
	tmp = phys_to_virt((tmp[0] >> 12) << 12);
	tmp = phys_to_virt((tmp[0] >> 12) << 12);
	tmp[0] = ((tmp[0] >> 1) << 1);
	assert(page_table_query(pt, 0x1ffff8000000) == NO_MAPPING);
	printf("6th Test: PASSED\n\n----------------\n");
	
	printf("Overall:  PASSED SUITE 2\n\n");
}
