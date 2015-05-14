#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <iostream>
#include <vector>
#include <exception>
#include <algorithm>
#include "./common.h"
#include "./io.h"
#include "./stats.h"
#include "./edge_generator.h"

#if DIST_UNIFORM
  #include "./uniform_distribution.h"
#else
  #error "No distribution selected, please use DIST_UNIFORM."
#endif

#ifndef MAX_EDGE_LENGTH
  #define MAX_EDGE_LENGTH 16.0
#endif

using namespace std;

// static inline double runtime(clock_t first, clock_t second) {
//   return (static_cast<double>(second - first)) / CLOCKS_PER_SEC;
// }

static void execute(vertex_t * const nodes,
                    vector<vid_t> * const edges,
                    const vid_t cntNodes,
                    const string outputNodeFile,
                    const string outputEdgeFile) {
  generateGraph(nodes, edges, cntNodes);
  generateEdges(nodes, edges, cntNodes, MAX_EDGE_LENGTH);
  printGraphStats(nodes, edges, cntNodes);
  int result = outputGraph(nodes, edges, cntNodes, outputNodeFile, outputEdgeFile);
  assert(result == 0);
}

int main(int argc, char *argv[]) {
  vertex_t * nodes;
  vector<vid_t> * edges;
  vid_t cntNodes = 0;
  char * outputNodeFile;
  char * outputEdgeFile;

  const int numArgs = 3;

  if (argc != (numArgs + 1)) {
    cerr << "\nERROR: Expected " << numArgs << " arguments, received " << argc-1 << '\n';
    cerr << "Usage: ./graphgen2 <num_nodes> "
            "<node_file> <edge_file>" << endl;
    return 1;
  }

  // bug in GCC, phantom uninitialized variable reported here
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  try {
    cntNodes = stoi(argv[1]);
  } catch (exception& e) {
    cerr << "\nERROR: " << e.what() << endl;
    return 1;
  }
  #pragma GCC diagnostic pop

  outputNodeFile = argv[2];
  outputEdgeFile = argv[3];

  cout << "Graph size: " << cntNodes << '\n';
  cout << "Output node file: " << outputNodeFile << '\n';
  cout << "Output edge file: " << outputEdgeFile << '\n';

  nodes = new vertex_t[cntNodes];
  edges = new vector<vid_t>[cntNodes];

  execute(nodes, edges, cntNodes, outputNodeFile, outputEdgeFile);

  delete[] nodes;
  delete[] edges;

  return 0;
}
