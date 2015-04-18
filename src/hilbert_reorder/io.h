#ifndef IO_H_
#define IO_H_

#include <string>
#include "./common.h"

using namespace std;

int readNodesFromFile(string filepath, vertex_t ** out_nodes, int * out_count);

#endif  // IO_H_
