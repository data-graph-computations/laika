#include "./io.h"
#include <string>

class ComputeEdgeListBuilder : public EdgeListBuilder {
 private:
  vertex_t ** outNodes;
  vertex_t * nodes;
  vid_t * outCntNodes;
  vid_t cntNodes;
  vid_t * outTotalEdges;
  vid_t totalEdges;
  vid_t * edges;
  vid_t ** outEdges;
  numaInit_t numaInit;

 public:
  ComputeEdgeListBuilder(vertex_t ** const outNodes,
                         vid_t * const outCntNodes,
                         vid_t ** const outEdges,
                         vid_t * const outTotalEdges,
                         numaInit_t numaInit) : EdgeListBuilder() {
    this->outNodes = outNodes;
    this->outCntNodes = outCntNodes;
    this->outEdges = outEdges;
    this->outTotalEdges = outTotalEdges;
    this->nodes = NULL;
    this->cntNodes = 0;
    this->edges = NULL;
    this->totalEdges = 0;
    this->numaInit = numaInit;
  }

  void set_node_count(vid_t cntNodes) {
    // this function should only ever be called once
    assert(this->nodes == NULL);
    this->cntNodes = *(this->outCntNodes) = cntNodes;
    if (this->numaInit.numaInitFlag) {
      this->nodes = *(this->outNodes) =
        static_cast<vertex_t *>(numaCalloc(numaInit, sizeof(vertex_t), cntNodes));
    } else {
      this->nodes = *(this->outNodes) = new vertex_t[cntNodes];
    }
  }

  void set_total_edge_count(vid_t totalEdges) {
    // this function should only ever be called once
    assert(this->edges == NULL);
    this->totalEdges = *(this->outTotalEdges) = totalEdges;
    if (this->numaInit.numaInitFlag) {
      this->edges = *(this->outEdges) =
        static_cast<vid_t *>(numaCalloc(numaInit, sizeof(vid_t), totalEdges));
    } else {
      this->edges = *(this->outEdges) = new vid_t[totalEdges];
    }
  }

  void set_first_edge_of_node(vid_t nodeid, vid_t firstEdgeIndex) {
    assert(nodeid < this->cntNodes);
    this->nodes[nodeid].edges = this->edges + firstEdgeIndex;
  }

  void create_edge(vid_t edgeIndex, vid_t destination) {
    assert(edgeIndex < this->totalEdges);
    this->edges[edgeIndex] = destination;
  }

  void build() {
    cilk_for (vid_t i = 1; i < this->cntNodes; ++i) {
      this->nodes[i-1].cntEdges =
        this->nodes[i].edges - this->nodes[i-1].edges;
    }
    vid_t lastNode = this->cntNodes - 1;
    vid_t * edgesEnd = this->edges + this->totalEdges;
    this->nodes[lastNode].cntEdges = edgesEnd - this->nodes[lastNode].edges;

    WHEN_TEST({
      vid_t totalCountedEdges = 0;
      for (vid_t i = 0; i < this->cntNodes; ++i) {
        totalCountedEdges += this->nodes[i].cntEdges;
      }
      assert(totalCountedEdges == this->totalEdges);
    })
  }
};

int readEdgesFromFile(const string filepath,
                      vertex_t ** outNodes,
                      vid_t * outCntNodes,
                      numaInit_t numaInit =
                        numaInit_t(0, 0, false)) {
  vid_t * edges;
  vid_t totalEdges;
  ComputeEdgeListBuilder builder(outNodes, outCntNodes, &edges, &totalEdges, numaInit);

  return edgelistfile_read(filepath, &builder);
}
