#ifndef BSP_SCHEDULING_H_
#define BSP_SCHEDULING_H_

#if D0_BSP || BASELINE

#include "./common.h"
#include "./update_function.h"

struct scheddata_t { };
typedef struct scheddata_t scheddata_t;

struct sched_t { };
typedef struct sched_t sched_t;

static void init_scheduling(vertex_t * const nodes, const vid_t cntNodes,
                            scheddata_t * const scheddata) {
  // no-op
}

static void execute_round(const int numRounds, vertex_t * const nodes,
                          const vid_t cntNodes,
                          scheddata_t * const scheddata) {
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  for (int round = 0; round < numRounds; ++round) {
    WHEN_DEBUG({
      cout << "Running bsp round " << round << endl;
    })

    cilk_for (vid_t i = 0; i < cntNodes; ++i) {
      update(nodes, i, round);
    }
  }
}

static void cleanup_scheduling(vertex_t * const nodes, const vid_t cntNodes,
                               scheddata_t * const scheddata) {
  // no-op
}

static void print_execution_data() {
  // no-op
}

#endif  // D0_BSP || BASELINE

#endif  // BSP_SCHEDULING_H_
