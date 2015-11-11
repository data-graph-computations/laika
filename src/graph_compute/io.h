#ifndef IO_H_
#define IO_H_

#include <pthread.h>
#include <string>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <iostream>
#include "./common.h"
#include "../libgraphio/libgraphio.h"
#include "./numa_init.h"

using namespace std;

int readEdgesFromFile(const string filepath, vertex_t ** outNodes,
                      vid_t * outCntNodes, numaInit_t numaInit);

static inline bool reverseEdgeExists(vertex_t * const node, vid_t v) {
  for (vid_t edge = 0; edge < node->cntEdges; edge++) {
    if (node->edges[edge] == v) {
      return true;
    }
  }
  return false;
}

WHEN_TEST(
  static inline void testSimpleAndUndirected(vertex_t * const nodes,
                                          const vid_t cntNodes) {
    for (vid_t v = 0; v < cntNodes; v++) {
      for (vid_t edge = 0; edge < nodes[v].cntEdges; edge++) {
        assert(nodes[v].edges[edge] != v);  //  Test for no self edges
        assert(reverseEdgeExists(&nodes[nodes[v].edges[edge]], v));  //  test return edge
      }
    }
  }
)

void makeSimpleAndUndirected(vertex_t * nodes,
                             vid_t cntNodes, numaInit_t numaInit);

#endif  // IO_H_
