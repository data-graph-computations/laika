#ifndef NUMA_SCHEDULING_H_
#define NUMA_SCHEDULING_H_

#if D1_NUMA

#include <algorithm>
#include <unordered_set>
#include "./common.h"
#include "./numa_init.h"

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
  vid_t * dependentEdges;
//  vid_t * dependentEdgeIndex;
//  vid_t * cntDependentEdges;
  chunkdata_t * chunkdata;
  vid_t cntChunks;
};
typedef struct scheddata_t scheddata_t;

struct sched_t {
  vid_t * dependentEdges;
  vid_t cntDependentEdges;
  vid_t dependencies;
  volatile vid_t satisfied;
};
typedef struct sched_t sched_t;

#include "./update_function.h"

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

static inline void calculateNeighborhood(std::unordered_set<vid_t> * neighbors,
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

static inline void calculateNodeDependenciesChunk(vertex_t * const nodes,
                                           const vid_t cntNodes,
                                           scheddata_t * const sched) {
  vid_t * dependentEdgeIndex = new (std::nothrow) vid_t[cntNodes];
  vid_t cntDependencies = 0;
  std::unordered_set<vid_t> neighbors;
  neighbors.reserve(1024);
  std::unordered_set<vid_t> oldNeighbors;
  oldNeighbors.reserve(1024);
  for (vid_t i = 0; i < cntNodes; i++) {
    calculateNeighborhood(&neighbors, &oldNeighbors, i, nodes, DISTANCE);
    dependentEdgeIndex[i] = cntDependencies;
    sched_t * node = &nodes[i].sched;
    node->dependencies = 0;
    vid_t outDep = 0;
    for (const auto& neighbor : neighbors) {
      if (samePhase(neighbor, i, sched->chunkdata)) {
        if (interChunkDependency(neighbor, i)) {
          ++node->dependencies;
        } else if (interChunkDependency(i, neighbor)) {
          cntDependencies++;
          outDep++;
        }
      }
    }
    node->satisfied = node->dependencies;
    node->cntDependentEdges = outDep;
  }
  printf("InterChunkDependencies: %lu\n",
    static_cast<uint64_t>(cntDependencies));
  sched->dependentEdges = new (std::nothrow) vid_t[cntDependencies+1];
  for (vid_t i = 0; i < cntNodes; i++) {
    nodes->sched.dependentEdges = &sched->dependentEdges[dependentEdgeIndex[i]];
    calculateNeighborhood(&neighbors, &oldNeighbors, i, nodes, DISTANCE);
    vid_t curIndex = dependentEdgeIndex[i];
    for (const auto& neighbor : neighbors) {
      if ((samePhase(neighbor, i, sched->chunkdata))
        && (interChunkDependency(i, neighbor))) {
          sched->dependentEdges[curIndex++] = neighbor;
      }
    }
  }
}

static inline void createChunkData(vertex_t * const nodes, const vid_t cntNodes,
                            scheddata_t * const scheddata) {
  scheddata->cntChunks = (cntNodes + (1 << CHUNK_BITS) - 1) >> CHUNK_BITS;
  scheddata->chunkdata = new (std::nothrow) chunkdata_t[scheddata->cntChunks];
  assert(scheddata->chunkdata != NULL);

  cilk_for (vid_t i = 0; i < scheddata->cntChunks; ++i) {
    chunkdata_t * chunk = &scheddata->chunkdata[i];
    chunk->nextIndex = i << CHUNK_BITS;
    chunk->phaseEndIndex[0] = std::min(chunk->nextIndex + ((1 << CHUNK_BITS) >> 1),
      cntNodes);
    chunk->phaseEndIndex[1] = std::min((i + 1) << CHUNK_BITS, cntNodes);
    // put code to greedily move boundaryIndex to minimize cost of
    // interChunk dependencies here
  }
}

static inline void init_scheduling(vertex_t * const nodes, const vid_t cntNodes,
                            scheddata_t * const scheddata) {
  createChunkData(nodes, cntNodes, scheddata);
  calculateNodeDependenciesChunk(nodes, cntNodes, scheddata);
}

struct numaSchedInit_t {
  
};
typedef struct numaSchedInit_t numaSchedInit_t;

void * processChunks(void * param) {
  numaSchedInit_t * config = static_cast<numaSchedInit_t *>(param);
  // Initialize the chunk
  bindThreadToCore(config->coreID);
  size_t chunkSize = config->dataTypeSize << config->numaInit.chunkBits;
  size_t numChunks = (config->numBytes + chunkSize - 1) / chunkSize;
  size_t chunksPerThread = (numChunks + config->numaInit.numWorkers - 1)
    / config->numaInit.numWorkers;
  size_t start = config->coreID*chunksPerThread*chunkSize;
  size_t end = (config->coreID+1)*chunksPerThread*chunkSize;
  end = (config->numBytes < end) ? config->numBytes : end;
  unsigned char * data = static_cast<unsigned char *>(config->data);
  for (size_t i = start; i < end; i++) {
    data[i] = 0;
  }
  return NULL;
}

void numaInitWriteZeroes(numaInit_t config,
                        size_t dataTypeSize,
                        void *data,
                        size_t numBytes) {
  pthread_t * workers =
    static_cast<pthread_t *>(malloc(sizeof(pthread_t)*config.numWorkers));
  for (int i = 0; i < config.numWorkers; i++) {
    chunkInit_t chunk(config, i, dataTypeSize, data, numBytes);
    assert(pthread_create(&workers[i], NULL, writeZeroes, &chunk) == 0);
  }
  for (int i = 0; i < config.numWorkers; i++) {
    assert(pthread_join(workers[i], NULL) == 0);
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
            if (nodes[j].sched.satisfied == 0) {
              update(nodes, j, globaldata, round);
              if (DISTANCE > 0) {
                nodes[j].sched.satisfied = nodes[j].sched.dependencies;
                vid_t * edges = nodes[j].sched.dependentEdges;
                for (vid_t k = 0; k < nodes[j].sched.cntDependentEdges; k++) {
                  __sync_sub_and_fetch(&nodes[edges[k]].sched.satisfied, 1);
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

static inline void cleanup_scheduling(vertex_t * const nodes, const vid_t cntNodes,
                               scheddata_t * const scheddata) {
  delete[] scheddata->chunkdata;
  delete[] scheddata->dependentEdges;
}

static inline void print_execution_data() {
  cout << "Chunk size bits: " << CHUNK_BITS << '\n';
}

#endif  // D1_NUMA

#endif  // NUMA_SCHEDULING_H_
