#ifndef LIBGRAPHIO_LIBGRAPHIO_H_
#define LIBGRAPHIO_LIBGRAPHIO_H_

#include <string>
#include <cinttypes>

typedef uint64_t vid_t;  // vertex id type

class EdgeListBuilder {
 public:
  // these methods must be called in order, top to bottom
  // otherwise, the behavior is undefined

  // this function should only ever be called once
  virtual void set_node_count(vid_t cntNodes) {}


  // this function should only ever be called once
  virtual void set_total_edge_count(vid_t totalEdges) {}

  // this function should be called in increasing order of nodeid
  virtual void set_first_edge_of_node(vid_t nodeid, vid_t firstEdgeIndex) {}

  // this function should be called in increasing order of edgeIndex
  virtual void create_edge(vid_t edgeIndex, vid_t destination) {}

  // this function should only ever be called once
  virtual void build() {}
};

int binadjlistfile_read(const std::string& filepath,
                        EdgeListBuilder * const builder);

int adjlistfile_read(const std::string& filepath,
                     EdgeListBuilder * const builder);

EdgeListBuilder * binadjlistfile_write(const std::string& filepath);

EdgeListBuilder * adjlistfile_write(const std::string& filepath);

#endif  // LIBGRAPHIO_LIBGRAPHIO_H_
