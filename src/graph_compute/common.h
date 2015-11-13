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

#ifndef NUM_CORES
  #define NUM_CORES 12
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
      #define NUMA_WORKERS NUM_CORES
    #endif
  #endif
  #ifndef NUMA_INIT
    #define NUMA_INIT 1
  #endif
  #ifndef NUMA_STEAL
    #define NUMA_STEAL 1
  #endif
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

#ifndef PAGERANK
  #define PAGERANK 1
#endif

#if PAGERANK == 0
  #define VERTEX_META_DATA 1
#else
  #define VERTEX_META_DATA 0
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

WHEN_TEST(
  extern volatile uint64_t roundUpdateCount;
)

#if D0_BSP
  #include "./bsp_scheduling.h"
  #define SCHEDULER_NAME "BSP"
#elif D1_CHUNK
  #include "./chunk_scheduling.h"
  #define SCHEDULER_NAME "CHUNK"
#elif D1_PHASE
  #include "./phase_scheduling.h"
  #define SCHEDULER_NAME "PHASE"
#elif D1_NUMA
  #include "./numa_scheduling.h"
  #define SCHEDULER_NAME "NUMA"
#elif BASELINE || D1_PRIO
  #include "./priority_scheduling.h"
  #define SCHEDULER_NAME "PRIORITY"
#else
  #error "No scheduling type defined!"
  #error "Specify one of BASELINE, D0_BSP, D1_PRIO, D1_CHUNK, D1_PHASE, D1_NUMA."
#endif

#include "./io.h"

#endif  // COMMON_H_
