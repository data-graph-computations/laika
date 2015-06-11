#include "./io.h"
#include <cstdio>
#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include "./common.h"
#include "../libgraphio/libgraphio.h"

using namespace std;

static int outputNodes(const vertex_t * const nodes, const vector<vid_t> * const edges,
                       const int cntNodes, const string& filepath) {
  FILE * output = fopen(filepath.c_str(), "w");
  if (output == NULL) {
    cerr << "ERROR: Couldn't open file " << filepath << endl;
    return -1;
  }

  fprintf(output, "%d 3 0 0\n", cntNodes);
  for (int i = 0; i < cntNodes; ++i) {
    fprintf(output, "%d %.6f %.6f %.6f\n",
            i, nodes[i].x, nodes[i].y, nodes[i].z);
  }
  fprintf(output, "# Generated with graphgen2.\n");

  return 0;
}

static int outputEdges(const vertex_t * const nodes, const vector<vid_t> * const edges,
                       const vid_t cntNodes, const string& filepath) {
  EdgeListBuilder * builder = NULL;
  try {
    builder = adjlistfile_write(filepath);
    if (builder == NULL) {
      std::cerr << "Received null builder when writing to file " << filepath << std::endl;
      return -1;
    }

    builder->set_node_count(cntNodes);

    vid_t totalEdges = 0;
    for (vid_t i = 0; i < cntNodes; ++i) {
      totalEdges += edges[i].size();
    }
    builder->set_total_edge_count(totalEdges);

    // calculate offsets
    totalEdges = 0;
    for (vid_t i = 0; i < cntNodes; ++i) {
      builder->set_first_edge_of_node(i, totalEdges);
      totalEdges += edges[i].size();
    }

    // writing edges
    totalEdges = 0;
    for (vid_t i = 0; i < cntNodes; ++i) {
      for (auto& edge : edges[i]) {
        builder->create_edge(totalEdges++, edge);
      }
    }

    builder->build();

    delete builder;
    return 0;
  } catch (std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    if (builder != NULL) {
      delete builder;
    }
    return -1;
  }
}

int outputGraph(const vertex_t * const nodes, const vector<vid_t> * const edges,
                const vid_t cntNodes, const string& outputNodeFile,
                const string& outputEdgeFile) {
  int result;
  result = outputNodes(nodes, edges, cntNodes, outputNodeFile);
  if (result != 0) {
    return result;
  }

  return outputEdges(nodes, edges, cntNodes, outputEdgeFile);
}
