#ifndef IO_H_
#define IO_H_

#include <string>
#include "./common.h"

using namespace std;

int readNodesFromFile(const string filepath, vertex_t ** outNodes, int * outCount);

int readEdgesFromFile(const string filepath, vertex_t * nodes, int cntNodes);

#endif  // IO_H_
