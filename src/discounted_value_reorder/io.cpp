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

class ReorderEdgeListBuilder : public EdgeListBuilder {
 private:
  vertex_t * nodes;
  vertex_t ** outNodes;
  vid_t * outCntNodes;
  vid_t ** outEdges;
  vid_t * edges;
  vid_t * outTotalEdges;
  vid_t cntNodes;
  vid_t totalEdges;

 public:
  ReorderEdgeListBuilder(vertex_t ** const outNodes,
                         vid_t * const outCntNodes,
                         vid_t ** const outEdges,
                         vid_t * const outTotalEdges) : EdgeListBuilder() {
    this->outNodes = outNodes;
    this->outCntNodes = outCntNodes;
    this->outEdges = outEdges;
    this->outTotalEdges = outTotalEdges;
    this->nodes = NULL;
    this->edges = NULL;
    *outNodes = NULL;
    *outEdges = NULL;
  }

  void set_node_count(vid_t cntNodes) {
    // this function should only ever be called once
    assert(this->nodes == NULL);
    this->cntNodes = *(this->outCntNodes) = cntNodes;
    this->nodes = *(this->outNodes) = new (std::nothrow) vertex_t[cntNodes];
  }

  void set_total_edge_count(vid_t totalEdges) {
    // this function should only ever be called once
    assert(this->edges == NULL);
    this->edges = *(this->outEdges) = new vid_t[totalEdges];
    this->totalEdges = *(this->outTotalEdges) = totalEdges;
  }

  void set_first_edge_of_node(vid_t nodeid, vid_t firstEdgeIndex) {
    assert(nodeid < this->cntNodes);
    this->nodes[nodeid].edgeData.edges = this->edges + firstEdgeIndex;
  }

  void create_edge(vid_t edgeIndex, vid_t destination) {
    assert(edgeIndex < this->totalEdges);
    this->edges[edgeIndex] = destination;
  }

  void build() {
    cilk_for (vid_t i = 1; i < this->cntNodes; ++i) {
      this->nodes[i-1].edgeData.cntEdges =
        this->nodes[i].edgeData.edges - this->nodes[i-1].edgeData.edges;
    }
    vid_t lastNode = this->cntNodes - 1;
    vid_t * edgesEnd = this->edges + this->totalEdges;
    this->nodes[lastNode].edgeData.cntEdges =
      edgesEnd - this->nodes[lastNode].edgeData.edges;

    WHEN_TEST({
      vid_t totalCountedEdges = 0;
      for (vid_t i = 0; i < this->cntNodes; ++i) {
        totalCountedEdges += this->nodes[i].edgeData.cntEdges;
      }
      assert(totalCountedEdges == this->totalEdges);
    })
  }
};

int readEdgesFromFile(const string& filepath,
                      vertex_t ** const outNodes, vid_t * const cntNodes) {
  vid_t * edges;
  vid_t totalEdges;
  ReorderEdgeListBuilder builder(outNodes, cntNodes, &edges, &totalEdges);

  return edgelistfile_read(filepath, &builder);
}
