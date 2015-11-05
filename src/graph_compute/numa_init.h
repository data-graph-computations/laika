#ifndef NUMA_INIT_H_
#define NUMA_INIT_H_

#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <cassert>
#include <cstring>

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif

struct numaInit_t {
  int numWorkers;
  size_t chunkBits;
  bool numaInitFlag;
  numaInit_t(int _numWorkers, size_t _chunkBits, bool _numaInitFlag) :
    numWorkers(_numWorkers), chunkBits(_chunkBits),
    numaInitFlag(_numaInitFlag) {}
  numaInit_t() {
    numWorkers = 1;
    chunkBits = 0;
    numaInitFlag = false;
  }
};
typedef struct numaInit_t numaInit_t;

struct chunkInit_t {
  numaInit_t numaInit;
  int coreID;
  size_t dataTypeSize;
  void *data;
  size_t numBytes;
  chunkInit_t(numaInit_t _numaInit, int _coreID, size_t _dataTypeSize,
    void *_data, size_t _numBytes) : numaInit(_numaInit), coreID(_coreID),
    dataTypeSize(_dataTypeSize), data(_data), numBytes(_numBytes) {}
};
typedef struct chunkInit_t chunkInit_t;

void numaInitWriteZeroes(numaInit_t config, size_t dataTypeSize,
  void *data, size_t numBytes);

void * numaCalloc(numaInit_t config, size_t dataTypeSize, size_t numElements);

#endif  // NUMA_INIT_H_
