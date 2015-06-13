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

using namespace std;

static inline double runtime(clock_t first, clock_t second) {
  return (static_cast<double>(second - first)) / CLOCKS_PER_SEC;
}

WHEN_TEST(
static void ensureEdgesAreBidirectional(const vector<vid_t> * const edges,
                                        vid_t cntNodes) {
  for (vid_t current = 0; current < cntNodes; ++current) {
    for (auto& neighbor : edges[current]) {
      bool found = false;
      for (auto& backEdge : edges[neighbor]) {
        if (current == backEdge) {
          found = true;
          break;
        }
      }
      assert(found);
    }
  }
})

static int execute(vertex_t * const nodes,
                   vector<vid_t> * const edges,
                   const vid_t cntNodes,
                   const vid_t nodeAvgDegree,
                   const string outputNodeFile,
                   const string outputEdgeFile) {
  int result = 0;
  clock_t before, after;

  before = clock();
  generateGraph(nodes, edges, cntNodes);
  after = clock();
  cout << "Point generation complete in: " << runtime(before, after) << "s\n";

  before = after;
  result = generateEdges(nodes, edges, cntNodes, nodeAvgDegree);
  if (result != 0) {
    return result;
  }
  after = clock();
  cout << "Edge generation complete in: " << runtime(before, after) << "s\n";

  before = after;
  printGraphStats(nodes, edges, cntNodes);
  after = clock();
  cout << "Stats calculation complete in: " << runtime(before, after) << "s\n";

  WHEN_TEST({
    before = after;
    ensureEdgesAreBidirectional(edges, cntNodes);
    after = clock();
    cout << "Edges verified to exist in both directions: " <<
            runtime(before, after) << "s\n";
  })

  before = after;
  result = outputGraph(nodes, edges, cntNodes, outputNodeFile, outputEdgeFile);
  if (result != 0) {
    return result;
  }
  after = clock();
  cout << "Graph written to file in: " << runtime(before, after) << "s\n";

  return 0;
}

int main(int argc, char *argv[]) {
  vertex_t * nodes;
  vector<vid_t> * edges;
  vid_t cntNodes = 0;
  vid_t nodeAvgDegree = 0;
  char * outputNodeFile;
  char * outputEdgeFile;

  const int numArgs = 4;

  cout << '\n';

  if (argc != (numArgs + 1)) {
    cerr << "ERROR: Expected " << numArgs << " arguments, received " << argc-1 << '\n';
    cerr << "Usage: ./graphgen2 <num_nodes> <node_avg_degree> "
            "<node_file> <edge_file>" << endl;
    return 1;
  }

  // bug in GCC, phantom uninitialized variable reported here
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  try {
    cntNodes = stoi(argv[1]);
    nodeAvgDegree = stoi(argv[2]);
  } catch (exception& e) {
    cerr << "ERROR: " << e.what() << endl;
    return 1;
  }
  #pragma GCC diagnostic pop

  outputNodeFile = argv[3];
  outputEdgeFile = argv[4];

  cout << "Graph size: " << cntNodes << '\n';
  cout << "Graph average degree: " << nodeAvgDegree << '\n';
  cout << "Output node file: " << outputNodeFile << '\n';
  cout << "Output edge file: " << outputEdgeFile << '\n';

  nodes = new vertex_t[cntNodes];
  edges = new vector<vid_t>[cntNodes];

  int result = execute(nodes, edges, cntNodes, nodeAvgDegree,
                       outputNodeFile, outputEdgeFile);
  assert(result == 0);

  delete[] nodes;
  delete[] edges;

  return 0;
}
