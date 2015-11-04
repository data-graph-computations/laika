#ifndef PRIORITY_SCHEDULING_H_
#define PRIORITY_SCHEDULING_H_

#if D1_PRIO

#include <vector>
#include <algorithm>
#include "./common.h"
#include "./update_function.h"

#if BASELINE
  #ifndef PRIORITY_GROUP_BITS
    #define PRIORITY_GROUP_BITS 0
  #endif
#endif

#ifndef PRIORITY_GROUP_BITS
  #define PRIORITY_GROUP_BITS 8
#endif

#define MAX_REC_DEPTH 10

struct scheddata_t {
  vid_t * roots;
  vid_t cntRoots;
};
typedef struct scheddata_t scheddata_t;

struct sched_t {
  vid_t id;
  vid_t priority;
  vid_t dependencies;
  volatile vid_t satisfied;
};
typedef struct sched_t sched_t;

// we need this processNodeSerial hack to avoid a Cilk bug when trying to do
// spawn depth limiting (which in turn is necessary because otherwise on large problem
// sizes worker stacks are overflowed)
static void processNodeSerial(vertex_t * const nodes, const vid_t index,
                              const vid_t cntNodes, const int round) {
  update(nodes, index, round);
  vertex_t * current = &nodes[index];

  // increment the dependencies for all nodes of greater priority
  for (size_t i = 0; i < current->cntEdges; ++i) {
    vid_t neighborId = current->edges[i];
    sched_t * neighbor = &nodes[neighborId].sched;
    if (neighbor->priority > current->sched.priority) {
      if (__sync_sub_and_fetch(&neighbor->satisfied, 1) == 0) {
        neighbor->satisfied = neighbor->dependencies;
        processNodeSerial(nodes, neighborId, cntNodes, round);
      }
    }
  }
}

static void processNode(vertex_t * const nodes, const vid_t index, const vid_t cntNodes,
                        const int depth, const int round) {
  update(nodes, index, round);
  vertex_t * current = &nodes[index];

  // increment the dependencies for all nodes of greater priority
  for (vid_t i = 0; i < current->cntEdges; ++i) {
    vid_t neighborId = current->edges[i];
    sched_t * neighbor = &nodes[neighborId].sched;
    if (neighbor->priority > current->sched.priority) {
      if (__sync_sub_and_fetch(&neighbor->satisfied, 1) == 0) {
        neighbor->satisfied = neighbor->dependencies;
        if (depth < MAX_REC_DEPTH) {
          cilk_spawn processNode(nodes, neighborId, cntNodes, depth + 1, round);
        } else {
          processNodeSerial(nodes, neighborId, cntNodes, round);
        }
      }
    } else {
      break;
    }
  }
  cilk_sync;
}

static void calculateNodeDependencies(vertex_t * const nodes, const vid_t cntNodes) {
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    nodes[i].sched.id = i;
    nodes[i].sched.dependencies = 0;
    for (vid_t j = 0; j < nodes[i].cntEdges; ++j) {
      sched_t * neighbor = &nodes[nodes[i].edges[j]].sched;
      if (nodes[i].sched.priority > neighbor->priority) {
        ++nodes[i].sched.dependencies;
      }
    }
    nodes[i].sched.satisfied = nodes[i].sched.dependencies;
  }
}

static int calculateIdBitSize(const uint64_t cntNodes) {
  uint64_t numBits = cntNodes - 1;
  numBits |= numBits >> 1;
  numBits |= numBits >> 2;
  numBits |= numBits >> 4;
  numBits |= numBits >> 8;
  numBits |= numBits >> 16;
  numBits |= numBits >> 32;
  return __builtin_popcountl(numBits);
}

static inline id_t createPriority(const vid_t id, const int bitsInId) {
  vid_t priority;

  if (bitsInId <= PRIORITY_GROUP_BITS) {
    return id;
  }

  // mask off the low ID bits that are not the priority group bits
  // and move them to the top of the priority
  vid_t orderMask = (1 << (bitsInId - PRIORITY_GROUP_BITS)) - 1;
  priority = (id & orderMask) << PRIORITY_GROUP_BITS;

  // add the priority group bits at the bottom of the priority
  priority |= id >> (bitsInId - PRIORITY_GROUP_BITS);
  return priority;
}

static void assignNodePriorities(vertex_t * const nodes, const vid_t cntNodes,
                                 const int bitsInId) {
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    nodes[i].sched.priority = createPriority(nodes[i].sched.id, bitsInId);

    WHEN_DEBUG({
      cout << "Node ID " << nodes[i].id
           << " got priority " << nodes[i].priority << '\n';
    })
  }

  WHEN_TEST({
    // ensure no two nodes have the same ID or priority
    for (vid_t i = 0; i < cntNodes; ++i) {
      for (vid_t j = i + 1; j < cntNodes; ++j) {
        assert(nodes[i].id != nodes[j].id);
        assert(nodes[i].priority != nodes[j].priority);
      }
    }
  })
}

// for each vertex, move its successors (by priority) to the front of the edges list
static void orderEdgesByPriority(vertex_t * const nodes, const vid_t cntNodes) {
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    std::stable_partition(nodes[i].edges, nodes[i].edges + nodes[i].cntEdges,
      [nodes, i](const vid_t& val) {
        return (nodes[i].sched.priority < nodes[val].sched.priority);
      });
  }
}

static void findRoots(vertex_t * const nodes, const vid_t cntNodes,
                      scheddata_t * const scheddata) {
  scheddata->cntRoots = 0;
  for (vid_t i = 0; i < cntNodes; ++i) {
    if (nodes[i].sched.dependencies == 0) {
      ++scheddata->cntRoots;
    }
  }

  scheddata->roots = new (std::nothrow) vid_t[scheddata->cntRoots];
  assert(scheddata->roots != NULL);

  vid_t position = 0;
  for (vid_t i = 0; i < cntNodes; ++i) {
    if (nodes[i].sched.dependencies == 0) {
      scheddata->roots[position++] = i;
    }
  }
}

static void init_scheduling(vertex_t * const nodes, const vid_t cntNodes,
                            scheddata_t * const scheddata) {
  int bitsInId = calculateIdBitSize(cntNodes);
  WHEN_DEBUG({ cout << "Bits in ID: " << bitsInId << '\n'; })
  assignNodePriorities(nodes, cntNodes, bitsInId);
  orderEdgesByPriority(nodes, cntNodes);
  calculateNodeDependencies(nodes, cntNodes);
  findRoots(nodes, cntNodes, scheddata);
}

static void execute_round(const int numRounds, vertex_t * const nodes,
                          const vid_t cntNodes,
                          scheddata_t * const scheddata) {
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  for (int round = 0; round < numRounds; ++round) {
    WHEN_DEBUG({
      cout << "Running d1 prio round " << round << endl;
    })
    cilk_for (vid_t i = 0; i < scheddata->cntRoots; ++i) {
      processNode(nodes, scheddata->roots[i], cntNodes, 0, round);
    }
  }
}

static void cleanup_scheduling(vertex_t * const nodes, const vid_t cntNodes,
                               scheddata_t * const scheddata) {
  delete[] scheddata->roots;
}

static void print_execution_data() {
  cout << "Priority group bits: " << PRIORITY_GROUP_BITS << '\n';
}

#endif  // D1_PRIO

#endif  // PRIORITY_SCHEDULING_H_
