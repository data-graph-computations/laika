#ifndef CHUNK_SCHEDULING_H_
#define CHUNK_SCHEDULING_H_

#if D1_CHUNK

#include <algorithm>
#include "./common.h"
#include "./update_function.h"

#ifndef CHUNK_BITS
  #define CHUNK_BITS 16
#endif

struct chunkdata_t {
  vid_t nextIndex;  // the next vertex in this chunk to be processed
  vid_t endIndex;   // the index of the first vertex beyond this chunk
};
typedef struct chunkdata_t chunkdata_t;

struct scheddata_t {
  chunkdata_t * chunkdata;
  vid_t cntChunks;
};
typedef struct scheddata_t scheddata_t;

static inline bool interChunkDependency(vid_t v, vid_t w) {
  static const vid_t chunkMask = (1 << CHUNK_BITS) - 1;
  if ((v >> CHUNK_BITS) == (w >> CHUNK_BITS)) {
    return false;
  } else {
    return ((v & chunkMask) < (w & chunkMask));
  }
}

static inline bool chunkDependency(vid_t v, vid_t w) {
  static const vid_t chunkMask = (1 << CHUNK_BITS) - 1;
  if ((v & chunkMask) == (w & chunkMask)) {
    return ((v >> CHUNK_BITS) < (w >> CHUNK_BITS));
  } else {
    return ((v & chunkMask) < (w & chunkMask));
  }
}

static void calculateNodeDependenciesChunk(vertex_t * const nodes,
                                           const vid_t cntNodes) {
  cilk_for (vid_t i = 0; i < cntNodes; i++) {
    vertex_t * node = &nodes[i];
    node->dependencies = 0;
    for (vid_t j = 0; j < node->cntEdges; j++) {
      if (interChunkDependency(node->edges[j], i)) {
        ++node->dependencies;
      }
    }
    node->satisfied = node->dependencies;
  }
}

// for each node, move inter-chunk successors to the front of the edges list
static void orderEdgesByChunk(vertex_t * const nodes, const vid_t cntNodes) {
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    std::stable_partition(nodes[i].edges, nodes[i].edges + nodes[i].cntEdges,
      [i](const vid_t& val) {return interChunkDependency(i, val);});
  }
}

static void createChunkData(vertex_t * const nodes, const vid_t cntNodes,
                            scheddata_t * const scheddata) {
  scheddata->cntChunks = (cntNodes + (1 << CHUNK_BITS) - 1) >> CHUNK_BITS;
  scheddata->chunkdata = new (std::nothrow) chunkdata_t[scheddata->cntChunks];
  assert(scheddata->chunkdata != NULL);

  for (vid_t i = 0; i < scheddata->cntChunks; ++i) {
    scheddata->chunkdata[i].endIndex = std::min((i + 1) << CHUNK_BITS, cntNodes);
  }
}

static void init_scheduling(vertex_t * const nodes, const vid_t cntNodes,
                            scheddata_t * const scheddata) {
  orderEdgesByChunk(nodes, cntNodes);
  calculateNodeDependenciesChunk(nodes, cntNodes);
  createChunkData(nodes, cntNodes, scheddata);
}

static void execute_round(const int round, vertex_t * const nodes,
                          const vid_t cntNodes, scheddata_t * const scheddata) {
  WHEN_DEBUG({
    cout << "Running chunk round" << round << endl;
  })

  for (vid_t i = 0; i < scheddata->cntChunks; i++) {
    scheddata->chunkdata[i].nextIndex = i << CHUNK_BITS;
  }

  volatile bool doneFlag = false;
  while (!doneFlag) {
    doneFlag = true;
    cilk_for (vid_t i = 0; i < scheddata->cntChunks; i++) {
      vid_t j = scheddata->chunkdata[i].nextIndex;
      bool localDoneFlag = false;
      while (!localDoneFlag && (j < scheddata->chunkdata[i].endIndex)) {
        if (nodes[j].satisfied == 0) {
          update(nodes, j);
          nodes[j].satisfied = nodes[j].dependencies;
          vid_t k = 0;
          while (k < nodes[j].cntEdges) {
            if (interChunkDependency(j, nodes[j].edges[k])) {
              __sync_sub_and_fetch(&nodes[nodes[j].edges[k]].satisfied, 1);
              k++;
            } else {
              break;
            }
          }
        } else {
          scheddata->chunkdata[i].nextIndex = j;
          localDoneFlag = true;  // we couldn't process one of the nodes, so break
          doneFlag = false;  // we couldn't process one, so we need another round
        }
        j++;
      }
      if (!localDoneFlag) {
        scheddata->chunkdata[i].nextIndex = j;
      }
    }
  }
}

static void cleanup_scheduling(vertex_t * const nodes, const vid_t cntNodes,
                               scheddata_t * const scheddata) {
  delete[] scheddata->chunkdata;
}

static void print_execution_data() {
  cout << "Chunk size bits: " << CHUNK_BITS << '\n';
}

#endif  // D1_CHUNK

#endif  // CHUNK_SCHEDULING_H_
