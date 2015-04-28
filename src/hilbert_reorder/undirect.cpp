#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include "./common.h"
#include "./io.h"

using namespace std;

int main(int argc, char *argv[]) {
  vertex_t * nodes;
  int cntNodes;
  char * inputNodeFile, * inputEdgeFile;
  char * outputNodeFile, * outputEdgeFile;

  if (argc != 5) {
    cerr << "\nERROR: Expected 4 arguments, received " << argc-1 << '\n';
    cerr << "Usage: ./undirect <input_nodes> <input_edges> "
            "<output_nodes> <output_edges>" << endl;
    return 1;
  }

  inputNodeFile = argv[1];
  inputEdgeFile = argv[2];
  outputNodeFile = argv[3];
  outputEdgeFile = argv[4];

  cout << "Input node file: " << inputNodeFile << '\n';
  cout << "Input edge file: " << inputEdgeFile << '\n';
  cout << "Output node file: " << outputNodeFile << '\n';
  cout << "Output edge file: " << outputEdgeFile << '\n';

  int result = readNodesFromFile(inputNodeFile, &nodes, &cntNodes);
  assert(result == 0);
  result = readUndirectedEdgesFromFile(inputEdgeFile, nodes, cntNodes);
  assert(result == 0);
  result = outputReorderedGraph(nodes, cntNodes, NULL,
                                outputNodeFile, outputEdgeFile);
  assert(result == 0);
  return 0;
}
