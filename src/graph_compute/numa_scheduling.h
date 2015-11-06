#ifndef NUMA_SCHEDULING_H_
#define NUMA_SCHEDULING_H_

#if D1_NUMA

#include <algorithm>
#include <unordered_set>
#include "./common.h"
#include "./numa_init.h"

#ifndef CHUNK_BITS
  #define CHUNK_BITS 16
#elif CHUNK_BITS == 0
  #error "CHUNK_BITS needs to be greater than 0 for D1_NUMA"
#endif

#ifndef NUMA_WORKERS
  #define NUMA_WORKERS 12
#elif NUMA_WORKERS == 0
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

// there is one of these per chunk
struct chunkdata_t {
  vid_t phaseEndIndex[NUM_PHASES];   // the index of the first vertex beyond this chunk
  vid_t nextIndex;  // the next vertex in this chunk to be processed
  vid_t workQueue;
};
typedef struct chunkdata_t chunkdata_t;

//  there is one of these total
struct scheddata_t {
  //  dependentEdges holds the dependent edges array, 
  //  one entry per inter-chunk dependency
  //  Each vertex has a sched_t, which contains a pointer 
  //  (also called dependentEdges) into this array.
  vid_t * dependentEdges;
  chunkdata_t * chunkdata;
  vid_t cntChunks;
};
typedef struct scheddata_t scheddata_t;

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

static inline void createChunkData(vertex_t * const nodes,
                                   const vid_t cntNodes,
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

static inline void init_scheduling(vertex_t * const nodes,
                                   const vid_t cntNodes,
                                   scheddata_t * const scheddata) {
  createChunkData(nodes, cntNodes, scheddata);
  calculateNodeDependenciesChunk(nodes, cntNodes, scheddata);
}

struct numaSchedInit_t {
  int coreID;  //  this worker needs to know which core she is
  int numRounds;  //  total number of rounds to perform
  int numPhases;  //  number of phases per round
  int pseudoRandomNumber;  //  a number in range (0,numCores)
  vid_t numChunksPerWorker;
  vid_t cntNodes;
  vertex_t * nodes;
  scheddata_t * scheddata;
  global_t * globaldata;
  mrmw_queue_t * workQueue;
  volatile int * remainingChunks;
  volatile int * phase;
};
typedef struct numaSchedInit_t numaSchedInit_t;

void * processChunks(void * param) {
  numaSchedInit_t * config = static_cast<numaSchedInit_t *>(param);
  // Initialize the chunk
  bindThreadToCore(config->coreID);

  for (int round = 0; round < config->numRounds; round++) {
    for (int phase = 0; phase < config->numPhases; phase++) {
      //  load up chunks that belong to me
      vid_t start = config->coreID*config->numChunksPerWorker;
      vid_t end = (config->coreID + 1)*config->numChunksPerWorkerBits;
      end = std::min(end, config->scheddata->cntChunks);
      for (vid_t chunk = start; chunk < end; chunk++) {
        config->workQueue[config->coreID].push(chunk);
      }
      //  When somebody completes the last chunk, they'll forward
      //  the config->phase variable.
      while (phase == &config->phase) {
        //  try to get a chunk from my own queue
        vid_t chunk = -1;
        vid_t queueNumber = config->coreId;
        while ((chunk == -1) && (phase == &config->phase)) {
          //  randomly steal
          chunk = config->workQueue[queueNumber].pop();
          queueNumber += config->pseudoRandomNumber;
          //  finding queueNumber % numCores
          while (queueNumber >= NUMA_WORKERS) {
            queueNumber -= NUMA_WORKERS;
          }
        }
        if (chunk != -1) {
          bool doneFlag = false;
          //  keep processing vertices from this chunk until it
          //  gets shelved or is done for this phase
          while (!doneFlag) {
            chunkdata_t * chunkdata = &config->scheddata->chunkdata[chunk];
            if (chunkdata->nextIndex < chunkdata->phaseEndIndex[phase]) {
              sched_t * node = &config->nodes[chunkdata->nextIndex].sched;
              if ((DISTANCE > 0)
                  && (node->satisfied > 0)
                  && (__sync_sub_and_fetch(&node->satisfied, 1) != -1)) {
                //  If we need to shelve vertex, we're done with the chunk
                doneFlag = true;
              } else {
                //  otherwise we process the vertex and decrement those
                //  vertices dependent on it
                update(config->nodes, chunkdata->nextIndex, config->globaldata, round);
                if (DISTANCE > 0) {
                  node->satisfied = node->dependencies;
                  for (vid_t edge = 0; edge < node->cntDependentEdges; edge++) {
                    //  if we discover a dependent vertex that we enable, we
                    //  push it on the appropriate work queue
                    sched_t * neighbor = &config->nodes[node->dependentEdges[edge]].sched;
                    if (__sync_sub_and_fetch(&neighbor->satisfied, 1) == -1) {
                      //  Released this chunk, so push it on its home work queue
                      vid_t enabledChunk = node->dependentEdges[edge] >> CHUNK_BITS;
                      vid_t queueNumber = enabledChunk / config->numChunksPerWorker;
                      config->workQueue[queueNumber].push(node->dependentEdges[edge]);
                    }
                  }
                }
                chunkdata->nextIndex++;
              }
              if (chunkdata->nextIndex == chunkdata->phaseEndIndex[phase]) {
                //  this chunk is at the end of the phase
                doneFlag = true;
                if (phase == config->numPhases) {
                  //  we finished the entire chunk (i.e., all phases)
                  //  so, we reset the index for next round
                  chunkdata->nextIndex = (chunk << CHUNK_BITS);
                }
                if (__sync_sub_and_fetch(remainingChunks, 1) == 0) {
                  //  this was the last chunk for this phase
                  //  so, reset the remainingChunks counter and
                  //  advance phase counter (mod numPhases)
                  *remainingChunks = config->scheddata->cntChunks;
                  *config->phase++;
                  if (phase == config->numPhases) {
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
  const int NUM_PHASES = 2;
  numaSchedInit_t * numaSchedInit =
    static_cast<numaSchedInit_t *>(malloc(sizeof(numaSchedInit_t)*NUMA_WORKERS));
  int pseudoRandomNumber = 1;
  mrmw_queue_t * workQueue =
  for (int i = 0; i < NUMA_WORKERS; i++) {
    numaSchedInit[i].coreID = i;
    numaSchedInit[i].numRounds = numRounds;
    numaSchedInit[i].numPhases = NUM_PHASES;
    pseudoRandomNumber += 3;
    if (pseudoRandomNumber > NUMA_WORKERS) {
      pseudoRandomNumber -=  NUMA_WORKERS;
    }
    numaSchedInit[i].pseudoRandomNumber = pseudoRandomNumber;
    numaSchedInit[i].numChunksPerWorker = 
    numaSchedInit[i].cntNodes = cntNodes;
    numaSchedInit[i].nodes = nodes;
    numaSchedInit[i].scheddata = scheddata;
    numaSchedInit[i].globaldata = globaldata;
    numaSchedInit[i].mrmw_queue_t = 
  }
  int coreID;  //  this worker needs to know which core she is
  int numRounds;  //  total number of rounds to perform
  int numPhases;  //  number of phases per round
  int pseudoRandomNumber;  //  a number in range (0,numCores)
  vid_t numChunksPerWorker;
  vid_t cntNodes;
  vertex_t * nodes;
  scheddata_t * scheddata;
  global_t * globaldata;
  mrmw_queue_t * workQueue;
  volatile int * remainingChunks;
  volatile int * phase;
};


  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  for (int round = 0; round < numRounds; ++round) {
    WHEN_DEBUG({
      cout << "Running chunk round" << round << endl;
    })

    for (vid_t i = 0; i < scheddata->cntChunks; i++) {
      scheddata->chunkdata[i].nextIndex = i << CHUNK_BITS;
    }

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
