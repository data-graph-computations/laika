#ifndef EXECUTION_ORDER_SORT_STRUCT_DEFS_H_
#define EXECUTION_ORDER_SORT_STRUCT_DEFS_H_

#include <cmath>
#include <cstring>
#include <string>
#include <algorithm>
#include "./common.h"

struct data_t {
  vid_t execution_number;
};

struct global_t {
  vid_t executed_nodes;
};

#endif  // EXECUTION_ORDER_SORT_STRUCT_DEFS_H_
