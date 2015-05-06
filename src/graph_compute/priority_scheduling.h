#ifndef PRIORITY_SCHEDULING_H_
#define PRIORITY_SCHEDULING_H_

#if D1_PRIO || BASELINE

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

// we need this processNodeSerial hack to avoid a Cilk bug when trying to do
// spawn depth limiting (which in turn is necessary because otherwise on large problem
// sizes worker stacks are overflowed)
static void processNodeSerial(vertex_t * nodes, const vid_t index, const vid_t cntNodes) {
  update(nodes, index);
  vertex_t * current = &nodes[index];

  // increment the dependencies for all nodes of greater priority
  for (size_t i = 0; i < current->cntEdges; ++i) {
    vid_t neighborId = current->edges[i];
    vertex_t * neighbor = &nodes[neighborId];
    if (neighbor->priority > current->priority) {
      size_t newSatisfied = __sync_add_and_fetch(&neighbor->satisfied, 1);
      if (newSatisfied == neighbor->dependencies) {
        processNodeSerial(nodes, neighborId, cntNodes);
      }
    }
  }
}

static void processNode(vertex_t * nodes, const vid_t index, const vid_t cntNodes,
  const int depth) {
  update(nodes, index);
  vertex_t * current = &nodes[index];

  // increment the dependencies for all nodes of greater priority
  for (size_t i = 0; i < current->cntEdges; ++i) {
    vid_t neighborId = current->edges[i];
    vertex_t * neighbor = &nodes[neighborId];
    if (neighbor->priority > current->priority) {
      size_t newSatisfied = __sync_add_and_fetch(&neighbor->satisfied, 1);
      if (newSatisfied == neighbor->dependencies) {
        if (depth < MAX_REC_DEPTH) {
          cilk_spawn processNode(nodes, neighborId, cntNodes, depth + 1);
        } else {
          processNodeSerial(nodes, neighborId, cntNodes);
        }
        // cilk_spawn processNode(nodes, neighborId, cntNodes);
      }
    } else {
      i = current->cntEdges;
    }
  }
  cilk_sync;
}

static void calculateNodeDependencies(vertex_t * nodes, const vid_t cntNodes) {
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    vertex_t * node = &nodes[i];
    node->dependencies = 0;
    for (size_t j = 0; j < node->cntEdges; ++j) {
      vertex_t * neighbor = &nodes[node->edges[j]];
      if (node->priority > neighbor->priority) {
        ++node->dependencies;
      }
    }
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

static void assignNodePriorities(vertex_t * nodes, const vid_t cntNodes,
                                 const int bitsInId) {
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    nodes[i].priority = createPriority(nodes[i].id, bitsInId);

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

static void init_scheduling(vertex_t * nodes, const vid_t cntNodes) {
  int bitsInId = calculateIdBitSize(cntNodes);
  WHEN_DEBUG({ cout << "Bits in ID: " << bitsInId << '\n'; })
  assignNodePriorities(nodes, cntNodes, bitsInId);
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    std::partition(nodes[i].edges, nodes[i].edges + nodes[i].cntEdges,
      [nodes, i](const vid_t& val) {return (nodes[i].priority < nodes[val].priority);});
  }
  calculateNodeDependencies(nodes, cntNodes);
}

static void execute_round(const int round, vertex_t * nodes, const vid_t cntNodes) {
  WHEN_DEBUG({
    cout << "Running d1 prio round " << round << endl;
  })

  vector<vid_t> roots;
  for (vid_t i = 0; i < cntNodes; ++i) {
    if (nodes[i].dependencies == 0) {
      roots.push_back(i);
    }
  }

  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    nodes[i].satisfied = 0;
  }

  cilk_for (vid_t i = 0; i < roots.size(); ++i) {
    processNode(nodes, roots[i], cntNodes, 0);
  }
}

static void print_execution_data() {
  cout << "Priority group bits: " << PRIORITY_GROUP_BITS << '\n';
}

#endif  // D1_PRIO || BASELINE

#endif  // PRIORITY_SCHEDULING_H_
