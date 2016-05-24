#ifndef COMMON_H_
#define COMMON_H_

#ifndef TEST
  #define TEST 0
#endif

#ifndef DEBUG
  #define DEBUG 0
#endif

// Use WHEN_TEST to conditionally include expensive sanity-checks,
// when doing more work than simply checking an assertion. For example:
//
// uint64_t result = fast_computation_method();
// WHEN_TEST({
//   uint64_t expectedResult = slow_but_safe_computation_method();
//   assert(result == expectedResult);
// })
#if TEST
  #define WHEN_TEST(ex) ex
#else
  #define WHEN_TEST(ex)
#endif

// Always wrap your debugging code in WHEN_DEBUG, so it doesn't
// get included into the executable by accident:
// WHEN_DEBUG({
//   printf("Debugging...");
//   printf("Still debugging...");
// })
#if DEBUG
  #define WHEN_DEBUG(ex) ex
#else
  #define WHEN_DEBUG(ex)
#endif

#ifndef VERBOSE
  #define VERBOSE 0
#endif

#ifndef BASELINE
  #define BASELINE 0
#endif

#ifndef D0_BSP
  #define D0_BSP 0
#elif D0_BSP == 1
  #ifndef DISTANCE
    #define DISTANCE 0
  #endif
#endif

#ifndef D1_PRIO
  #define D1_PRIO 0
#endif

#ifndef D1_CHUNK
  #define D1_CHUNK 0
#endif

#ifndef D1_CHROM
  #define D1_CHROM 0
#endif

#ifndef D1_LOCKS
  #define D1_LOCKS 0
#endif

#ifndef D1_PHASE
  #define D1_PHASE 0
#elif D1_PHASE == 1
  #ifndef CHUNK_BITS
    #define CHUNK_BITS 16
  #endif
#endif

#ifndef D1_NUMA
  #define D1_NUMA 0
#elif D1_NUMA == 1
  #ifndef CHUNK_BITS
    #define CHUNK_BITS 16
  #endif
  #ifndef NUMA_WORKERS
    #ifndef PARALLEL
      #define NUMA_WORKERS 1
    #elif PARALLEL == 0
      #define NUMA_WORKERS 1
    #elif PARALLEL == 1
      #define NUMA_WORKERS 12
    #endif
  #endif
  #ifndef NUMA_INIT
    #define NUMA_INIT 1
  #endif
#endif

#ifndef NUMA_STEAL
  #define NUMA_STEAL 1
#endif

#ifndef NUMA_INIT
  #define NUMA_INIT 0
#endif

#ifndef NUMA_WORKERS
  #define NUMA_WORKERS 0
#endif

#ifndef CHUNK_BITS
  #define CHUNK_BITS 0
#endif

#ifndef PARALLEL
  #define PARALLEL 0
#endif

#ifndef DISTANCE
  #define DISTANCE 1
#elif DISTANCE == 0
  #ifndef IN_PLACE
    #define IN_PLACE 0
  #endif
#elif DISTANCE == 1
  #ifndef IN_PLACE
    #define IN_PLACE 1
  #endif
#endif

#ifndef IN_PLACE
  #define IN_PLACE 1
#endif

#ifndef TEST_CONVERGENCE
  #define TEST_CONVERGENCE 0
#endif

#ifndef USE_GLOBAL_REST_LENGTH
  #define USE_GLOBAL_REST_LENGTH 1
#endif

#ifndef USE_RANDOM_PRIORITY_HASH
  #define USE_RANDOM_PRIORITY_HASH 0
#endif
//  this switch allows you to print the histogram
//  of the edge lengths in the graph, centered
//  on the average edge length as measured at T=0
#ifndef PRINT_EDGE_LENGTH_HISTOGRAM
  #define PRINT_EDGE_LENGTH_HISTOGRAM 0
#endif

//  this is the number of evenly spaced
//  buckets used by the edge length histogram
#ifndef NUM_BUCKETS
  #define NUM_BUCKETS 63
#endif

#ifndef APPLICATION
  #define APPLICATION 0
#endif

#ifndef NORMALIZE_TO_UNIT_CUBE
  #define NORMALIZE_TO_UNIT_CUBE 1
#endif

#ifndef PAGERANK
  #define PAGERANK 0
#endif

#ifndef MASS_SPRING_DASHPOT
  #define MASS_SPRING_DASHPOT 0
#endif

#ifndef EXECUTION_ORDER_SORT
  #define EXECUTION_ORDER_SORT 0
#endif

#if MASS_SPRING_DASHPOT
  #define VERTEX_META_DATA 1
  #define APP_NAME "MASS_SPRING_DASHPOT"
#elif PAGERANK
  #define VERTEX_META_DATA 0
  #define APP_NAME "PAGERANK"
#elif EXECUTION_ORDER_SORT
  #define VERTEX_META_DATA 0
  #define APP_NAME "EXECUTION_ORDER_SORT"
#else
  #error "No application selected"
#endif

#if PARALLEL
  #include <cilk/cilk.h>
  #include <cilk/cilk_api.h>
#else
  #define cilk_for for
  #define cilk_spawn
  #define cilk_sync
#endif

#include <cinttypes>
#include <cassert>
#include "../libgraphio/libgraphio.h"
#include "./concurrent_queue.h"

#ifndef TEST_SIMPLE_AND_UNDIRECTED
  #define TEST_SIMPLE_AND_UNDIRECTED 0
#endif

WHEN_TEST(
  extern volatile uint64_t roundUpdateCount;
)

#if D0_BSP
  #define NEEDS_SCHEDULER_DATA 0
  #include "./bsp_scheduling.h"
  #define SCHEDULER_NAME "BSP"
#elif D1_CHUNK
  #define NEEDS_SCHEDULER_DATA 1
  #include "./chunk_scheduling.h"
  #define SCHEDULER_NAME "CHUNK"
#elif D1_PHASE
  #define NEEDS_SCHEDULER_DATA 1
  #include "./phase_scheduling.h"
  #define SCHEDULER_NAME "PHASE"
#elif D1_CHROM
  #define NEEDS_SCHEDULER_DATA 0
  #include "./chromatic_scheduling.h"
  #define SCHEDULER_NAME "CHROMATIC"
#elif D1_NUMA
  #define NEEDS_SCHEDULER_DATA 1
  #include "./numa_scheduling.h"
  #define SCHEDULER_NAME "NUMA"
#elif D1_LOCKS
  #define NEEDS_SCHEDULER_DATA 1
  #include "./locks_scheduling.h"
  #define SCHEDULER_NAME "LOCKS"
#elif BASELINE || D1_PRIO
  #define NEEDS_SCHEDULER_DATA 1
  #include "./priority_scheduling.h"
  #define SCHEDULER_NAME "PRIORITY"
#else
  #error "No scheduling type defined!"
  #error "Specify one of BASELINE, D0_BSP, D1_PRIO, D1_CHUNK, D1_PHASE, D1_NUMA."
#endif

#include "./io.h"

#endif  // COMMON_H_
