#include "./io.h"
#include <cstdio>
#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
#include <algorithm>
#include "./common.h"

using namespace std;

#define ADJGRAPH "AdjacencyGraph"

static int outputNodes(const vertex_t * const nodes, const int cntNodes,
                       const string& filepath) {
  FILE * output = fopen(filepath.c_str(), "w");
  if (output == NULL) {
    cerr << "ERROR: Couldn't open file " << filepath << endl;
    return -1;
  }

  fprintf(output, "%d 3 0 0\n", cntNodes);
  for (int i = 0; i < cntNodes; ++i) {
    fprintf(output, "%d %14.14f %14.14f %14.14f\n",
            i, nodes[i].x, nodes[i].y, nodes[i].z);
  }
  fprintf(output, "# Generated with graphgen2.\n");

  return 0;
}

static int outputEdges(const vertex_t * const nodes, const int cntNodes,
                       const string& filepath) {
  FILE * output = fopen(filepath.c_str(), "w");
  if (output == NULL) {
    cerr << "ERROR: Couldn't open file " << filepath << endl;
    return -1;
  }

  fprintf(output, ADJGRAPH "\n");
  fprintf(output, "%d\n", cntNodes);

  // calculating the total number of edges
  size_t totalEdges = 0;
  for (int i = 0; i < cntNodes; ++i) {
    totalEdges += nodes[i].edgeData.cntEdges;
  }
  fprintf(output, "%lu\n", totalEdges);

  // calculating offsets
  totalEdges = 0;
  for (int i = 0; i < cntNodes; ++i) {
    fprintf(output, "%lu\n", totalEdges);
    totalEdges += nodes[i].edgeData.cntEdges;
  }

  // outputing edges
  for (int i = 0; i < cntNodes; ++i) {
    const edges_t * const edgeData = &nodes[i].edgeData;
    for (size_t j = 0; j < edgeData->cntEdges; ++j) {
      fprintf(output, "%lu\n", edgeData->edges[j]);
    }
  }

  return 0;
}

int outputGraph(const vertex_t * const nodes, const int cntNodes,
                const string& outputNodeFile, const string& outputEdgeFile) {
  int result;
  result = outputNodes(nodes, cntNodes, outputNodeFile);
  if (result != 0) {
    return result;
  }

  return outputEdges(nodes, cntNodes, translationMapping, outputEdgeFile);
}
