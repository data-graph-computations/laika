#include "./io.h"
#include <string>
#include <cstdio>
#include <cassert>
#include <cstring>
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

int readEdgesFromFile(const string filepath, vertex_t ** outNodes, vid_t * outCntNodes) {
  FILE * input = fopen(filepath.c_str(), "r");
  if (input == NULL) {
    cerr << "ERROR: Couldn't open file " << filepath << endl;
    return -1;
  }

  char adjGraph[15];
  vid_t n, m, result;

  result = fscanf(input, "%15s\n%ld\n%ld\n", adjGraph, &n, &m);
  if (result != 3 || strcmp(ADJGRAPH, adjGraph) != 0) {
    cleanupOnFormatError(input, "edge", 1);
    return -1;
  }

  vertex_t * nodes = new (std::nothrow) vertex_t[n];
  assert(nodes != NULL);
  assert(m >= 0);

  vid_t * edgeList = new (std::nothrow) vid_t[m];
  assert(edgeList != 0);
  uint64_t offset, previousOffset = 0;
  for (vid_t i = 0; i < n; ++i) {
    result = fscanf(input, "%lu\n", &offset);
    if (result != 1) {
      cleanupOnFormatError(input, "edge", i + 3);
      return -1;
    }
    nodes[i].id = i;
    nodes[i].edges = edgeList + offset;

    if (i > 0) {
      nodes[i-1].cntEdges = offset - previousOffset;
    }
    previousOffset = offset;
  }
  nodes[n-1].cntEdges = m - previousOffset;

  for (vid_t i = 0; i < m; ++i) {
    result = fscanf(input, "%lu\n", &edgeList[i]);
    if (result != 1) {
      cleanupOnFormatError(input, "edge", i + n + 3);
      return -1;
    }
  }

  *outNodes = nodes;
  *outCntNodes = n;

  return 0;
}
