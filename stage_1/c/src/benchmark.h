#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stddef.h>

/* Same benchmarks, sizes, query workload and metric names as the Python
   version. Results go to benchmarks/results/c_<benchmark>.csv with the
   common columns: language,benchmark,structure,n_books,metric,value
   which = "datalake", "index", "metadata" or "all". n_sizes = 0 uses the defaults. */
int run_benchmark(const char *which, const int *sizes, size_t n_sizes);

#endif