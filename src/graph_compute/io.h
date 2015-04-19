#ifndef IO_H_
#define IO_H_

#include <string>
#include "./common.h"

using namespace std;

int readEdgesFromFile(const string filepath, vertex_t ** outNodes, int * outCntNodes);

#endif  // IO_H_
