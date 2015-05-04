#ifndef BSP_SCHEDULING_H_
#define BSP_SCHEDULING_H_

#if D0_BSP

#include "./common.h"
#include "./update_function.h"

static void init_scheduling(vertex_t * nodes, const vid_t cntNodes) {
  // no-op
}

static void execute_round(const int round, vertex_t * nodes, const vid_t cntNodes) {
  WHEN_DEBUG({
    cout << "Running bsp round " << round << endl;
  })

  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    update(nodes, i);
  }
}

static void print_execution_data() {
  // no-op
}

#endif  // D0_BSP

#endif  // BSP_SCHEDULING_H_
