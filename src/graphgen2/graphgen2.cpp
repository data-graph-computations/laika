#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <exception>
#include "./common.h"
#include "./io.h"

#if DIST_UNIFORM
  #include "./uniform_distribution.h"
#endif

using namespace std;

int main(int argc, char *argv[]) {
  vertex_t * nodes;
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

  try {
    cntNodes = stoi(argv[1]);
  } catch (exception& e) {
    cerr << "\nERROR: " << e.what() << endl;
    return 1;
  }

  outputNodeFile = argv[2];
  outputEdgeFile = argv[3];

  cout << "Graph size: " << cntNodes << '\n';
  cout << "Output node file: " << outputNodeFile << '\n';
  cout << "Output edge file: " << outputEdgeFile << '\n';
  cout << "Distribution: " << printDistributionName() << endl;

  nodes = new vertex_t[cntNodes];

  generateGraph(nodes, cntNodes);
  outputGraph(nodes, cntNodes, outputNodeFile, outputEdgeFile);

  delete[] nodes;

  return 0;
}
