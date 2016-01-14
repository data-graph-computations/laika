#ifndef PAGERANK_STRUCT_DEFS_H_
#define PAGERANK_STRUCT_DEFS_H_

#include <cmath>
#include <cstring>
#include <string>
#include <algorithm>
#include "./common.h"

typedef double pagerank_t;

struct data_t {
  pagerank_t pagerank;
  pagerank_t contrib;
};

struct global_t {
  pagerank_t d;

  #if TEST_CONVERGENCE
    pagerank_t * sumSqDelta;
  #endif
};

#endif  // PAGERANK_STRUCT_DEFS_H_
