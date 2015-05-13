#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>
#include <exception>
#include "./common.h"
#include "./io.h"

#if DIST_UNIFORM
  #include "./uniform_distribution.h"
#else
  #error "No distribution selected, please use DIST_UNIFORM."
#endif

using namespace std;

static void printGraphStats(const vertex_t * const nodes,
                            const vector<vid_t> * const edges,
                            const vid_t cntNodes) {
  // none yet
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

  generateGraph(nodes, edges, cntNodes);
  printGraphStats(nodes, edges, cntNodes);
  int result = outputGraph(nodes, edges, cntNodes, outputNodeFile, outputEdgeFile);
  assert(result == 0);

  delete[] nodes;
  delete[] edges;

  return 0;
}
