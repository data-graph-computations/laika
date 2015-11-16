#ifndef CHROMATIC_SCHEDULING_H_
#define CHROMATIC_SCHEDULING_H_

#if D1_CHROM

#include <algorithm>
#include "./common.h"

struct scheddata_t {
  vid_t cntColors;
  vid_t * cntNodesPerColor;
  vid_t * nodesByColor;
};
typedef struct scheddata_t scheddata_t;

struct sched_t {
  vid_t color;
};
typedef struct sched_t sched_t;

#include "./update_function.h"

static inline vid_t colorGraph(vertex_t * const nodes,
                              const vid_t cntNodes) {
  bool * colors = new (std::nothrow) bool[cntNodes+1];
  vid_t maxColor = 0;
  for (vid_t v = 0; v < cntNodes; v++) {
    for (vid_t edge = 0; edge <= nodes[v].cntEdges; edge++) {
      colors[edge] = false;
    }
    for (vid_t edge = 0; edge < nodes[v].cntEdges; edge++) {
      if (nodes[v].edges[edge] < v) {
        colors[edge] = true;
      }
    }
    static const vid_t MAX_COLOR = cntNodes + 1;
    nodes[v].sched.color = MAX_COLOR;
    for (vid_t edge = 0; edge <= nodes[v].cntEdges; edge++) {
      //  color not yet taken
      if (colors[edge] == false) {
        if (nodes[v].sched.color == MAX_COLOR) {
          nodes[v].sched.color = edge;
          maxColor = std::max(maxColor, edge);
        }
      }
    }
  }
  return maxColor;
}

static inline void init_scheduling(vertex_t * const nodes,
                                   const vid_t cntNodes,
                                   scheddata_t * const scheddata) {
  scheddata->cntColors = colorGraph(nodes, cntNodes);
  scheddata->cntNodesPerColor = new (std::nothrow) vid_t[scheddata->cntColors + 1]();
  scheddata->nodesByColor = new (std::nothrow) vid_t[cntNodes];
  for (vid_t v = 0; v < cntNodes; v++) {
    scheddata->cntNodesPerColor[nodes[v].sched.color + 1]++;
  }
  vid_t * index = new (std::nothrow) vid_t[scheddata->cntColors + 1]();
  index[0] = 0;
  for (vid_t c = 1; c <= scheddata->cntColors; c++) {
    scheddata->cntNodesPerColor[c] += scheddata->cntNodesPerColor[c - 1];
    index[c] = scheddata->cntNodesPerColor[c];
  }
  for (vid_t v = 0; v < cntNodes; v++) {
    vid_t color = nodes[v].sched.color;
    scheddata->nodesByColor[index[color]] = v;
    index[color]++;
  }
}

static inline void execute_rounds(const int numRounds,
                                  vertex_t * const nodes,
                                  const vid_t cntNodes,
                                  scheddata_t * const scheddata,
                                  global_t * const globaldata) {
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  for (int round = 0; round < numRounds; ++round) {
    WHEN_DEBUG({
      cout << "Running chromatic round " << round << endl;
    })
    for (vid_t c = 0; c < scheddata->cntColors; c++) {
      vid_t start = scheddata->cntNodesPerColor[c];
      vid_t stop = scheddata->cntNodesPerColor[c + 1];
      cilk_for (vid_t i = start; i < stop; i++) {
        update(nodes, scheddata->nodesByColor[i], globaldata, round);
      }
    }
  }
}

static inline void cleanup_scheduling(vertex_t * const nodes,
                                      const vid_t cntNodes,
                                      scheddata_t * const scheddata) {
  // no-op
}

static inline void print_execution_data() {
  // no-op
}

#endif  // D1_CHROM

#endif  // CHROMATIC_SCHEDULING_H_
