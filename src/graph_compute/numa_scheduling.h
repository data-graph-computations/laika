#ifndef NUMA_SCHEDULING_H_
#define NUMA_SCHEDULING_H_

#if D1_NUMA

#include <algorithm>
#include <unordered_set>
#include "./common.h"
#include "./concurrent_queue.h"
#include "./numa_init.h"

#if CHUNK_BITS < 1
  #error "CHUNK_BITS needs to be greater than 0 for D1_NUMA"
#endif

#if NUMA_WORKERS < 1
  #error "NUMA_WORKERS needs to be greater than 0 for D1_NUMA"
#endif

//  there is one of these per vertex
struct sched_t {
  vid_t * dependentEdges;  //  pointer into dependentEdges array in scheddata_t
  vid_t cntDependentEdges;  //  number of dependentEdges for this vertex
  vid_t dependencies;
  volatile vid_t satisfied;
};
typedef struct sched_t sched_t;

#include "./update_function.h"

// there is one of these per chunk
struct chunkdata_t {
  //  the index of the first vertex beyond this chunk
  //  this could be generalized to an arbitrary number of phases, but is not currently
  vid_t phaseEndIndex[2];
  vid_t nextIndex;  // the next vertex in this chunk to be processed
  vid_t workQueueNumber;
};
typedef struct chunkdata_t chunkdata_t;

struct scheddata_t;

struct numaSchedInit_t {
  int coreID;  //  this worker needs to know which core she is
  int numRounds;  //  total number of rounds to perform
  int numPhases;  //  number of phases per round
  vid_t cntNodes;
  vertex_t * nodes;
  scheddata_t * scheddata;
  global_t * globaldata;
  mrmw_queue_t * workQueue;
  volatile int * remainingChunks;
  volatile int * phase;
};
typedef struct numaSchedInit_t numaSchedInit_t;

//  there is one of these total
struct scheddata_t {
  //  dependentEdges holds the dependent edges array,
  //  one entry per inter-chunk dependency
  //  Each vertex has a sched_t, which contains a pointer
  //  (also called dependentEdges) into this array.
  vid_t * dependentEdges;  //  adjacency list of inter chunk dependencies
  numaSchedInit_t * numaSchedInit;  // init struct for pthreads
  chunkdata_t * chunkdata;  //  each chunk has metadata for its processing
  volatile vid_t * queueData;  //  data array for worker queues
  vid_t cntChunks;
  vid_t numChunksPerWorker;
};
typedef struct scheddata_t scheddata_t;

inline vid_t logBaseTwoRoundUp(vid_t x) {
  vid_t power = 0;
  static const vid_t one = 1;
  while ((one << power) < x) {
    power++;
  }
  return power;
}

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
                                         vertex_t * const nodes,
                                         vid_t distance) {
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
                                                  scheddata_t * const scheddata) {
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
    node->cntDependentEdges = 0;
    for (const auto& neighbor : neighbors) {
      if (samePhase(neighbor, i, scheddata->chunkdata)) {
        if (interChunkDependency(neighbor, i)) {
          node->dependencies++;
        } else if (interChunkDependency(i, neighbor)) {
          cntDependencies++;
          node->cntDependentEdges++;
        }
      }
    }
    node->satisfied = node->dependencies;
  }
  WHEN_TEST({
  printf("InterChunkDependencies: %lu\n",
    static_cast<uint64_t>(cntDependencies));
  })
  //  9 bits -> 4KB page granularity
  numaInit_t numaInit(NUMA_WORKERS,
                      std::min(9, CHUNK_BITS), static_cast<bool>(NUMA_INIT));
  scheddata->dependentEdges =
    static_cast<vid_t *>(numaCalloc(numaInit, sizeof(vid_t), cntDependencies+1));
  for (vid_t i = 0; i < cntNodes; i++) {
    nodes[i].sched.dependentEdges = &scheddata->dependentEdges[dependentEdgeIndex[i]];
    calculateNeighborhood(&neighbors, &oldNeighbors, i, nodes, DISTANCE);
    vid_t curIndex = dependentEdgeIndex[i];
    for (const auto& neighbor : neighbors) {
      if ((samePhase(neighbor, i, scheddata->chunkdata))
        && (interChunkDependency(i, neighbor))) {
          scheddata->dependentEdges[curIndex++] = neighbor;
      }
    }
  }
  delete[] dependentEdgeIndex;
}

static inline void createChunkData(vertex_t * const nodes,
                                   const vid_t cntNodes,
                                   scheddata_t * const scheddata) {
  scheddata->cntChunks = (cntNodes + (1 << CHUNK_BITS) - 1) >> CHUNK_BITS;
  scheddata->chunkdata = new (std::nothrow) chunkdata_t[scheddata->cntChunks]();
  assert(scheddata->chunkdata != NULL);
  scheddata->numChunksPerWorker =
    (scheddata->cntChunks + NUMA_WORKERS - 1) / NUMA_WORKERS;

  for (vid_t i = 0; i < scheddata->cntChunks; ++i) {
    chunkdata_t * chunk = &scheddata->chunkdata[i];
    chunk->nextIndex = i << CHUNK_BITS;
    chunk->phaseEndIndex[0] = std::min(chunk->nextIndex + (1 << (CHUNK_BITS - 1)),
      cntNodes);
    chunk->phaseEndIndex[1] = std::min((i + 1) << CHUNK_BITS, cntNodes);
    chunk->workQueueNumber = i / scheddata->numChunksPerWorker;
    assert(chunk->workQueueNumber < NUMA_WORKERS);
    // put code to greedily move boundaryIndex to minimize cost of
    // interChunk dependencies here
  }
}

static inline void populateWorkerParameters(vertex_t * const nodes,
                                            const vid_t cntNodes,
                                            scheddata_t * const scheddata) {
  const int NUM_PHASES = 2;
  scheddata->numaSchedInit =
    static_cast<numaSchedInit_t *>(malloc(sizeof(numaSchedInit_t)*NUMA_WORKERS));
  numaSchedInit_t * numaSchedInit = scheddata->numaSchedInit;
  //  a set of NUMA_WORKERS work queues for manages chunks
  vid_t logChunksPerWorker = logBaseTwoRoundUp(scheddata->numChunksPerWorker + 1);
  //  the data array used internally by the work queues
  numaInit_t numaInit(NUMA_WORKERS, 1, static_cast<bool>(NUMA_INIT));
  scheddata->queueData =
    static_cast<vid_t *>(numaCalloc(numaInit, sizeof(vid_t),
                                    NUMA_WORKERS << logChunksPerWorker));
  for (int i = 0; i < NUMA_WORKERS; i++) {
    numaSchedInit[i].coreID = i;
    numaSchedInit[i].numPhases = NUM_PHASES;
    numaSchedInit[i].cntNodes = cntNodes;
    numaSchedInit[i].nodes = nodes;
    numaSchedInit[i].scheddata = scheddata;
    numaSchedInit[i].workQueue =
      new (std::nothrow) mrmw_queue_t(&scheddata->queueData[i << logChunksPerWorker],
                                      logChunksPerWorker, static_cast<vid_t>(-1));
  }
}

static inline void init_scheduling(vertex_t * const nodes,
                                   const vid_t cntNodes,
                                   scheddata_t * const scheddata) {
  createChunkData(nodes, cntNodes, scheddata);
  calculateNodeDependenciesChunk(nodes, cntNodes, scheddata);
  populateWorkerParameters(nodes, cntNodes, scheddata);
}

inline void * processChunks(void * param) {
  numaSchedInit_t * config = static_cast<numaSchedInit_t *>(param);
  scheddata_t * scheddata = config->scheddata;
  numaSchedInit_t * numaSchedInit = scheddata->numaSchedInit;
  vid_t stealQueueNumber = config->coreID;
  // Initialize the chunk
  bindThreadToCore(config->coreID);
  static const vid_t SENTINEL = static_cast<vid_t>(-1);

  for (int round = 0; round < config->numRounds; round++) {
    for (int phase = 0; phase < config->numPhases; phase++) {
      //  load up chunks that belong to me
      vid_t start = config->coreID*scheddata->numChunksPerWorker;
      vid_t end = (config->coreID + 1)*scheddata->numChunksPerWorker;
      end = std::min(end, config->scheddata->cntChunks);
      for (vid_t chunk = start; chunk < end; chunk++) {
        config->workQueue->push(chunk);
      }
      //  When somebody completes the last chunk, they'll forward
      //  the config->phase variable.
      while (phase == *config->phase) {
        //  try to get a chunk from my own queue
        vid_t chunk = numaSchedInit[stealQueueNumber].workQueue->pop();
      #if NUMA_STEAL
        if (chunk == SENTINEL) {
          stealQueueNumber = config->coreID;
        }
      #endif
        while ((chunk == SENTINEL) && (phase == (*config->phase))) {
          //  randomly steal
          chunk = numaSchedInit[stealQueueNumber].workQueue->pop();
          if (chunk != SENTINEL) {
            stealQueueNumber++;
            //  finding queueNumber % numCores
            if (stealQueueNumber >= NUMA_WORKERS) {
              stealQueueNumber -= NUMA_WORKERS;
            }
          }
        }
        if (chunk != SENTINEL) {
          bool doneFlag = false;
          //  keep processing vertices from this chunk until it
          //  gets shelved or is done for this phase
          while (!doneFlag) {
            chunkdata_t * chunkdata = &config->scheddata->chunkdata[chunk];
            if (chunkdata->nextIndex < chunkdata->phaseEndIndex[phase]) {
              sched_t * node = &config->nodes[chunkdata->nextIndex].sched;
              if ((DISTANCE > 0)
                  && (node->satisfied != SENTINEL)
                  && (node->satisfied > 0)
                  && (__sync_sub_and_fetch(&node->satisfied, 1) != SENTINEL)) {
                //  If we need to shelve vertex, we're done with the chunk
                doneFlag = true;
              } else if (chunkdata->nextIndex < chunkdata->phaseEndIndex[phase]) {
                //  otherwise we process the vertex and decrement those
                //  vertices dependent on it
                update(config->nodes, chunkdata->nextIndex, config->globaldata, round);
                if (DISTANCE > 0) {
                  node->satisfied = node->dependencies;
                  for (vid_t edge = 0; edge < node->cntDependentEdges; edge++) {
                    //  if we discover a dependent vertex that we enable, we
                    //  push it on the appropriate work queue
                    sched_t * neighbor = &config->nodes[node->dependentEdges[edge]].sched;
                    if (__sync_sub_and_fetch(&neighbor->satisfied, 1) == SENTINEL) {
                      //  Released this chunk, so push it on its home work queue
                      vid_t enabledChunk = node->dependentEdges[edge] >> CHUNK_BITS;
                      vid_t queueNumber =
                        scheddata->chunkdata[enabledChunk].workQueueNumber;
                      numaSchedInit[queueNumber].workQueue->push(enabledChunk);
                    }
                  }
                }
                chunkdata->nextIndex++;
              }
            }
            if (chunkdata->nextIndex == chunkdata->phaseEndIndex[phase]) {
              //  this chunk is at the end of the phase
              doneFlag = true;
              if ((phase + 1) == config->numPhases) {
                //  we finished the entire chunk (i.e., all phases)
                //  so, we reset the index for next round
                chunkdata->nextIndex = (chunk << CHUNK_BITS);
              }
              if (__sync_sub_and_fetch(config->remainingChunks, 1) == 0) {
                //  this was the last chunk for this phase
                //  so, reset the remainingChunks counter and
                //  advance phase counter (mod numPhases)
                *config->remainingChunks = config->scheddata->cntChunks;
                (*config->phase)++;
                if (*config->phase == config->numPhases) {
                  *config->phase = 0;
                }
                //  make sure writes to these shared variables are visible
                __sync_synchronize();
              }
            }
          }
        }
      }
    }
  }
  return NULL;
}

static inline void execute_rounds(const int numRounds,
                                  vertex_t * const nodes,
                                  const vid_t cntNodes,
                                  scheddata_t * const scheddata,
                                  global_t * const globaldata) {
  //  the shared variable for tracking remaining chunks during a phase
  volatile int remainingChunks = scheddata->cntChunks;
  //  the shared variable for tracking the phase of the round
  volatile int phase = 0;
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  for (int i = 0; i < NUMA_WORKERS; i++) {
    scheddata->numaSchedInit[i].globaldata = globaldata;
    scheddata->numaSchedInit[i].numRounds = numRounds;
    scheddata->numaSchedInit[i].remainingChunks = &remainingChunks;
    scheddata->numaSchedInit[i].phase = &phase;
  }
  pthread_t * workers =
    static_cast<pthread_t *>(malloc(sizeof(pthread_t)*NUMA_WORKERS));
  for (int i = 0; i < NUMA_WORKERS; i++) {
    numaSchedInit_t * config = &scheddata->numaSchedInit[i];
    int result = pthread_create(&workers[i], NULL, processChunks,
      reinterpret_cast<void *>(config));
    assert(result == 0);
  }
  for (int i = 0; i < NUMA_WORKERS; i++) {
    int result = pthread_join(workers[i], NULL);
    assert(result == 0);
  }
}

static inline void cleanup_scheduling(vertex_t * const nodes,
                                      const vid_t cntNodes,
                                      scheddata_t * const scheddata) {
  delete[] scheddata->chunkdata;
  delete[] scheddata->dependentEdges;
}

static inline void print_execution_data() {
  cout << "Chunk size bits: " << CHUNK_BITS << '\n';
  cout << "Number of workers: " << NUMA_WORKERS << '\n';
}

#endif  // D1_NUMA

#endif  // NUMA_SCHEDULING_H_
