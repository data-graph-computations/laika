// #include <cilk/cilk.h>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <ctime>
#include <string>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "./common.h"
#include "./io.h"

using namespace std;

#define PRIORITY_GROUP_BITS 8

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
  for (int i = 0; i < cntNodes; ++i) {
    nodes[i].priority = createPriority(nodes[i].id, bitsInId);

    WHEN_DEBUG({
      cout << "Node ID " << nodes[i].id
           << " got priority " << nodes[i].priority << '\n';
    })
  }

  WHEN_TEST({
    for (int i = 0; i < cntNodes; ++i) {
      for (int j = i + 1; j < cntNodes; ++j) {
        assert(nodes[i].priority != nodes[j].priority);
      }
    }
  })
}

// fill in each node with random-looking data
static void fillInNodeData(vertex_t * nodes, const int cntNodes) {
  for (int i = 0; i < cntNodes; ++i) {
    nodes[i].data = nodes[i].priority + ((-nodes[i].id) ^ (nodes[i].dependencies * 31));
  }
}

static void calculateNodeDependencies(vertex_t * nodes, const int cntNodes) {
  int i;
  for (i = 0; i < cntNodes; ++i) {
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
    for (size_t i = 0; i < current->cntEdges; ++i) {
      vid_t neighborId = current->edges[i];
      vertex_t * neighbor = &nodes[neighborId];
      if (neighbor->priority > current->priority) {
        ++neighbor->satisfied;
        if (neighbor->satisfied == neighbor->dependencies) {
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

  for (int i = 0; i < cntNodes; ++i) {
    nodes[i].satisfied = 0;
  }

  for (int i = 0; i < cntNodes; ++i) {
    processNode(nodes, i, cntNodes);
  }
}

int main(int argc, char *argv[]) {
  vertex_t * nodes;
  int cntNodes;
  char * inputEdgeFile;
  int numRounds = 100;

  if (argc != 2) {
    cerr << "\nERROR: Expected 1 argument, received " << argc-1 << '\n';
    cerr << "Usage: ./compute <input_edges>" << endl;
    return 1;
  }

  inputEdgeFile = argv[1];

  cout << "Input edge file: " << inputEdgeFile << '\n';

  int result = readEdgesFromFile(inputEdgeFile, &nodes, &cntNodes);
  assert(result == 0);

  int bitsInId = calculateIdBitSize(cntNodes);
  WHEN_DEBUG({ cout << "Bits in ID: " << bitsInId << '\n'; })
  assignNodePriorities(nodes, cntNodes, bitsInId);
  calculateNodeDependencies(nodes, cntNodes);

  // our nodes don't have any real data associated with them
  // generate some fake data instead
  fillInNodeData(nodes, cntNodes);

  clock_t start = clock();

  for (int i = 0; i < numRounds; ++i) {
    runRound(i, nodes, cntNodes);
  }

  double seconds = static_cast<double>(clock() - start) / CLOCKS_PER_SEC;

  cout << "Done computing " << numRounds << " rounds!\n";
  cout << "Time taken:     " << setprecision(8) << seconds << "s\n";
  cout << "Time per round: " << setprecision(8) << seconds / numRounds << "s\n";

  // so GCC doesn't eliminate the rounds loop as unnecessary work
  double data = 0.0;
  for (int i = 0; i < cntNodes; ++i) {
    data += nodes[i].data;
  }
  cout << "Final result: " << data << '\n';

  return 0;
}
