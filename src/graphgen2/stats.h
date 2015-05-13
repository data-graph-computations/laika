#ifndef STATS_H_
#define STATS_H_

#include <vector>
#include "./common.h"

void printGraphStats(const vertex_t * const nodes,
                     const std::vector<vid_t> * const edges,
                     const vid_t cntNodes);

#endif  // STATS_H_
