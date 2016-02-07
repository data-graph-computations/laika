#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <stdexcept>
#include "./libgraphio.h"
#include "./common.h"

#define BINADJLIST_MAGIC 68862015

typedef uint64_t adjlist_data_t;

static inline int safe_vid_t_read(std::ifstream * const input,
                                  vid_t * const output) {
  // the conversion from adjlist_data_t to vid_t
  // should always be a narowing conversion
  assert(sizeof(adjlist_data_t) >= sizeof(vid_t));
  adjlist_data_t tmp;
  input->read(reinterpret_cast<char*>(&tmp), sizeof(adjlist_data_t));

  // static_cast is safe because we determined
  // the vid_t -> adjlist_data_t is a widening conversion
  if (tmp <= static_cast<adjlist_data_t>(std::numeric_limits<vid_t>::max())) {
    *output = static_cast<vid_t>(tmp);
    return 0;
  } else {
    std::cerr << "vid_t type not wide enough, please recompile with huge graph support";
    std::cerr << std::endl;
    return -1;
  }
}

static inline void safe_vid_t_write(std::ofstream * const output,
                                    const vid_t value) {
  // the conversion from vid_t to adjlist_data_t
  // should always be a widening conversion
  assert(sizeof(adjlist_data_t) >= sizeof(vid_t));
  adjlist_data_t tmp = static_cast<adjlist_data_t>(value);
  output->write(reinterpret_cast<char*>(&tmp), sizeof(adjlist_data_t));
}

static int binadjlistfile_read_v1(const std::string& filepath,
                                  std::ifstream * const binadjlist,
                                  EdgeListBuilder * const builder,
                                  vid_t startIndex, vid_t endIndex) {

  int result;
  vid_t totalNodes;
  vid_t totalEdges;

  result = safe_vid_t_read(binadjlist, &totalNodes);
  if (result != 0) {
    return result;
  }

  result = safe_vid_t_read(binadjlist, &totalEdges);
  if (result != 0) {
    return result;
  }

  vid_t cntNodes = endIndex - startIndex;
  builder->set_node_count(cntNodes);
  binadjlist.seekg(startIndex * sizeof(vid_t), binadjlist.cur);
  
  vid_t edgeOffset;
  safe_vid_t_read(binadjlist, &edgeOffset);
  builder->set_first_edge_of_node(0, 0);
  vid_t firstEdgeIndex;
  for (vid_t i = 1; i < cntNodes; ++i) {
    result = safe_vid_t_read(binadjlist, &firstEdgeIndex); 
    if (result != 0) {
      return result;
    }
    builder->set_first_edge_of_node(i, firstEdgeIndex - edgeOffset);
  }

  bool skippedNextEdgeIndex = endIndex != totalNodes;
  vid_t cntEdges;
  if (skippedNextEdgeIndex) {
    safe_vid_t_read(binadjlist, &firstEdgeIndex);
    cntEdges = firstEdgeIndex - edgeOffset;
  } else {
    cntEdges = totalEdges - edgeOffset;
  }

  builder->set_total_edge_count(cntEdges);
  binadjlist.seekg(((totalNodes - endIndex) + edgeOffset - (skippedNextEdgeIndex ? 1 : 0)) * sizeof(vid_t), binadjlist.cur);
  
  vid_t destination;
  for (vid_t i = 0; i < cntEdges; ++i) {
    result = safe_vid_t_read(binadjlist, &destination);
    if (result != 0) {
      return result;
    }
    builder->create_edge(i, destination);
  }

  builder->build();
  binadjlist->close();
  return 0;
}

/*
// v1 binadjlist structure:
// total number of nodes (N): 8 bytes
// total number of edges (M): 8 bytes
// N edge indexes:            8 bytes each
// M edge destinations:       8 bytes each
static int binadjlistfile_read_v1(const std::string& filepath,
                                  std::ifstream * const binadjlist,
                                  EdgeListBuilder * const builder) {
  int result;
  vid_t cntNodes;
  vid_t totalEdges;

  result = safe_vid_t_read(binadjlist, &cntNodes);
  if (result != 0) {
    return result;
  }

  result = safe_vid_t_read(binadjlist, &totalEdges);
  if (result != 0) {
    return result;
  }

  builder->set_node_count(cntNodes);
  builder->set_total_edge_count(totalEdges);

  for (vid_t i = 0; i < cntNodes; ++i) {
    vid_t firstEdgeIndex;

    result = safe_vid_t_read(binadjlist, &firstEdgeIndex);
    if (result != 0) {
      return result;
    }

    builder->set_first_edge_of_node(i, firstEdgeIndex);
  }

  if (binadjlist->fail()) {
    std::cerr << "Unknown file error for file " << filepath << std::endl;
    return -1;
  }

  if (binadjlist->eof()) {
    std::cerr << "Unexpected end of file " << filepath << std::endl;
    return -1;
  }

  for (vid_t i = 0; i < totalEdges; ++i) {
    vid_t destination;

    result = safe_vid_t_read(binadjlist, &destination);
    if (result != 0) {
      return result;
    }

    builder->create_edge(i, destination);
  }

  if (binadjlist->fail()) {
    std::cerr << "Unknown file error for file " << filepath << std::endl;
    return -1;
  }

  builder->build();
  binadjlist->close();
  return 0;
}
*/

// binadjlist file structure:
// magic number:          4 bytes
// version number:        4 bytes
// version-specific data: see version-specific function
int binadjlistfile_read(const std::string& filepath,
                        EdgeListBuilder * const builder,
                        vid_t startIndex, vid_t endIndex) {
  std::ifstream binadjlist(filepath, std::ifstream::binary);

  if (!binadjlist.is_open()) {
    std::cerr << "Could not open file " << filepath << std::endl;
    return -1;
  }

  uint32_t magic;
  uint32_t version;
  binadjlist.read(reinterpret_cast<char*>(&magic), sizeof(magic));
  binadjlist.read(reinterpret_cast<char*>(&version), sizeof(version));

  if (binadjlist.eof()) {
    std::cerr << "Unexpected end of file " << filepath << std::endl;
    return -1;
  }

  if (magic != BINADJLIST_MAGIC) {
    std::cerr << "Incorrect magic number for file " << filepath << std::endl;
    return -1;
  }

  if (version == 1) {
    return binadjlistfile_read_v1(filepath, &binadjlist, builder, startIndex, endIndex);
  }

  std::cerr << "Unknown version number " << version
            << " for file " << filepath << std::endl;
  return -1;
}

class BinadjlistWriterV1 : public EdgeListBuilder {
 private:
  std::string filepath;
  std::ofstream * output;
  vid_t cntNodes = -1;
  vid_t totalEdges = -1;
  vid_t lastUsedNodeId = static_cast<vid_t>(-1);
  vid_t lastUsedEdgeId = static_cast<vid_t>(-1);

 public:
  BinadjlistWriterV1(const std::string& filepath,
                     std::ofstream * const output) {
    this->filepath = filepath;
    this->output = output;
  }

  // this function should only be called once
  void set_node_count(vid_t cntNodes) {
    assert(this->cntNodes == static_cast<vid_t>(-1));
    this->cntNodes = cntNodes;
    safe_vid_t_write(this->output, cntNodes);
  }

  // this function should only be called once
  void set_total_edge_count(vid_t totalEdges) {
    assert(this->totalEdges == static_cast<vid_t>(-1));
    this->totalEdges = totalEdges;
    safe_vid_t_write(this->output, totalEdges);
  }

  void set_first_edge_of_node(vid_t nodeid, vid_t firstEdgeIndex) {
    assert(nodeid == this->lastUsedNodeId + 1);
    assert(nodeid < this->cntNodes);
    this->lastUsedNodeId = nodeid;
    safe_vid_t_write(this->output, firstEdgeIndex);
  }

  void create_edge(vid_t edgeIndex, vid_t destination) {
    assert(edgeIndex == this->lastUsedEdgeId + 1);
    assert(edgeIndex < this->totalEdges);
    this->lastUsedEdgeId = edgeIndex;
    safe_vid_t_write(this->output, destination);
  }

  void build() {
    assert(this->lastUsedEdgeId == this->totalEdges - 1);
    assert(this->lastUsedNodeId == this->cntNodes - 1);

    if (this->output->fail()) {
      throw new std::runtime_error("Unknown error for file " + this->filepath);
    }

    this->output->close();
    delete this->output;
  }
};

EdgeListBuilder * binadjlistfile_write(const std::string& filepath) {
  std::ofstream * output = new std::ofstream(filepath, std::ofstream::binary);

  if (!output->is_open()) {
    std::cerr << "Could not open file " << filepath << std::endl;
    return NULL;
  }

  const uint32_t magic = BINADJLIST_MAGIC;
  const uint32_t version = 1;
  output->write(reinterpret_cast<const char*>(&magic), sizeof(magic));
  output->write(reinterpret_cast<const char*>(&version), sizeof(version));
  return new BinadjlistWriterV1(filepath, output);
}
