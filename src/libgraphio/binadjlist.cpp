#include <fstream>
#include <iostream>
#include <string>
#include "./libgraphio.h"
#include "./common.h"

#define BINADJLIST_MAGIC 68862015

// v1 binadjlist structure:
// total number of nodes (N): 8 bytes
// total number of edges (M): 8 bytes
// N edge indexes:            8 bytes each
// M edge destinations:       8 bytes each
static int binadjlistfile_read_v1(const std::string& filepath,
                                  std::ifstream * binadjlist,
                                  EdgeListBuilder * const builder) {
  vid_t cntNodes, totalEdges;
  binadjlist->read(reinterpret_cast<char*>(&cntNodes), sizeof(cntNodes));
  binadjlist->read(reinterpret_cast<char*>(&totalEdges), sizeof(cntNodes));
  builder->set_node_count(cntNodes);
  builder->set_total_edge_count(totalEdges);

  for (vid_t i = 0; i < cntNodes; ++i) {
    vid_t firstEdgeIndex;
    binadjlist->read(reinterpret_cast<char*>(&firstEdgeIndex), sizeof(firstEdgeIndex));
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
    binadjlist->read(reinterpret_cast<char*>(&destination), sizeof(destination));
    builder->create_edge(i, destination);
  }

  if (binadjlist->fail()) {
    std::cerr << "Unknown file error for file " << filepath << std::endl;
    return -1;
  }

  if (!binadjlist->eof()) {
    std::cerr << "File seems to continue past specified end: "
              << filepath << std::endl;
    return -1;
  }

  builder->build();
  return 0;
}

// binadjlist file structure:
// magic number:          4 bytes
// version number:        4 bytes
// version-specific data: see version-specific function
int binadjlistfile_read(const std::string& filepath,
                        EdgeListBuilder * const builder) {
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
    return binadjlistfile_read_v1(filepath, &binadjlist, builder);
  }

  std::cerr << "Unknown version number " << version
            << " for file " << filepath << std::endl;
  return -1;
}
