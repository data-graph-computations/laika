#ifndef BSP_SCHEDULING_H_
#define BSP_SCHEDULING_H_

#if D0_BSP

#include "./common.h"
#include "./update_function.h"

struct scheddata_t { };
typedef struct scheddata_t scheddata_t;

static void init_scheduling(vertex_t * const nodes, const vid_t cntNodes,
                            scheddata_t * const scheddata) {
  // no-op
}

static void execute_round(const int round, vertex_t * const nodes, const vid_t cntNodes,
                          scheddata_t * const scheddata) {
  WHEN_DEBUG({
    cout << "Running bsp round " << round << endl;
  })

  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    update(nodes, i);
  }
}

static void cleanup_scheduling(vertex_t * const nodes, const vid_t cntNodes,
                               scheddata_t * const scheddata) {
  // no-op
}

static void print_execution_data() {
  // no-op
}

#endif  // D0_BSP

#endif  // BSP_SCHEDULING_H_
