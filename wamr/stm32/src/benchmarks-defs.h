#ifndef BENCHMARKS_DEFS_H
#define BENCHMARKS_DEFS_H

typedef int bench_result;
typedef void *bench_args;
typedef bench_result (*benchmark_runner)(bench_args);

bench_result run_active_bench(bench_args);

#endif