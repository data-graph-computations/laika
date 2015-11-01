#ifndef NUMA_INIT_H_
#define NUMA_INIT_H_

#include <string>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <sched.h>
#include <unistd.h>
#include <pthread.h>
#include "./io.h"
#include "./common.h"
#include "../libgraphio/libgraphio.h"

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE 
#endif

typedef struct numaInit_t {
  size_t chunkBits;
  int numWorkers;
  bool numaInitFlag;
  numaInit_t(int _numWorkers, size_t _chunkBits, bool _numaInitFlag) : numaWorkers(_numWorkers), chunkBits(_chunkBits), numaInitFlag(_numaInitFlag) {};
};

typedef struct chunkInit_t {
  numaInit_t numaInit;
  int coreID;
  size_t dataTypeSize;
  void *data;
  size_t numBytes;
  chunkInit_t(numaInit_t _numaInit, int _coreID, size_t _dataTypeSize, void *_data, size_t _numBytes) : numaInit(_numaInit), coreID(_coreID), dataTypeSize(_dataTypeSize), data(_data), numBytes(_numBytes) {};
};

int numaInitWriteZeroes(numaInit_t config, size_t dataTypeSize, void *data, size_t length);

#endif // NUMA_INIT_H_
