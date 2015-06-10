#include "./io.h"
#include <cstdio>
#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
#include <algorithm>
#include "./common.h"
#include "../libgraphio/libgraphio.h"

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

class ReorderEdgeListBuilder : public EdgeListBuilder {
 private:
  vertex_t * nodes;
  vid_t expectedCntNodes;
  vid_t ** outEdges;
  vid_t * edges;
  vid_t * outTotalEdges;
  vid_t totalEdges;

 public:
  ReorderEdgeListBuilder(vertex_t * const nodes,
                         const vid_t expectedCntNodes,
                         vid_t ** const outEdges) : EdgeListBuilder() {
    this->nodes = nodes;
    this->expectedCntNodes = expectedCntNodes;
    this->outEdges = outEdges;
    this->edges = NULL;
    *outEdges = NULL;
  }

  void set_node_count(vid_t cntNodes) {
    // this function should only ever be called once
    assert(this->expectedCntNodes == cntNodes);
  }

  void set_total_edge_count(vid_t totalEdges) {
    // this function should only ever be called once
    assert(this->edges == NULL);
    this->edges = *(this->outEdges) = new vid_t[totalEdges];
    this->totalEdges = *(this->outTotalEdges) = totalEdges;
  }

  void set_first_edge_of_node(vid_t nodeid, vid_t firstEdgeIndex) {
    assert(nodeid < this->expectedCntNodes);
    this->nodes[nodeid].edgeData.edges = this->edges + firstEdgeIndex;
  }

  void create_edge(vid_t edgeIndex, vid_t destination) {
    assert(edgeIndex < this->totalEdges);
    this->edges[edgeIndex] = destination;
  }

  void build() {
    vid_t i;
    cilk_for (i = 1; i < this->expectedCntNodes; ++i) {
      this->nodes[i-1].edgeData.cntEdges =
        this->nodes[i].edgeData.edges - this->nodes[i-1].edgeData.edges;
    }
    i = this->expectedCntNodes - 1;
    vid_t * edgesEnd = this->edges + this->totalEdges;
    this->nodes[i].edgeData.cntEdges = edgesEnd - this->nodes[i].edgeData.edges;

    WHEN_TEST({
      vid_t totalCountedEdges = 0;
      for (vid_t i = 0; i < this->expectedCntNodes; ++i) {
        totalCountedEdges += this->nodes[i].edgeData.cntEdges;
      }
      assert(totalCountedEdges == this->totalEdges);
    })
  }
};

int readEdgesFromFile(const string filepath, vertex_t * nodes, vid_t cntNodes) {
  vid_t * edges;
  vid_t totalEdges;
  ReorderEdgeListBuilder builder(nodes, cntNodes, &edges, &totalEdges);

  return adjlistfile_read(filepath, &builder);
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
  try {
    EdgeListBuilder * builder = adjlistfile_write(filepath);
    if (builder == NULL) {
      std::cerr << "Received null builder when writing to file " << filepath << std::endl;
      return -1;
    }

    builder->set_node_count(cntNodes);

    vid_t totalEdges = 0;
    for (vid_t i = 0; i < cntNodes; ++i) {
      totalEdges += reorderedNodes[i].edgeData.cntEdges;
    }
    builder->set_total_edge_count(totalEdges);

    // calculate offsets
    totalEdges = 0;
    for (vid_t i = 0; i < cntNodes; ++i) {
      builder->set_first_edge_of_node(i, totalEdges);
      totalEdges += reorderedNodes[i].edgeData.cntEdges;
    }

    // writing edges
    totalEdges = 0;
    for (vid_t i = 0; i < cntNodes; ++i) {
      const edges_t * const edgeData = &reorderedNodes[i].edgeData;
      for (vid_t j = 0; j < edgeData->cntEdges; ++j) {
        const vid_t translatedEdge = translationMapping != NULL ?
          translationMapping[edgeData->edges[j]] : edgeData->edges[j];

        builder->create_edge(totalEdges++, translatedEdge);
      }
    }

    builder->build();
    return 0;
  } catch (std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return -1;
  }
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
