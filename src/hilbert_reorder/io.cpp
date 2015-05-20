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

static void cleanupOnFormatError(FILE * input, string type, int line) {
  int result;
  cerr << "ERROR: Illegal " << type << " file format on line " << line << endl;
  result = fclose(input);
  assert(result == 0);
}

int readNodesFromFile(const string filepath, vertex_t ** outNodes, vid_t * outCount) {
  FILE * input = fopen(filepath.c_str(), "r");
  if (input == NULL) {
    cerr << "ERROR: Couldn't open file " << filepath << endl;
    return -1;
  }

  vid_t n, ndims, zero1, zero2, result;
  result = fscanf(input, "%lu %lu %lu %lu\n", &n, &ndims, &zero1, &zero2);
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

  for (vid_t i = 0; i < n; ++i) {
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

int readUndirectedEdgesFromFile(const string filepath,
                                vertex_t * nodes, vid_t cntNodes) {
  FILE * input = fopen(filepath.c_str(), "r");
  if (input == NULL) {
    cerr << "ERROR: Couldn't open file " << filepath << endl;
    return -1;
  }

  char adjGraph[15];
  vid_t n, m, result;

  result = fscanf(input, "%15s\n%lu\n%lu\n", adjGraph, &n, &m);
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

  edge_t * edgeList = new (std::nothrow) edge_t[2 * m];
  assert(edgeList != 0);
  uint64_t offset, previousOffset = 0;
  for (vid_t i = 0; i < n; ++i) {
    result = fscanf(input, "%lu\n", &offset);
    if (result != 1) {
      cleanupOnFormatError(input, "edge", i + 3);
      return -1;
    }

    nodes[i].edgeData.edges = NULL;
    if (i > 0) {
      nodes[i-1].edgeData.cntEdges = offset - previousOffset;
    }
    previousOffset = offset;
  }
  nodes[n-1].edgeData.cntEdges = m - previousOffset;

  vid_t curNode = 0;
  size_t curNodeEdges = nodes[curNode].edgeData.cntEdges;
  size_t curNodeSeen = 0;
  vid_t e;
  for (vid_t i = 0; i < m; ++i) {
    if (curNodeSeen == curNodeEdges) {
      while (nodes[++curNode].edgeData.cntEdges == 0) {}
      curNodeEdges = nodes[curNode].edgeData.cntEdges;
      curNodeSeen = 0;
    }
    result = fscanf(input, "%lu\n", &e);
    if (result != 1) {
      cleanupOnFormatError(input, "edge", i + n + 3);
      return -1;
    }
    edgeList[2 * i] = edge_t(curNode, e);
    edgeList[2 * i + 1] = edge_t(e, curNode);
    curNodeSeen++;
  }
  std::sort(edgeList, edgeList + 2 * m);

  vid_t *nbrs = new vid_t[2 * m];
  nbrs[0] = edgeList[0].second;
  nodes[edgeList[0].first].edgeData.edges = nbrs;

  int numReal = 1;
  for (vid_t i = 1; i < 2 * m; i++) {
    if (edgeList[i] == edgeList[i - 1]) {
      continue;  // duplicate
    }
    if (edgeList[i].first != edgeList[i - 1].first) {
      nodes[edgeList[i].first].edgeData.edges = nbrs + numReal;
      nodes[edgeList[i - 1].first].edgeData.cntEdges =
        (nbrs + numReal) - nodes[edgeList[i - 1].first].edgeData.edges;
    }
    nbrs[numReal++] = edgeList[i].second;
  }
  nodes[edgeList[2 * m - 1].first].edgeData.cntEdges =
    (nbrs + numReal) - nodes[edgeList[2 * m - 1].first].edgeData.edges;

  delete edgeList;
  return 0;
}

int readEdgesFromFile(const string filepath, vertex_t * nodes, vid_t cntNodes) {
  FILE * input = fopen(filepath.c_str(), "r");
  if (input == NULL) {
    cerr << "ERROR: Couldn't open file " << filepath << endl;
    return -1;
  }

  char adjGraph[15];
  vid_t n, m, result;

  result = fscanf(input, "%15s\n%lu\n%lu\n", adjGraph, &n, &m);
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
  for (vid_t i = 0; i < n; ++i) {
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

  for (vid_t i = 0; i < m; ++i) {
    result = fscanf(input, "%lu\n", &edgeList[i]);
    if (result != 1) {
      cleanupOnFormatError(input, "edge", i + n + 3);
      return -1;
    }
  }

  return 0;
}

static int outputNodes(const vertex_t * const reorderedNodes, const vid_t cntNodes,
                          const string& filepath) {
  FILE * output = fopen(filepath.c_str(), "w");
  if (output == NULL) {
    cerr << "ERROR: Couldn't open file " << filepath << endl;
    return -1;
  }

  fprintf(output, "%lu 3 0 0\n", cntNodes);
  for (vid_t i = 0; i < cntNodes; ++i) {
    fprintf(output, "%lu %.6f %.6f %.6f\n",
            i, reorderedNodes[i].x, reorderedNodes[i].y, reorderedNodes[i].z);
  }
  fprintf(output, "# Reordered using Hilbert curve reordering.\n");

  return 0;
}

int outputEdges(const vertex_t * const reorderedNodes, const vid_t cntNodes,
                          const vid_t * const translationMapping,
                          const string& filepath) {
  FILE * output = fopen(filepath.c_str(), "w");
  if (output == NULL) {
    cerr << "ERROR: Couldn't open file " << filepath << endl;
    return -1;
  }

  fprintf(output, ADJGRAPH "\n");
  fprintf(output, "%lu\n", cntNodes);

  // calculating the total number of edges
  size_t totalEdges = 0;
  for (vid_t i = 0; i < cntNodes; ++i) {
    totalEdges += reorderedNodes[i].edgeData.cntEdges;
  }
  fprintf(output, "%lu\n", totalEdges);

  // calculating offsets
  totalEdges = 0;
  for (vid_t i = 0; i < cntNodes; ++i) {
    fprintf(output, "%lu\n", totalEdges);
    totalEdges += reorderedNodes[i].edgeData.cntEdges;
  }

  // outputing edges
  for (vid_t i = 0; i < cntNodes; ++i) {
    const edges_t * const edgeData = &reorderedNodes[i].edgeData;
    for (size_t j = 0; j < edgeData->cntEdges; ++j) {
      const vid_t translatedEdge = translationMapping != NULL ?
        translationMapping[edgeData->edges[j]] : edgeData->edges[j];
      fprintf(output, "%lu\n", translatedEdge);
    }
  }

  return 0;
}

int outputReorderedGraph(const vertex_t * const reorderedNodes, const vid_t cntNodes,
                         const vid_t * const translationMapping,
                         const string& outputNodeFile, const string& outputEdgeFile) {
  int result;
  result = outputNodes(reorderedNodes, cntNodes, outputNodeFile);
  if (result != 0) {
    return result;
  }

  return outputEdges(reorderedNodes, cntNodes, translationMapping, outputEdgeFile);
}
