#ifndef NUMA_SCHEDULING_H_
#define NUMA_SCHEDULING_H_
/*


#if D1_NUMA

#include <algorithm>
#include <unordered_set>
#include "./common.h"
#include "./update_function.h"

#ifndef CHUNK_BITS
  #define CHUNK_BITS 16
#endif

vid_t logBaseTwoRoundUp(vid_t x) {
  vid_t power = 0;
  while ((1 << power) < x) {
    power++;
  }
  return power;
}

struct chunkdata_t {
  vid_t nextIndex;  // the next vertex in this chunk to be processed
  vid_t phaseEndIndex[2];   // the index of the first vertex beyond this chunk
};
typedef struct chunkdata_t chunkdata_t;

struct scheddata_t {
  numaInit_t numaInit;
  vid_t * dependentEdges;
  vid_t * dependentEdgeIndex;
  vid_t * cntDependentEdges;
  chunkdata_t * chunkdata;
  vid_t * chunkQueueData;
  mrmw_queue_t chunkQ* 
  vid_t cntChunks;
};
typedef struct scheddata_t scheddata_t;

static inline bool samePhase(vid_t v, vid_t w, chunkdata_t * const chunkdata) {
  // this assumes that the boundary between phases is the
  // midpoint of the chunk - we will generalize later
  // to a custom boundary to optimize overall work
  bool vPhase = v < chunkdata[v >> CHUNK_BITS].phaseEndIndex[0];
  bool wPhase = w < chunkdata[w >> CHUNK_BITS].phaseEndIndex[0];
  return (vPhase == wPhase);
}

static inline bool interChunkDependency(vid_t v, vid_t w) {
  static const vid_t chunkMask = (1 << CHUNK_BITS) - 1;
  if ((v >> CHUNK_BITS) == (w >> CHUNK_BITS)) {
    return false;
  } else if ((v & chunkMask) == (w & chunkMask)) {
    return (v < w);
  } else {
    return ((v & chunkMask) < (w & chunkMask));
  }
}

static void calculateNeighborhood(std::unordered_set<vid_t> * neighbors,
                                  std::unordered_set<vid_t> * oldNeighbors,
                                  vid_t v,
                                  vertex_t * const nodes, vid_t distance) {
  neighbors->clear();
  neighbors->insert(v);
  oldNeighbors->clear();
  for (vid_t d = 0; d < distance; d++) {
    *oldNeighbors = *neighbors;
    for (const auto& v : *oldNeighbors) {
      for (vid_t j = 0; j < nodes[v].cntEdges; j++) {
        if (oldNeighbors->count(nodes[v].edges[j]) == 0) {
          neighbors->insert(nodes[v].edges[j]);
        }
      }
    }
  }
}

static void calculateNodeDependenciesChunk(vertex_t * const nodes,
                                           const vid_t cntNodes,
                                           scheddata_t * const sched) {
  sched->dependentEdgeIndex = static_cast<vid_t *>(numaCalloc(sched->numaInit, sizeof(vid_t), cntNodes));
  sched->cntDependentEdges = static_cast<vid_t *>(numaCalloc(sched->numaInit, sizeof(vid_t), cntNodes));
  vid_t cntDependencies = 0;
  std::unordered_set<vid_t> neighbors;
  neighbors.reserve(1024);
  std::unordered_set<vid_t> oldNeighbors;
  oldNeighbors.reserve(1024);
  for (vid_t i = 0; i < cntNodes; i++) {
    calculateNeighborhood(&neighbors, &oldNeighbors, i, nodes, DISTANCE);
    sched->dependentEdgeIndex[i] = cntDependencies;
    vertex_t * node = &nodes[i];
    node->dependencies = 0;
    vid_t outDep = 0;
    for (const auto& nbr : neighbors) {
      if (samePhase(nbr, i, sched->chunkdata)) {
        if (interChunkDependency(nbr, i)) {
          ++node->dependencies;
        } else if (interChunkDependency(i, nbr)) {
          cntDependencies++;
          outDep++;
        }
      }
    }
    node->satisfied = node->dependencies;
    sched->cntDependentEdges[i] = outDep;
  }
  printf("InterChunkDependencies: %lu\n",
    static_cast<uint64_t>(cntDependencies));
  sched->dependentEdges = static_cast<vid_t *>(numaCalloc(sched->numaInit, sizeof(vid_t), cntDependencies+1));
  for (vid_t i = 0; i < cntNodes; i++) {
    calculateNeighborhood(&neighbors, &oldNeighbors, i, nodes, DISTANCE);
    vid_t curIndex = sched->dependentEdgeIndex[i];
    for (const auto& nbr : neighbors) {
      if ((samePhase(nbr, i, sched->chunkdata))
        && (interChunkDependency(i, nbr))) {
          sched->dependentEdges[curIndex++] = nbr;
      }
    }
  }
}

static void createChunkData(vertex_t * const nodes, const vid_t cntNodes,
                            scheddata_t * const scheddata) {
  scheddata->cntChunks = (cntNodes + (1 << CHUNK_BITS) - 1) >> CHUNK_BITS;
  //scheddata->chunkdata = new (std::nothrow) chunkdata_t[scheddata->cntChunks];
  numaInit_t tmpConfig = scheddata->numaInit;
  tmpConfig.chunkBits = 0;
  scheddata->chunkdata = static_cast<chunkdata_t *>(numaCalloc(tmpConfig, sizeof(chunkdata_t), scheddata->cntChunks));
  assert(scheddata->chunkdata != NULL);

  for (vid_t i = 0; i < scheddata->cntChunks; ++i) {
    chunkdata_t * chunk = &scheddata->chunkdata[i];
    chunk->nextIndex = i << CHUNK_BITS;
    chunk->phaseEndIndex[0] = std::min(chunk->nextIndex + (1 << (CHUNK_BITS - 1)),
      cntNodes);
    chunk->phaseEndIndex[1] = std::min((i + 1) << CHUNK_BITS, cntNodes);
    // put code to greedily move boundaryIndex to minimize cost of
    // interChunk dependencies here
  }
}

static void init_scheduling(vertex_t * const nodes, const vid_t cntNodes,
                            scheddata_t * const scheddata) {
  createChunkData(nodes, cntNodes, scheddata);
  calculateNodeDependenciesChunk(nodes, cntNodes, scheddata);

  vid_t numChunksLog = logBaseTwoRoundUp(((cntNodes-1) >> scheddata->numaInit.chunkBits) + 1);
  vid_t numChunks = 1 << numChunksLog;
  vid_t queueLength = numChunks*static_cast<vid_t>(scheddata->numaInit.numWorkers);
  scheddata->chunkQueueData = static_cast<vid_t *>(numaCalloc(scheddata->numaInit, sizeof(vid_t), queueLength));
  scheddata->chunkQ = static_cast<mrmw_queue_t *>(scheddata->numaInit.numWorkers*sizeof(mrmw_queue_t *))
  for (int i = 0; i < scheddata->numaInit.numWorkers; i++) {
    scheddata->chunkQ[i] = mrmw_queue_t(chunkQueueData + (i*numChunks), static_cast<size_t>(numChunksLog)); 
    // push this worker's chunks into this queue
  }
}

static void execute_round(const int numRounds, vertex_t * const nodes,
                          const vid_t cntNodes, scheddata_t * const scheddata) {

  volatile int remainingRounds = numRounds;
  volatile int remainingPhases = NUM_PHASES;
  volatile int remainingChunks = cntChunks;

  for (vid_t i = 0; i < scheddata->cntChunks; i++) {
    scheddata->chunkdata[i].nextIndex = i << CHUNK_BITS;
  }

  const int NUM_PHASES = 2;
  for (int phase = 0; phase < NUM_PHASES; phase++) {

  }

}

static void execute_round(const int numRounds, vertex_t * const nodes,
                          const vid_t cntNodes, scheddata_t * const scheddata) {
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  for (int round = 0; round < numRounds; ++round) {
    WHEN_DEBUG({
      cout << "Running chunk round" << round << endl;
    })

    for (vid_t i = 0; i < scheddata->cntChunks; i++) {
      scheddata->chunkdata[i].nextIndex = i << CHUNK_BITS;
    }

    const int NUM_PHASES = 2;
    for (int phase = 0; phase < NUM_PHASES; phase++) {
      volatile bool doneFlag = false;
      while (!doneFlag) {
        doneFlag = true;
        cilk_for (vid_t i = 0; i < scheddata->cntChunks; i++) {
          chunkdata_t * chunk = &scheddata->chunkdata[i];
          vid_t j = chunk->nextIndex;
          bool localDoneFlag = false;
          while (!localDoneFlag && (j < chunk->phaseEndIndex[phase])) {
            if (nodes[j].satisfied == 0) {
              update(nodes, j);
              if (DISTANCE > 0) {
                nodes[j].satisfied = nodes[j].dependencies;
                vid_t edgeIndex = scheddata->dependentEdgeIndex[j];
                vid_t * edges = &scheddata->dependentEdges[edgeIndex];
                for (vid_t k = 0; k < scheddata->cntDependentEdges[j]; k++) {
                  __sync_sub_and_fetch(&nodes[edges[k]].satisfied, 1);
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
  }
}

static void cleanup_scheduling(vertex_t * const nodes, const vid_t cntNodes,
                               scheddata_t * const scheddata) {
  free(scheddata->chunkdata);
  free(scheddata->dependentEdges);
  free(scheddata->dependentEdgeIndex);
  free(scheddata->cntDependentEdges);
  free(scheddata->chunkQ);
}

static void print_execution_data() {
  cout << "Chunk size bits: " << CHUNK_BITS << '\n';
}

#endif  // D1_NUMA

*/
#endif  // NUMA_SCHEDULING_H_
