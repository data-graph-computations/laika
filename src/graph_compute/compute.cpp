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
#include <vector>
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

#ifndef CHUNK_BITS
  #define CHUNK_BITS 16
#endif

#ifndef PARALLEL
  #define PARALLEL 0
#endif

#ifndef D0_BSP
  #define D0_BSP 0
#endif

#ifndef D1_PRIO
  #define D1_PRIO 0
#endif

#ifndef D1_CHUNK
  #define D1_CHUNK 0
#endif

#if PARALLEL
  #include <cilk/cilk.h>
#else
  #define cilk_for for
  #define cilk_spawn
  #define cilk_sync
#endif

WHEN_TEST(
  static uint64_t roundUpdateCount = 0;
)
#define MAX_REC_DEPTH 10

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

// fill in each node with random-looking data
static void fillInNodeData(vertex_t * nodes, const vid_t cntNodes) {
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    nodes[i].data = nodes[i].priority + ((-nodes[i].id) ^ (nodes[i].dependencies * 31));
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

static void update(vertex_t * nodes, const vid_t index) {
  // ensuring that the number of updates in total is correct per round
  WHEN_TEST({
    __sync_add_and_fetch(&roundUpdateCount, 1);
  })

  vertex_t * current = &nodes[index];
  // recalculate this node's data
  double data = current->data;
  for (size_t i = 0; i < current->cntEdges; ++i) {
    data += nodes[current->edges[i]].data;
  }
  data /= (current->cntEdges + 1);
  current->data = data;
}

#pragma GCC diagnostic ignored "-Wunused-variable"
// we need this processNodeSerial hack to avoid a Cilk bug when trying to do
// spawn depth limiting (which in turn is necessary because otherwise on large problem
// sizes worker stacks are overflowed)
static void processNodeSerial(vertex_t * nodes, const vid_t index, const vid_t cntNodes) {
  update(nodes, index);
  vertex_t * current = &nodes[index];

  // increment the dependencies for all nodes of greater priority
  // TODO(predrag): consider sorting the edges by priority
  //                so we don't have to check all of them at every step
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

#pragma GCC diagnostic ignored "-Wunused-variable"
static void processNode(vertex_t * nodes, const vid_t index, const vid_t cntNodes,
  const int depth) {
  update(nodes, index);
  vertex_t * current = &nodes[index];

  // increment the dependencies for all nodes of greater priority
  // TODO(predrag): consider sorting the edges by priority
  //                so we don't have to check all of them at every step
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
    }
  }
  cilk_sync;
}

#pragma GCC diagnostic ignored "-Wunused-function"
static void runRoundPrioD1(const int round, vertex_t * nodes,
  const vid_t cntNodes) {
  WHEN_DEBUG({
    cout << "Running d1 prio round " << round << endl;
  })

  WHEN_TEST({
    roundUpdateCount = 0;
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

#pragma GCC diagnostic ignored "-Wunused-function"
static void runRoundChunk(const int round, vertex_t * nodes, const vid_t cntNodes) {
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
    }
  }
  delete chunkIndex;
}

#pragma GCC diagnostic ignored "-Wunused-function"
static void runRoundBsp(const int round, vertex_t * nodes, const vid_t cntNodes) {
  WHEN_DEBUG({
    cout << "Running bsp round " << round << endl;
  })

  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    update(nodes, i);
  }

  WHEN_TEST({
    assert(roundUpdateCount == (uint64_t)cntNodes);
  })
}

int main(int argc, char *argv[]) {
  vertex_t * nodes;
  vid_t cntNodes;
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
#if D1_CHUNK
  calculateNodeDependenciesChunk(nodes, cntNodes);
#else
  calculateNodeDependencies(nodes, cntNodes);
#endif
  // our nodes don't have any real data associated with them
  // generate some fake data instead
  fillInNodeData(nodes, cntNodes);

  struct timespec starttime, endtime;
  result = clock_gettime(CLOCK_MONOTONIC, &starttime);
  assert(result == 0);

  // suppress fake GCC warning, seems to be a bug in GCC 4.8/4.9/5.1
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  for (int i = 0; i < numRounds; ++i) {
#if D1_CHUNK
    runRoundChunk(i, nodes, cntNodes);
#elif D0_BSP
    runRoundBsp(i, nodes, cntNodes);
#else
    runRoundPrioD1(i, nodes, cntNodes);
#endif
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
  cout << "Chunk size bits: " << CHUNK_BITS << '\n';
  cout << "D0_BSP: " << D0_BSP << '\n';
  cout << "D1_PRIO: " << D1_PRIO << '\n';
  cout << "D1_CHUNK: " << D1_CHUNK << '\n';
  cout << "Debug flag: " << DEBUG << '\n';
  cout << "Test flag: " << TEST << '\n';

  // so GCC doesn't eliminate the rounds loop as unnecessary work
  double data = 0.0;
  for (vid_t i = 0; i < cntNodes; ++i) {
    data += nodes[i].data;
  }
  cout << "Final result (ignore this line): " << data << '\n';


  return 0;
}
