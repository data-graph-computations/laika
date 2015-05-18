#include "./io.h"
#include <cstdio>
#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include "./common.h"

using namespace std;

#define ADJGRAPH "AdjacencyGraph"

static int outputNodes(const vertex_t * const nodes, const vector<vid_t> * const edges,
                       const int cntNodes, const string& filepath) {
  FILE * output = fopen(filepath.c_str(), "w");
  if (output == NULL) {
    cerr << "ERROR: Couldn't open file " << filepath << endl;
    return -1;
  }

  fprintf(output, "%d 3 0 0\n", cntNodes);
  for (int i = 0; i < cntNodes; ++i) {
    fprintf(output, "%d %.8f %.8f %.8f\n",
            i, nodes[i].x, nodes[i].y, nodes[i].z);
  }
  fprintf(output, "# Generated with graphgen2.\n");

  return 0;
}

static int outputEdges(const vertex_t * const nodes, const vector<vid_t> * const edges,
                       const int cntNodes, const string& filepath) {
  FILE * output = fopen(filepath.c_str(), "w");
  if (output == NULL) {
    cerr << "ERROR: Couldn't open file " << filepath << endl;
    return -1;
  }

  fprintf(output, ADJGRAPH "\n");
  fprintf(output, "%d\n", cntNodes);

  // calculating the total number of edges
  vid_t totalEdges = 0;
  for (int i = 0; i < cntNodes; ++i) {
    totalEdges += edges[i].size();
  }
  fprintf(output, "%lu\n", totalEdges);

  // calculating offsets
  totalEdges = 0;
  for (int i = 0; i < cntNodes; ++i) {
    fprintf(output, "%lu\n", totalEdges);
    totalEdges += edges[i].size();
  }

  // outputing edges
  for (int i = 0; i < cntNodes; ++i) {
    vid_t numEdges = edges[i].size();
    for (vid_t j = 0; j < numEdges; ++j) {
      fprintf(output, "%lu\n", edges[i][j]);
    }
  }

  return 0;
}

int outputGraph(const vertex_t * const nodes, const vector<vid_t> * const edges,
                const vid_t cntNodes, const string& outputNodeFile,
                const string& outputEdgeFile) {
  int result;
  result = outputNodes(nodes, edges, cntNodes, outputNodeFile);
  if (result != 0) {
    return result;
  }

  return outputEdges(nodes, edges, cntNodes, outputEdgeFile);
}
