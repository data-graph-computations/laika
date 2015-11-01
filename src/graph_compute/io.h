#ifndef IO_H_
#define IO_H_

#include <sys/mman.h>
#include <pthread.h>
#include <string>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <iostream>
#include "./common.h"
#include "../libgraphio/libgraphio.h"
#include "./numa_init.h"

using namespace std;

int readEdgesFromFile(const string filepath, vertex_t ** outNodes,
                      vid_t * outCntNodes, numaInit_t numaInit);

#endif  // IO_H_
