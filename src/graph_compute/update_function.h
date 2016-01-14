#ifndef UPDATE_FUNCTION_H_
#define UPDATE_FUNCTION_H_

#include <math.h>
#include <string.h>
#include <string>
#include <algorithm>
#include "./common.h"

WHEN_TEST(
  extern volatile uint64_t roundUpdateCount;
)

using namespace std;

#if EXECUTION_ORDER_SORT
  #include "./execution_order_sort_struct_defs.h"
#elif PAGERANK
  #include "./pagerank_struct_defs.h"
#elif MASS_SPRING_DASHPOT
  #include "./msd_struct_defs.h"
#endif

typedef struct data_t data_t;
typedef struct global_t global_t;

typedef struct vertex_t {
  vid_t * edges;
  vid_t cntEdges;

  #if IN_PLACE
    data_t data;
  #else
    data_t data[2];
  #endif

  #if NEEDS_SCHEDULER_DATA
    sched_t sched;
  #endif
} vertex_t;

//  Every scheduling algorithm is required to define
//  a sched_t datatype which includes whatever per-vertex data
//  they need to perform their scheduling (e.g., priority, satisfied, dependencies etc.)
#if EXECUTION_ORDER_SORT
  #include "./execution_order_sort_update_function.h"
#elif PAGERANK
  #include "./pagerank_update_function.h"
#elif MASS_SPRING_DASHPOT
  #include "./msd_update_function.h"
#endif

#endif  // UPDATE_FUNCTION_H_
