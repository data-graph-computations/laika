#ifndef IO_H_
#define IO_H_

#include <string>
#include "./common.h"

using namespace std;

int readEdgesFromFile(const string& filepath,
                      vertex_t ** const outNodes, vid_t * const cntNodes);

#endif  // IO_H_
