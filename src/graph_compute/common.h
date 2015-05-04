#ifndef COMMON_H_
#define COMMON_H_

#include <cinttypes>
#include <cassert>

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

#ifndef BASELINE
  #define BASELINE 0
#endif

#ifndef D0_BSP
  #define D0_BSP 0
#endif

#ifndef D1_PRIO
  #define D1_PRIO 0
#endif

#ifndef D1_CHUNK
  #define D1_CHUNK 0
#endif

#ifndef PARALLEL
  #define PARALLEL 0
#endif

#if PARALLEL
  #include <cilk/cilk.h>
#else
  #define cilk_for for
  #define cilk_spawn
  #define cilk_sync
#endif

typedef uint64_t vid_t;  // vertex id type

struct vertex_t {
  vid_t id;
  vid_t priority;
  size_t dependencies;
  size_t satisfied;
  size_t cntEdges;
  vid_t * edges;
  double data;
};
typedef struct vertex_t vertex_t;

#endif  // COMMON_H_
