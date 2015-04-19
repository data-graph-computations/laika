#include "./io.h"
#include <cstdio>
#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
#include "./common.h"

using namespace std;

#define ADJGRAPH "AdjacencyGraph"

static void cleanupOnFormatError(FILE * input, string type, int line) {
  int result;
  cerr << "ERROR: Illegal " << type << " file format on line " << line << endl;
  result = fclose(input);
  assert(result == 0);
}

int readNodesFromFile(const string filepath, vertex_t ** outNodes, int * outCount) {
  FILE * input = fopen(filepath.c_str(), "r");
  if (input == NULL) {
    cerr << "ERROR: Couldn't open file " << filepath << endl;
    return -1;
  }

  int n, ndims, zero1, zero2, result;
  result = fscanf(input, "%d %d %d %d\n", &n, &ndims, &zero1, &zero2);
  if (result != 4) {
    cleanupOnFormatError(input, "node", 1);
    return -1;
  }
  assert(n >= 1);
  assert(ndims == 3);
  assert(zero1 == 0);
  assert(zero2 == 0);

  vertex_t * nodes = new (std::nothrow) vertex_t[n];
  assert(nodes != 0);

  for (int i = 0; i < n; ++i) {
    result = fscanf(input, "%lu %lf %lf %lf\n",
                    &nodes[i].id, &nodes[i].x, &nodes[i].y, &nodes[i].z);
    if (result != 4) {
      cleanupOnFormatError(input, "node", i+1);
      return -1;
    }
  }

  result = fclose(input);
  assert(result == 0);

  *outCount = n;
  *outNodes = nodes;
  return 0;
}

int readEdgesFromFile(const string filepath, vertex_t * nodes, int cntNodes) {
  FILE * input = fopen(filepath.c_str(), "r");
  if (input == NULL) {
    cerr << "ERROR: Couldn't open file " << filepath << endl;
    return -1;
  }

  char adjGraph[15];
  int n, m, result;

  result = fscanf(input, "%15s\n%d\n%d\n", adjGraph, &n, &m);
  if (result != 3 || strcmp(ADJGRAPH, adjGraph) != 0) {
    cleanupOnFormatError(input, "edge", 1);
    return -1;
  }

  if (n != cntNodes) {
    cerr << "ERROR: Expected " << cntNodes << " nodes, found "
         << n << " nodes in edge file.";
    result = fclose(input);
    assert(result == 0);
    return -1;
  }

  assert(m >= 0);

  vid_t * edgeList = new (std::nothrow) vid_t[m];
  assert(edgeList != 0);
  uint64_t offset, previousOffset = 0;
  for (int i = 0; i < n; ++i) {
    result = fscanf(input, "%lu\n", &offset);
    if (result != 1) {
      cleanupOnFormatError(input, "edge", i + 3);
      return -1;
    }
    nodes[i].edgeData.edges = edgeList + offset;

    if (i > 0) {
      nodes[i-1].edgeData.cntEdges = offset - previousOffset;
    }
    previousOffset = offset;
  }
  nodes[n-1].edgeData.cntEdges = m - previousOffset;

  for (int i = 0; i < m; ++i) {
    result = fscanf(input, "%lu\n", &edgeList[i]);
    if (result != 1) {
      cleanupOnFormatError(input, "edge", i + n + 3);
      return -1;
    }
  }

  return 0;
}

static int outputNodes(const vertex_t * const reorderedNodes, const int cntNodes,
                          const string& filepath) {
  FILE * output = fopen(filepath.c_str(), "w");
  if (output == NULL) {
    cerr << "ERROR: Couldn't open file " << filepath << endl;
    return -1;
  }

  fprintf(output, "%d 3 0 0\n", cntNodes);
  for (int i = 0; i < cntNodes; ++i) {
    fprintf(output, "%d %14.14f %14.14f %14.14f\n",
            i, reorderedNodes[i].x, reorderedNodes[i].y, reorderedNodes[i].z);
  }
  fprintf(output, "# Reordered using Hilbert curve reordering.\n");

  return 0;
}

static int outputEdges(const vertex_t * const reorderedNodes, const int cntNodes,
                          const vid_t * const translationMapping,
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
    totalEdges += reorderedNodes[i].edgeData.cntEdges;
  }
  fprintf(output, "%lu\n", totalEdges);

  // calculating offsets
  totalEdges = 0;
  for (int i = 0; i < cntNodes; ++i) {
    fprintf(output, "%lu\n", totalEdges);
    totalEdges += reorderedNodes[i].edgeData.cntEdges;
  }

  // outputing edges
  for (int i = 0; i < cntNodes; ++i) {
    const edges_t * const edgeData = &reorderedNodes[i].edgeData;
    for (size_t j = 0; j < edgeData->cntEdges; ++j) {
      const vid_t translatedEdge = translationMapping[edgeData->edges[j]];
      fprintf(output, "%lu\n", translatedEdge);
    }
  }

  return 0;
}

int outputReorderedGraph(const vertex_t * const reorderedNodes, const int cntNodes,
                         const vid_t * const translationMapping,
                         const string& outputNodeFile, const string& outputEdgeFile) {
  int result;
  result = outputNodes(reorderedNodes, cntNodes, outputNodeFile);
  if (result != 0) {
    return result;
  }

  return outputEdges(reorderedNodes, cntNodes, translationMapping, outputEdgeFile);
}
