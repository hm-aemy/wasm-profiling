#include <init.h>

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "benchmarks-defs.h"


extern uintptr_t _stack;
extern uintptr_t __bss_end__;
static void mark_stack()
{
	uintptr_t *sp;
	asm volatile("mov %0, sp"
				 : "=r"(sp));
	for (uintptr_t *ptr = sp - 0x100; ptr > &__bss_end__; ptr--)
	{
		*ptr = 0xDEADBEEF;
	}
}
static size_t count_stack()
{
	size_t result = 0;
	int consec_markers = 0;
	for (uintptr_t *ptr = &_stack - 1; ptr > &__bss_end__ && consec_markers < 8; ptr--)
	{
		if (*ptr == 0xDEADBEEF)
		{
			consec_markers += 1;
		}
		else
		{
			consec_markers = 0;
		}
		result += sizeof(uintptr_t);
	}
	return result - 8 * sizeof(uintptr_t);
}

int post_main()
{
	for (volatile uint8_t i = 1; i; i++)
		;
	trace_buffer("trace ready!\n", 13);
	const char* err = run_active_bench(NULL);
	if (!err)
	{
		size_t stack_usage = count_stack();
		printf("Max stack use: %d\n", stack_usage);
	} else {
		printf("Error from wasmi: %s\n", err);
	}
	printf("END OF TEST\n");
	trace_buffer("TRACE_DONE\n", 11);
	return err;
}

int main()
{
	init();
	mark_stack();
	return post_main();
}

#ifdef HEAP_TRACE
enum __attribute__((__packed__)) AllocType
{
	Malloc,
	Realloc,
	Free
};
typedef struct __attribute__((__packed__)) TraceData
{
	enum AllocType tag;
	union
	{
		struct
		{
			uintptr_t old;
			uintptr_t new;
			size_t size;
		} realloc;
		struct
		{
			uintptr_t new;
			size_t size;
		} malloc;
		struct
		{
			uintptr_t old;
		} free;

	} as;
} TraceData;

void *__real_malloc(size_t);
void *__wrap_malloc(size_t __size)
{
	void *ptr = __real_malloc(__size);
	TraceData trace = {
		.tag = Malloc,
		.as = {
			.malloc = {
				.new = ptr,
				.size = __size,
			}}};
	trace_buffer((uint8_t *)&trace, sizeof(trace));
	return ptr;
}

void *__real_realloc(void *, size_t);
void *__wrap_realloc(void *ptr, size_t n)
{
	void *ptr_new = __real_realloc(ptr, n);
	TraceData trace = {
		.tag = Realloc,
		.as = {
			.realloc = {
				.new = ptr_new,
				.old = ptr,
				.size = n,
			}}};
	trace_buffer((uint8_t *)&trace, sizeof(trace));
	return ptr_new;
}

void __real_free(void *);
void __wrap_free(void *ptr)
{
	TraceData trace = {
		.tag = Free,
		.as = {
			.free = {
				.old = ptr,
			}}};
	trace_buffer((uint8_t *)&trace, sizeof(trace));
	return __real_free(ptr);
}

#endif