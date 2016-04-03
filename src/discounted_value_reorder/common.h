#ifndef COMMON_H_
#define COMMON_H_

#include <cinttypes>
#include <cassert>
#include <utility>

#include "../libgraphio/libgraphio.h"

#ifndef TEST
  #define TEST 0
#endif

#ifndef DEBUG
  #define DEBUG 0
#endif

struct edges_t {
  vid_t cntEdges;
  vid_t * edges;
};
typedef struct edges_t edges_t;

struct vertex_t {
  vid_t id;
  vid_t reorderId;
  double value;
  edges_t edgeData;
};
typedef struct vertex_t vertex_t;

#endif  // COMMON_H_
