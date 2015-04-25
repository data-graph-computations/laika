#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <ctime>
#include <string>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <exception>
#include "./common.h"
#include "./io.h"

using namespace std;

#ifndef BASELINE
  #define BASELINE 0
#endif

#if BASELINE
  #ifndef PRIORITY_GROUP_BITS
    #define PRIORITY_GROUP_BITS 0
  #endif
#endif

#ifndef PRIORITY_GROUP_BITS
  #define PRIORITY_GROUP_BITS 8
#endif

#ifndef PARALLEL
  #define PARALLEL 0
#endif

#if PARALLEL
  #include <cilk/cilk.h>
#else
  #define cilk_for for
  #define cilk_spawn
  #define cilk_sync
#endif

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

static void assignNodePriorities(vertex_t * nodes, const int cntNodes,
                                 const int bitsInId) {
  cilk_for (int i = 0; i < cntNodes; ++i) {
    nodes[i].priority = createPriority(nodes[i].id, bitsInId);

    WHEN_DEBUG({
      cout << "Node ID " << nodes[i].id
           << " got priority " << nodes[i].priority << '\n';
    })
  }

  WHEN_TEST({
    // ensure no two nodes have the same ID or priority
    for (int i = 0; i < cntNodes; ++i) {
      for (int j = i + 1; j < cntNodes; ++j) {
        assert(nodes[i].id != nodes[j].id);
        assert(nodes[i].priority != nodes[j].priority);
      }
    }
  })
}

// fill in each node with random-looking data
static void fillInNodeData(vertex_t * nodes, const int cntNodes) {
  cilk_for (int i = 0; i < cntNodes; ++i) {
    nodes[i].data = nodes[i].priority + ((-nodes[i].id) ^ (nodes[i].dependencies * 31));
  }
}

static void calculateNodeDependencies(vertex_t * nodes, const int cntNodes) {
  cilk_for (int i = 0; i < cntNodes; ++i) {
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

static void processNode(vertex_t * nodes, const int index, const int cntNodes) {
  vertex_t * current = &nodes[index];
  if (current->satisfied == current->dependencies) {
    // recalculate this node's data
    double data = current->data;
    for (size_t i = 0; i < current->cntEdges; ++i) {
      data += nodes[current->edges[i]].data;
    }
    data /= (current->cntEdges + 1);
    current->data = data;

    // increment the dependencies for all nodes of greater priority
    // TODO(predrag): consider sorting the edges by priority
    //                so we don't have to check all of them at every step
    for (size_t i = 0; i < current->cntEdges; ++i) {
      vid_t neighborId = current->edges[i];
      vertex_t * neighbor = &nodes[neighborId];
      if (neighbor->priority > current->priority) {
        size_t newSatisfied = __sync_add_and_fetch(&neighbor->satisfied, 1);
        if (newSatisfied == neighbor->dependencies) {
          processNode(nodes, neighborId, cntNodes);
        }
      }
    }
  }
}

static void runRound(const int round, vertex_t * nodes, const int cntNodes) {
  WHEN_DEBUG({
    cout << "Running round " << round << endl;
  })

  cilk_for (int i = 0; i < cntNodes; ++i) {
    nodes[i].satisfied = 0;
  }

  cilk_for (int i = 0; i < cntNodes; ++i) {
    processNode(nodes, i, cntNodes);
  }
}

int main(int argc, char *argv[]) {
  vertex_t * nodes;
  int cntNodes;
  char * inputEdgeFile;
  int numRounds = 0;

  if (argc != 3) {
    cerr << "\nERROR: Expected 2 arguments, received " << argc-1 << '\n';
    cerr << "Usage: ./compute <num_rounds> <input_edges>" << endl;
    return 1;
  }

  try {
    numRounds = stoi(argv[1]);
  } catch (exception& e) {
    cerr << "\nERROR: " << e.what() << endl;
    return 1;
  }

  inputEdgeFile = argv[2];

  cout << "Input edge file: " << inputEdgeFile << '\n';

  int result = readEdgesFromFile(inputEdgeFile, &nodes, &cntNodes);
  assert(result == 0);

  cout << "Graph size: " << cntNodes << '\n';

  int bitsInId = calculateIdBitSize(cntNodes);
  WHEN_DEBUG({ cout << "Bits in ID: " << bitsInId << '\n'; })
  assignNodePriorities(nodes, cntNodes, bitsInId);
  calculateNodeDependencies(nodes, cntNodes);

  // our nodes don't have any real data associated with them
  // generate some fake data instead
  fillInNodeData(nodes, cntNodes);

  struct timespec starttime, endtime;
  result = clock_gettime(CLOCK_MONOTONIC, &starttime);
  assert(result == 0);

  // suppress fake GCC warning, seems to be a bug in GCC 4.8/4.9
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  for (int i = 0; i < numRounds; ++i) {
    runRound(i, nodes, cntNodes);
  }

  result = clock_gettime(CLOCK_MONOTONIC, &endtime);
  assert(result == 0);
  int64_t ns = endtime.tv_nsec;
  ns -= starttime.tv_nsec;
  double seconds = static_cast<double>(ns) * 1e-9;
  seconds += endtime.tv_sec - starttime.tv_sec;

  cout << "Done computing " << numRounds << " rounds!\n";
  cout << "Time taken:     " << setprecision(8) << seconds << "s\n";
  cout << "Time per round: " << setprecision(8) << seconds / numRounds << "s\n";
  cout << "Baseline: " << BASELINE << '\n';
  cout << "Parallel: " << PARALLEL << '\n';
  cout << "Priority group bits: " << PRIORITY_GROUP_BITS << '\n';
  cout << "Debug flag: " << DEBUG << '\n';
  cout << "Test flag: " << TEST << '\n';

  // so GCC doesn't eliminate the rounds loop as unnecessary work
  double data = 0.0;
  for (int i = 0; i < cntNodes; ++i) {
    data += nodes[i].data;
  }
  cout << "Final result (ignore this line): " << data << '\n';

  return 0;
}
