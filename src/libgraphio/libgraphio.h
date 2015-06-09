#ifndef LIBGRAPHIO_LIBGRAPHIO_H_
#define LIBGRAPHIO_LIBGRAPHIO_H_

#include <string>
#include <cinttypes>

typedef uint64_t vid_t;  // vertex id type

class EdgeListBuilder {
 public:
  virtual void set_node_count(vid_t cntNodes) {};
  virtual void set_total_edge_count(vid_t totalEdges) {};
  virtual void set_first_edge_of_node(vid_t nodeid, vid_t firstEdgeIndex) {};
  virtual void create_edge(vid_t edgeIndex, vid_t destination) {};
  virtual void build() {};
};

int binadjlistfile_read(const std::string& filepath,
                        EdgeListBuilder * const builder);

int adjlistfile_read(const std::string& filepath,
                     EdgeListBuilder * const builder);

#endif  // LIBGRAPHIO_LIBGRAPHIO_H_
