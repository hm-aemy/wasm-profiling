#include "benchmarks.h"
#include "benchmarks-defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define expander(B, m) m(B)
#define _TEST_knucleotide 1
#define _TEST_reverse_complement 2
#define _test(benchname) _TEST_##benchname
#define _TEST_result expander(BENCHMARK, _test)


const char* wasm_interp(const unsigned char *input, size_t len);

#ifdef EMBENCH
#define FUN_NAME_GEN(b) run_##b
#define FUN_NAME expander(BENCHMARK, FUN_NAME_GEN)
__attribute__((weak)) bench_result FUN_NAME()
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    clock_t start = clock();
    const char* err = wasm_interp(BENCH, SIZE);
    clock_t end = clock();
    printf("First runtime delay: %ldms\n", (end - start) * 1000 / (CLOCKS_PER_SEC));
    printf("Second runtime delay: %ldms\n", (end - start) * 1000 / (CLOCKS_PER_SEC));
    if (!err)
    {
        printf("BENCHMARK"
               " result: %ld\n",
               err);
    }
    return err;
}
#endif

const char* run_active_bench(void*) {
    return FUN_NAME();
}
#undef FUN_NAME
#undef expander