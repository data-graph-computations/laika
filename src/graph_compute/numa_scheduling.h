#ifndef NUMA_SCHEDULING_H_
#define NUMA_SCHEDULING_H_

#if D1_NUMA

#if !MPI_OFF
#include <mpi.h>
#endif

#include <algorithm>
#include <unordered_set>
#include <set>
#include <unordered_map>
#include "./common.h"
#include "./concurrent_queue.h"
#include "./numa_init.h"

#if CHUNK_BITS < 1
  #error "CHUNK_BITS needs to be greater than 0 for D1_NUMA"
#endif

#if NUMA_WORKERS < 1
  #error "NUMA_WORKERS needs to be greater than 0 for D1_NUMA"
#endif

#define PHASES 2

//  there is one of these per vertex
struct sched_t {
  vid_t * dependentEdges;  //  pointer into dependentEdges array in scheddata_t
  vid_t cntDependentEdges;  //  number of dependentEdges for this vertex
  vid_t dependencies;
  volatile vid_t satisfied;
};
typedef struct sched_t sched_t;

// update_function.h depends on sched_t being defined
#include "./update_function.h"

// there is one of these per chunk
struct chunkdata_t {
  //  the index of the first vertex beyond this chunk
  //  this could be generalized to an arbitrary number of phases, but is not currently
  vid_t phaseEndIndex[2];
  vid_t workQueueNumber;
  volatile vid_t nextIndex;  // the next vertex in this chunk to be processed
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
  volatile int * remainingStragglers;
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
  int rounds;
  int numRanks;
  int myMpiId;
  vid_t totalChunks;
  vid_t startChunk;
  vid_t myChunks;
  vid_t chunksPerRank;
  vid_t numExpectedDeqs0;
  vid_t numExpectedDeqs1;
  vid_t numExpectedRcvs0;
  vid_t numExpectedRcvs1;
};
typedef struct scheddata_t scheddata_t;

typedef struct {
  scheddata_t *scheddata;
  vertex_t *nodes;
} net_worker_data_t;

#define Q_SIZE (1 << 16)
#define Q_MASK(p) ((p) & (Q_SIZE - 1))
volatile vid_t netQ[Q_SIZE];
volatile uint64_t h = 0, t = 0;
pthread_t netWorker;
sem_t net_sem;

vid_t *rnd_deqs;
vid_t *rnd_rcvs;
vid_t maxRemoteEdges = 0;
unordered_map<vid_t, data_t> remoteData;
mpi_data_t *sched_mpi;

void *net(void *nwd);

template<typename T>
inline T logBaseTwoRoundUp(T x) {
  T power = 0;
  static const T one = 1;
  while ((one << power) < x) {
    power++;
  }
  return power;
}

__attribute__((target("sse4.2")))
inline uint32_t randomValue(const uint32_t seed,
                            const unsigned int randVal = 0xF1807D63) {
  return _mm_crc32_u32(randVal, seed);
}

static inline bool samePhase(vid_t v, vid_t w, chunkdata_t * const chunkdata) {
  // this assumes that the boundary between phases is the
  // midpoint of the chunk - we will generalize later
  // to a custom boundary to optimize overall work
  bool vPhase = v < chunkdata[v >> CHUNK_BITS].phaseEndIndex[0];
  bool wPhase = w < chunkdata[w >> CHUNK_BITS].phaseEndIndex[0];
  return (vPhase == wPhase);
}

static inline int getPhase(vid_t v) {
  static const vid_t chunkMask = (1 << CHUNK_BITS) - 1;
  return ((v & chunkMask) < (1 << (CHUNK_BITS - 1))) ? 0 : 1;
}

static inline bool chunkCrossing(vid_t v, vid_t w) {
  return (v >> CHUNK_BITS) != (w >> CHUNK_BITS);
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

static bool isLocalNode(vid_t i, scheddata_t * const scheddata) {
  return i >= (scheddata->startChunk << CHUNK_BITS) && i < ((scheddata->startChunk + scheddata->myChunks) << CHUNK_BITS);
}

static int getProcForNode(vid_t i, scheddata_t * const scheddata) {
  return std::min((i >> CHUNK_BITS) / scheddata->chunksPerRank,
    (vid_t) (scheddata->numRanks - 1));
}

static inline void calculateNeighborhood(std::unordered_set<vid_t> * neighbors,
                                         std::unordered_set<vid_t> * oldNeighbors,
                                         vid_t v,
                                         vertex_t * const nodes,
                                         vid_t distance,
                                         scheddata_t * const scheddata) {
  neighbors->clear();
  neighbors->insert(v);
  oldNeighbors->clear();
  for (vid_t d = 0; d < distance; d++) {
    *oldNeighbors = *neighbors;
    for (const auto& v : *oldNeighbors) {
      for (vid_t j = 0; j < nodes[v].cntEdges; j++) {
        if (isLocalNode(nodes[v].edges[j], scheddata) && oldNeighbors->count(nodes[v].edges[j]) == 0) {
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
    calculateNeighborhood(&neighbors, &oldNeighbors, i, nodes, DISTANCE, scheddata);
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
  numaInit_t numaInit(NUMA_WORKERS,
                      CHUNK_BITS, static_cast<bool>(NUMA_INIT));
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

static void orderEdgesByChunk(vertex_t * const nodes, const vid_t cntNodes,
  scheddata_t * const scheddata) {
  std::set<vid_t> rcvNodes0;
  std::set<vid_t> rcvNodes1;
  scheddata->numExpectedDeqs0 = 0;
  scheddata->numExpectedDeqs1 = 0;
  for (vid_t i = 0; i < cntNodes; ++i) {
    vid_t *interEnd = std::stable_partition(nodes[i].edges,
      nodes[i].edges + nodes[i].cntEdges,
      [i](const vid_t& val) {return chunkCrossing(i, val);});
    vid_t *remoteEnd = std::stable_partition(nodes[i].edges, interEnd,
      [scheddata](const vid_t& val) {return !isLocalNode(val, scheddata);});
    vid_t numRemotEdges = (vid_t) (remoteEnd - nodes[i].edges);
    if (numRemoteEdges > maxRemoteEdges) {
      maxRemoteEdges = numRemoteEdges;
    }
    std::sort(nodes[i].edges, remoteEnd);
    if (remoteEnd != nodes[i].edges) {
      if (getPhase(i) == 0) {
        scheddata->numExpectedDeqs0++;
      } else {
        scheddata->numExpectedDeqs1++;
      }
    }
    vid_t *e = nodes[i].edges;
    while (e < remoteEnd) {
      if (getPhase(*e) == 0) {
        rcvNodes0.insert(*e);
      } else {
        rcvNodes1.insert(*e);
      }
      e++;
    }
  }
  scheddata->numExpectedRcvs0 = rcvNodes0.size();
  scheddata->numExpectedRcvs1 = rcvNodes1.size();
}

static void relabelEdges(vertex_t * const nodes, const vid_t cntNodes,
  scheddata_t * const scheddata) {
  for (vid_t i = 0; i < cntNodes; i++) {
    for (vid_t j = 0; j < nodes[i].cntEdges; j++) {   
      if (isLocalNode(nodes[i].edges[j], scheddata)) {
        nodes[i].edges[j] -= (scheddata->startChunks << CHUNK_BITS);
      }
    }
  }
}

static inline void createChunkData(vertex_t * const nodes,
                                   const vid_t cntNodes,
                                   scheddata_t * const scheddata) {
  scheddata->cntChunks = (cntNodes + (1 << CHUNK_BITS) - 1) >> CHUNK_BITS;
  scheddata->chunkdata = new (std::nothrow) chunkdata_t[scheddata->cntChunks]();
  assert(scheddata->chunkdata != NULL);
  scheddata->numChunksPerWorker =
    (scheddata->cntChunks + NUMA_WORKERS - 1) / NUMA_WORKERS;

  cilk_for (vid_t i = 0; i < scheddata->cntChunks; ++i) {
    chunkdata_t * chunk = &scheddata->chunkdata[i];
    chunk->nextIndex = i << CHUNK_BITS;
    chunk->phaseEndIndex[0] = std::min(chunk->nextIndex + (1 << (CHUNK_BITS - 1)),
      cntNodes);
    chunk->phaseEndIndex[1] = std::min((i + 1) << CHUNK_BITS, cntNodes);
    chunk->workQueueNumber = i / scheddata->numChunksPerWorker;
    WHEN_TEST(
      assert(chunk->workQueueNumber < NUMA_WORKERS);
    )
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
  vid_t logChunksPerWorker = logBaseTwoRoundUp<vid_t>((cntNodes >> CHUNK_BITS) + 1);
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

// this queue can only hold size - 1 entries since we are never allowed to
// enqueue in the position that t points to
// blocking operation, multiple producer
static void enQ(const vid_t i) {
  vid_t reserve = __sync_fetch_and_add(&h, 1);
  // TODO(jjthomas) might eliminate this loop if we can't fill
  while (reserve >= t + Q_SIZE) {}
  netQ[Q_MASK(reserve)] = i;
}

// nonblocking, single consumer
static vid_t deQ() {
  if (t == h) {  // formerly while loop on this condition
    return UINT64_MAX;
  }
  while (netQ[Q_MASK(t)] == UINT64_MAX) {}
  vid_t rVal = netQ[Q_MASK(t)];
  netQ[Q_MASK(t)] = UINT64_MAX;
  t++;
  return rVal;
}

#if !MPI_OFF
// call from single thread
static void broadcast(vertex_t *nodes, scheddata_t *scheddata, const vid_t i,
                      const int round) {
  static size_t msg_len = sizeof(net_vertex_t) + sizeof(vid_t) * maxRemoteEdges;
  static net_vertex_t *nvt = (net_vertex_t *) malloc(msg_len);
  static vid_t *edgeBuffer = (vid_t *)(nvt + 1);
  vid_t k = 0;
  nvt->id = i;
  nvt->data = nodes[i].data;
  nvt->round = round;
  int lastProc = -1;
  vid_t lastIndex = 0;
  while (k < nodes[i].cntEdges) {
    if (!isLocalNode(nodes[i].edges[k], scheddata)) {
      int proc = getProcForNode(nodes[i].edges[k], scheddata);
      if (proc != lastProc) {
        if (lastProc != -1) {
          nvt->cntEdges = k - lastIndex;
          memcpy(edgeBuffer, nodes[i].edges + lastIndex, sizeof(vid_t) * nvt->cntEdges);
          MPI_Send(nvt, msg_len, MPI_BYTE, lastProc, 0, MPI_COMM_WORLD);
        }
        lastProc = proc;
        lastIndex = k;
       }
    } else {
      break;
     }
    k++;
  }
  if (k > 0) {
    nvt->cntEdges = k - lastIndex;
    memcpy(edgeBuffer, nodes[i].edges + lastIndex, sizeof(vid_t) * nvt->cntEdges);
    MPI_Send(nvt, msg_len, MPI_BYTE, lastProc, 0, MPI_COMM_WORLD);   
  }
}

static void receive(vertex_t *nodes, scheddata_t *scheddata, net_vertex_t *nvt) {
  remoteData[nvt->id] = nvt->data;
  vid_t *edgeBuffer = (vid_t *)(nvt + 1);
  vid_t k = 0;
  while (k < nvt->cntEdges) {
    if (interChunkDependency(nvt->id, edgeBuffer[k])) {
      vid_t adjusted = edgeBuffer[k] - (scheddata->startChunks << CHUNK_BITS);
      if (__sync_sub_and_fetch(&nodes[adjusted].sched->satisfied, 1) == SENTINEL) {
        //  Released this chunk, so push it on its home work queue
        vid_t enabledChunk = adjusted >> CHUNK_BITS;
        vid_t queueNumber =
          scheddata->chunkdata[enabledChunk].workQueueNumber;
        numaSchedInit[queueNumber].workQueue->push(enabledChunk);
      }
    }
    k++;
  }
}
 
void *net(void *nwd) {
  net_worker_data_t *data = reinterpret_cast<net_worker_data_t *>(nwd);
  scheddata_t *scheddata = data->scheddata;
  vertex_t *nodes = data->nodes;
  size_t msg_len = sizeof(net_vertex_t) + sizeof(vid_t) * maxRemoteEdges;
  net_vertex_t *nvt = (net_vertex_t *) malloc(msg_len);
  
  MPI_Request req;
  vid_t deqed;

  for (int r = 0; r < PHASES * scheddata->rounds; r++) {
    bool initRcv = true;
    int testFlag;
    while (rnd_deqs[r] < scheddata->numExpectedDeqs ||
           rnd_rcvs[r] < scheddata->numExpectedRcvs) {
      if (rnd_deqs[r] < scheddata->numExpectedDeqs && (deqed = deQ()) != UINT64_MAX) {
        broadcast(nodes, scheddata, deqed, r);
        rnd_deqs[r]++;
      }
      if (rnd_rcvs[r] < scheddata->numExpectedRcvs) {
        if (initRcv) {
          MPI_Irecv(nvt, msg_len, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG,
            MPI_COMM_WORLD, &req);
          initRcv = false;
        } else {
          MPI_Test(&req, &testFlag, MPI_STATUS_IGNORE);
          if (testFlag) {
            receive(nodes, scheddata, nvt);
            rnd_rcvs[nvt->round]++;
            initRcv = true;
          }
        }
      }
    }
    // MPI_Barrier(MPI_COMM_WORLD);
    sem_post(&net_sem);
  }
  return NULL;
 }
#endif

static inline void init_scheduling(vertex_t * const nodes,
                                   const vid_t cntNodes,
                                   scheddata_t * const scheddata,
                                   mpi_data_t *mpi,
                                   int argc, char **argv) {

  scheddata->rounds = rounds;

#if !MPI_OFF
  sem_init(&net_sem, 0, 0);
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &scheddata->numRanks);
  MPI_Comm_rank(MPI_COMM_WORLD, &scheddata->myMpiId);
#else
  scheddata->numRanks = 1;
  scheddata->myMpiId = 0;
#endif
  vid_t totalNodes = atol(argv[1]);
  scheddata->totalChunks = (totalNodes + (1 << CHUNK_BITS) - 1) >> CHUNK_BITS;
  scheddata->chunksPerRank = scheddata->totalChunks / scheddata->numRanks;
  scheddata->startChunk = scheddata->myMpiId * scheddata->chunksPerRank;
  scheddata->myChunks = (scheddata->myMpiId == scheddata->numMpiRanks - 1) ?
    totalChunks - scheddata->startChunk : scheddata->chunksPerRank;
  mpi->local_start = scheddata->startChunk << CHUNK_BITS;
  mpi->local_end = (scheddata->startChunk + scheddata->myChunks) << CHUNK_BITS;
  mpi->data = &remoteData;

  createChunkData(nodes, cntNodes, scheddata);
  relabelEdges(nodes, cntNodes, scheddata);
  orderEdgesByChunk(nodes, cntNodes, scheddata);
  calculateNodeDependenciesChunk(nodes, cntNodes, scheddata);
  populateWorkerParameters(nodes, cntNodes, scheddata);

  for (vid_t i = 0; i < Q_SIZE; i++) {
    netQ[i] = UINT64_MAX;
  }

  rnd_deqs = new vid_t[PHASES * rounds];
  rnd_rcvs = new vid_t[PHASES * rounds];
  for (int i = 0; i < PHASES * rounds; i++) {
    rnd_deqs[i] = 0;
    rnd_rcvs[i] = 0;
  }

  MPI_Barrier(MPI_COMM_WORLD);  // sync everyone up
  net_worker_data_t nwd = { .scheddata = scheddata, .nodes = nodes };
  pthread_create(&netWorker, NULL, net, &nwd);
}

inline void * processChunks(void * param) {
  numaSchedInit_t * config = static_cast<numaSchedInit_t *>(param);
  scheddata_t * scheddata = config->scheddata;
  numaSchedInit_t * numaSchedInit = scheddata->numaSchedInit;

  // Initialize the chunk
  bindThreadToCore(config->coreID);
  static const vid_t SENTINEL = static_cast<vid_t>(-1);

  vid_t stealQueueNumber = config->coreID;
  uint32_t seed = static_cast<uint32_t>(config->coreID) + 1;
  seed *= static_cast<uint32_t>(numaSchedInit->cntNodes);
  static const uint32_t queueNumberMask
    = (1 << logBaseTwoRoundUp<uint32_t>(NUMA_WORKERS)) - 1;

  for (int round = 0; round < config->numRounds; round++) {
    for (int phase = 0; phase < config->numPhases; phase++) {
      //  load up chunks that belong to me
      __sync_sub_and_fetch(config->remainingStragglers, 1);
      //  wait until all workers have reached the barrier
      while (static_cast<int>(*config->remainingStragglers) > 0) {}
      vid_t start = config->coreID*scheddata->numChunksPerWorker;
      vid_t end = (config->coreID + 1)*scheddata->numChunksPerWorker;
      end = std::min(end, scheddata->cntChunks);
      config->workQueue->push(start, end);
      //  start with your own queue - you just put stuff in it
      stealQueueNumber = config->coreID;
      //  When somebody completes the last chunk, they'll reset
      //  the config->remainingStragglers variable.
      while (static_cast<int>(*config->remainingStragglers) == 0) {
        vid_t chunk;
      #if NUMA_STEAL
        //  try to get a chunk from my own queue
        chunk = numaSchedInit[config->coreID].workQueue->pop();
      #else
        chunk = SENTINEL;
      #endif
        while ((chunk == SENTINEL)
               && (static_cast<int>(*config->remainingStragglers) == 0)) {
          //  randomly steal
          chunk = numaSchedInit[stealQueueNumber].workQueue->pop();
          if (chunk == SENTINEL) {
            seed = randomValue(seed) + 1;
            stealQueueNumber = seed & queueNumberMask;
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
            chunkdata_t * chunkdata = &scheddata->chunkdata[chunk];
            if (chunkdata->nextIndex < chunkdata->phaseEndIndex[phase]) {
              sched_t * const node = &config->nodes[chunkdata->nextIndex].sched;
              if ((DISTANCE > 0)
                  && (static_cast<vid_t>(node->satisfied) != SENTINEL)
                  && (static_cast<vid_t>(node->satisfied) > 0)
                  && (__sync_sub_and_fetch(&node->satisfied, 1) != SENTINEL)) {
                //  If we need to shelve vertex, we're done with the chunk
                doneFlag = true;
              } else {
                //  otherwise we process the vertex and decrement those
                //  vertices dependent on it
                // TODO update this to use remoteData if necessary
                update(config->nodes, chunkdata->nextIndex, config->globaldata, round, sched_mpi);
                if (DISTANCE > 0) {
                  node->satisfied = node->dependencies;
                  vid_t j = chunkdata->nextIndex;
                  if (config->nodes[j].cntEdges > 0 && !isLocalNode(config->nodes[j].edges[0], scheddata)) {
                    enQ(j);
                  }
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
                //  so, reset the remainingChunks, remainingStragglers, and
                //  remainingPushes counters
              #if !MPI_OFF
                sem_wait(&net_sem);
              #endif
                *config->remainingStragglers = NUMA_WORKERS;
                *config->remainingChunks = scheddata->cntChunks;
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
                                  global_t * const globaldata,
                                  mpi_data_t *mpi) {
  //  the shared variable for tracking remaining chunks during a phase
  volatile int remainingChunks = scheddata->cntChunks;
  //  the counter that tells workers how many remaining
  //  workers need to report before pushing
  volatile int remainingStragglers = NUMA_WORKERS;
  sched_mpi = mpi;

  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  for (int i = 0; i < NUMA_WORKERS; i++) {
    scheddata->numaSchedInit[i].globaldata = globaldata;
    scheddata->numaSchedInit[i].numRounds = numRounds;
    scheddata->numaSchedInit[i].remainingChunks = &remainingChunks;
    scheddata->numaSchedInit[i].remainingStragglers = &remainingStragglers;
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
  delete[] scheddata->numaSchedInit;
  delete[] scheddata->queueData;

#if !MPI_OFF
  pthread_join(netWorker, NULL);
  delete rnd_deqs;
  delete rnd_rcvs;
  MPI_Finalize();
#endif
}

static inline void print_execution_data() {
  cout << "Chunk size bits: " << CHUNK_BITS << '\n';
  cout << "Number of workers: " << NUMA_WORKERS << '\n';
}

#endif  // D1_NUMA

#endif  // NUMA_SCHEDULING_H_
