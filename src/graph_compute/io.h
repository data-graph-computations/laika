#ifndef IO_H_
#define IO_H_

#include <string>
#include "./common.h"

using namespace std;

int readEdgesFromFile(const string filepath, vertex_t ** outNodes, vid_t * outCntNodes);

#endif  // IO_H_
