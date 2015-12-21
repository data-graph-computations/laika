#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include "./libgraphio.h"
#include "./common.h"

#define ADJGRAPH "AdjacencyGraph"

#if HUGE_GRAPH_SUPPORT
  #define VID_T_LITERAL "%lld"
#else
  #define VID_T_LITERAL "%d"
#endif

static void cleanupOnFormatError(FILE * const input,
                                 const std::string& type, const int line) {
  int result;
  std::cerr << "ERROR: Illegal " << type
            << " file format on line " << line << std::endl;
  result = fclose(input);
  assert(result == 0);
}

int adjlistfile_read(const std::string& filepath,
                     EdgeListBuilder * const builder) {
  FILE * input = fopen(filepath.c_str(), "r");
  if (input == NULL) {
    std::cerr << "ERROR: Couldn't open file " << filepath << std::endl;
    return -1;
  }

  char adjGraph[15];
  vid_t cntNodes, totalEdges, result;

  // read header line of adjlist file
  result = fscanf(input, "%15s\n" VID_T_LITERAL "\n" VID_T_LITERAL "\n",
                  adjGraph, &cntNodes, &totalEdges);
  if (result != 3 || strcmp(ADJGRAPH, adjGraph) != 0) {
    cleanupOnFormatError(input, "edge", 1);
    return -1;
  }

  builder->set_node_count(cntNodes);
  builder->set_total_edge_count(totalEdges);

  for (vid_t i = 0; i < cntNodes; ++i) {
    vid_t firstEdgeIndex;
    result = fscanf(input, VID_T_LITERAL "\n", &firstEdgeIndex);
    if (result != 1) {
      cleanupOnFormatError(input, "edge", i + 3);
      return -1;
    }
    builder->set_first_edge_of_node(i, firstEdgeIndex);
  }

  for (vid_t i = 0; i < totalEdges; ++i) {
    vid_t destination;
    result = fscanf(input, VID_T_LITERAL "\n", &destination);
    if (result != 1) {
      cleanupOnFormatError(input, "edge", i + cntNodes + 3);
      return -1;
    }
    builder->create_edge(i, destination);
  }

  builder->build();
  return 0;
}

class AdjlistWriter : public EdgeListBuilder {
 private:
  std::string filepath;
  FILE * output;
  vid_t cntNodes;
  vid_t totalEdges;
  vid_t lastUsedNodeId = static_cast<vid_t>(-1);
  vid_t lastUsedEdgeId = static_cast<vid_t>(-1);

 public:
  explicit AdjlistWriter(const std::string& filepath) {
    this->filepath = filepath;
    output = std::fopen(filepath.c_str(), "w");
    if (output == NULL) {
      throw new std::runtime_error("Couldn't open file: " + filepath);
    }

    std::fprintf(output, ADJGRAPH "\n");
  }

  void set_node_count(vid_t cntNodes) {
    this->cntNodes = cntNodes;
    std::fprintf(output, VID_T_LITERAL "\n", cntNodes);
  }

  void set_total_edge_count(vid_t totalEdges) {
    this->totalEdges = totalEdges;
    std::fprintf(output, VID_T_LITERAL "\n", totalEdges);
  }

  void set_first_edge_of_node(vid_t nodeid, vid_t firstEdgeIndex) {
    assert(nodeid == this->lastUsedNodeId + 1);
    assert(nodeid < this->cntNodes);
    this->lastUsedNodeId = nodeid;
    std::fprintf(output, VID_T_LITERAL "\n", firstEdgeIndex);
  }

  void create_edge(vid_t edgeIndex, vid_t destination) {
    assert(edgeIndex == this->lastUsedEdgeId + 1);
    assert(edgeIndex < this->totalEdges);
    this->lastUsedEdgeId = edgeIndex;
    std::fprintf(output, VID_T_LITERAL "\n", destination);
  }

  void build() {
    assert(this->lastUsedEdgeId == this->totalEdges - 1);
    assert(this->lastUsedNodeId == this->cntNodes - 1);
    if (std::ferror(output)) {
      throw new std::runtime_error("Error writing file: " + this->filepath);
    }
    std::fclose(output);
  }
};

EdgeListBuilder * adjlistfile_write(const std::string& filepath) {
  return new AdjlistWriter(filepath);
}
