#include "./stats.h"
#include <vector>
#include <iostream>
#include <algorithm>

using namespace std;

void printGraphStats(const vertex_t * const nodes,
                     const vector<vid_t> * const edges,
                     const vid_t cntNodes) {
  vid_t totalEdges = 0;
  vid_t maxEdges = 0;
  vid_t minNonZeroEdges = -1;
  vid_t numDisconnectedNodes = 0;

  for (vid_t i = 0; i < cntNodes; ++i) {
    vid_t size = edges[i].size();
    totalEdges += size;
    maxEdges = max(maxEdges, size);

    if (size == 0) {
      ++numDisconnectedNodes;
    } else {
      minNonZeroEdges = min(minNonZeroEdges, size);
    }
  }

  cout << "\nGraph stats:\n";
  cout << "  total nodes: " << cntNodes << '\n';
  cout << "  total edges: " << totalEdges << '\n';
  cout << "  avg edges per node: " << static_cast<double>(totalEdges) / cntNodes << '\n';
  cout << "  node max edges: " << maxEdges << '\n';
  cout << "  connected node min edges: " << minNonZeroEdges << '\n';
  cout << "  # of disconnected nodes: " << numDisconnectedNodes << '\n';
  cout << endl;
}
