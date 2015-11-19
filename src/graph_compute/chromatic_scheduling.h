#ifndef CHROMATIC_SCHEDULING_H_
#define CHROMATIC_SCHEDULING_H_

#if D1_CHROM

#include <algorithm>
#include "./common.h"

struct scheddata_t {
  vid_t cntColors;  //  total number of colors used
  vid_t * cntNodesPerColor;  //  number of vertices for each color
  vid_t * nodesByColor;  //  all vertices sorted by color
};
typedef struct scheddata_t scheddata_t;

//  This per-vertex struct includes the color of the vertex
struct sched_t {
};
typedef struct sched_t sched_t;

#include "./update_function.h"

static inline void testColoring(vertex_t * const nodes,
                                const vid_t cntNodes,
                                vid_t * const colorAssignments) {
  for (vid_t v = 0; v < cntNodes; v++) {
    for (vid_t edge = 0; edge < nodes[v].cntEdges; edge++) {
      assert(colorAssignments[v] != colorAssignments[nodes[v].edges[edge]]);
    }
  }
}

static inline vid_t colorGraph(vertex_t * const nodes,
                              const vid_t cntNodes,
                              vid_t * const colorAssignments) {
  bool * colors = new (std::nothrow) bool[cntNodes+1];
  vid_t maxColor = 0;
  for (vid_t v = 0; v < cntNodes; v++) {
    //  initialize all possible colors for v
    for (vid_t edge = 0; edge <= nodes[v].cntEdges; edge++) {
      colors[edge] = false;
    }
    //  for all lower-numbered vertices, remove alread-taken colors from consideration
    for (vid_t edge = 0; edge < nodes[v].cntEdges; edge++) {
      if (nodes[v].edges[edge] < v) {
        colors[colorAssignments[nodes[v].edges[edge]]] = true;
      }
    }
    //  initialize v's color to an impossible color
    static const vid_t NO_COLOR = cntNodes + 1;
    colorAssignments[v] = NO_COLOR;
    //  loop through possible colors and take the first that is available
    for (vid_t edge = 0; edge <= nodes[v].cntEdges; edge++) {
      //  color not yet taken
      if (colors[edge] == false) {
        if (colorAssignments[v] == NO_COLOR) {
          colorAssignments[v] = edge;
          maxColor = std::max(maxColor, edge);
        }
      }
    }
  }
  delete[] colors;
  WHEN_TEST(
    testColoring(nodes, cntNodes, colorAssignments);
  )
  return maxColor;
}

static inline void init_scheduling(vertex_t * const nodes,
                                   const vid_t cntNodes,
                                   scheddata_t * const scheddata) {
  //  array that stores per-vertex colors
  vid_t * colorAssignments = new (std::nothrow) vid_t[cntNodes];
  scheddata->cntColors = colorGraph(nodes, cntNodes, colorAssignments);
  //  how many vertices are there per color
  scheddata->cntNodesPerColor = new (std::nothrow) vid_t[scheddata->cntColors + 1]();
  scheddata->nodesByColor = new (std::nothrow) vid_t[cntNodes];
  //  we count up color assignments per color
  //  the +1 exists so that we can take the difference between subsequent colors
  //  in the next block to find indices into the nodesByColor array
  for (vid_t v = 0; v < cntNodes; v++) {
    scheddata->cntNodesPerColor[colorAssignments[v] + 1]++;
  }
  vid_t * index = new (std::nothrow) vid_t[scheddata->cntColors + 1]();
  index[0] = 0;
  //  find pointers into nodesByColor array in index array
  for (vid_t c = 1; c <= scheddata->cntColors; c++) {
    scheddata->cntNodesPerColor[c] += scheddata->cntNodesPerColor[c - 1];
    index[c] = scheddata->cntNodesPerColor[c];
  }
  //  copy vertex id's into appropriate range of nodesByColor
  for (vid_t v = 0; v < cntNodes; v++) {
    vid_t color = colorAssignments[v];
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
  delete[] scheddata->cntNodesPerColor;
  delete[] scheddata->nodesByColor;
}

static inline void print_execution_data() {
  // no-op
}

#endif  // D1_CHROM

#endif  // CHROMATIC_SCHEDULING_H_
