#ifndef CHUNK_SCHEDULING_H_
#define CHUNK_SCHEDULING_H_

#if D1_CHUNK

#include <algorithm>
#include "./common.h"
#include "./update_function.h"

#ifndef CHUNK_BITS
  #define CHUNK_BITS 16
#endif

static inline bool chunkDependency(vid_t v, vid_t w) {
  static const vid_t chunkMask = (1 << CHUNK_BITS) - 1;
  if ((v & chunkMask) == (w & chunkMask)) {
    return ((v >> CHUNK_BITS) < (w >> CHUNK_BITS));
  } else {
    return ((v & chunkMask) < (w & chunkMask));
  }
}

static void calculateNodeDependenciesChunk(vertex_t * nodes, const vid_t cntNodes) {
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    int chunkId = i >> CHUNK_BITS;
    vertex_t * node = &nodes[i];
    node->dependencies = 0;
    for (size_t j = 0; j < node->cntEdges; ++j) {
      if ((static_cast<int>(node->edges[j]) >> CHUNK_BITS) != chunkId) {
        if (chunkDependency(node->edges[j], i) == true) {
          ++node->dependencies;
        }
      }
    }
    node->satisfied = node->dependencies;
  }
}

static void init_scheduling(vertex_t * nodes, const vid_t cntNodes) {
  calculateNodeDependenciesChunk(nodes, cntNodes);
}

static void execute_round(const int round, vertex_t * nodes, const vid_t cntNodes) {
  WHEN_DEBUG({
    cout << "Running chunk round" << round << endl;
  })

  // cilk_for (vid_t i = 0; i < cntNodes; i++) {
  //   nodes[i].satisfied = nodes[i].dependencies;
  // }

  static const vid_t chunkMask = (1 << CHUNK_BITS) - 1;
  vid_t numChunks = ((cntNodes + chunkMask) >> CHUNK_BITS);
  vid_t * chunkIndex = new vid_t[numChunks];
  for (vid_t i = 0; i < numChunks; i++) {
    chunkIndex[i] = i << CHUNK_BITS;
  }
  bool doneFlag = false;
  while (!doneFlag) {
    doneFlag = true;
    cilk_for (vid_t i = 0; i < numChunks; i++) {
      vid_t j = chunkIndex[i];
      bool localDoneFlag = false;
      while (!localDoneFlag && (j < std::min((i + 1) << CHUNK_BITS, cntNodes))) {
        if (nodes[j].satisfied == 0) {
          update(nodes, j);
          nodes[j].satisfied = nodes[j].dependencies;
          for (vid_t k = 0; k < nodes[j].cntEdges; k++) {
            if ((nodes[j].edges[k] >> CHUNK_BITS) != i) {
              if (chunkDependency(j, nodes[j].edges[k]) == true) {
                __sync_sub_and_fetch(&nodes[nodes[j].edges[k]].satisfied, 1);
              }
            }
          }
        } else {
          chunkIndex[i] = j;
          localDoneFlag = true;  // we couldn't process one of the nodes, so break
          doneFlag = false;  // we couldn't process one, so we need another round
        }
        j++;
      }
      if (!localDoneFlag) {
        chunkIndex[i] = j;
      }
    }
  }
  delete chunkIndex;
}

static void print_execution_data() {
  cout << "Chunk size bits: " << CHUNK_BITS << '\n';
}

#endif  // D1_CHUNK

#endif  // CHUNK_SCHEDULING_H_
