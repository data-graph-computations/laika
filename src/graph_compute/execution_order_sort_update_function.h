#ifndef EXECUTION_ORDER_SORT_UPDATE_FUNCTION_H_
#define EXECUTION_ORDER_SORT_UPDATE_FUNCTION_H_

#include <cmath>
#include <cstring>
#include <string>
#include <algorithm>
#include "./common.h"

inline static uint64_t hashOfVertexData(data_t * vertex) {
  return vertex->execution_number;
}

inline static void fillInNodeData(vertex_t * const nodes,
                                  const vid_t cntNodes) {
  // no-op
}

inline static void fillInGlobalData(vertex_t * const nodes,
                                    const vid_t cntNodes,
                                    global_t * const globaldata,
                                    int numRounds) {
  globaldata->executed_nodes = 0;
}

inline void update(vertex_t * const nodes,
                   const vid_t index,
                   global_t * const globaldata,
                   const int round = 0) {
  // ensuring that the number of updates in total is correct per round
  WHEN_TEST({
    __sync_add_and_fetch(&roundUpdateCount, 1);
  })

  nodes[index].data.execution_number =
    __sync_fetch_and_add(&globaldata->executed_nodes, 1);
}

#endif  // EXECUTION_ORDER_SORT_UPDATE_FUNCTION_H_
