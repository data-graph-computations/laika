#ifndef IO_H_
#define IO_H_

#include <string>
#include <vector>
#include "./common.h"

using namespace std;

int outputGraph(const vertex_t * const nodes, const vector<vid_t> * const edges,
                const vid_t cntNodes, const string& outputNodeFile,
                const string& outputEdgeFile);

#endif  // IO_H_
