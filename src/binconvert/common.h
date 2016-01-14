#ifndef COMMON_H_
#define COMMON_H_

#include <cinttypes>
#include <cassert>
#include "../libgraphio/libgraphio.h"

#ifndef TEST
  #define TEST 1
#endif

#ifndef DEBUG
  #define DEBUG 0
#endif

#ifndef PARALLEL
  #define PARALLEL 1
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
  #ifdef NDEBUG
    #error "Cannot run in TEST mode with NDEBUG set! Unset NDEBUG to continue."
  #endif
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

#if PARALLEL
  #include <cilk/cilk.h>
#else
  #define cilk_for for
  #define cilk_spawn
  #define cilk_sync
#endif

#endif  // COMMON_H_
