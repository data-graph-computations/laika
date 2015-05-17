#ifndef EDGE_GENERATOR_H_
#define EDGE_GENERATOR_H_

#include <vector>
#include <algorithm>
#include "./common.h"

int generateEdges(vertex_t * const nodes,
                  std::vector<vid_t> * const edges,
                  const vid_t cntNodes,
                  const double maxEdgeLength);

#endif  // EDGE_GENERATOR_H_
