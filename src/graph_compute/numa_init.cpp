#include "./numa_init.h"
#include <iostream>

void * writeZeroes(void * param) {
  chunkInit_t * config = static_cast<chunkInit_t *>(param);
  // Initialize the chunk
  bindThreadToCore(config->coreID);
  size_t chunkSize = config->dataTypeSize << config->numaInit.chunkBits;
  size_t numChunks = (config->numBytes + chunkSize - 1) / chunkSize;
  size_t chunksPerThread = (numChunks + config->numaInit.numWorkers - 1)
    / config->numaInit.numWorkers;
  size_t start = config->coreID*chunksPerThread*chunkSize;
  size_t end = (config->coreID+1)*chunksPerThread*chunkSize;
  end = (config->numBytes < end) ? config->numBytes : end;
  unsigned char * data = static_cast<unsigned char *>(config->data);
  for (size_t i = start; i < end; i++) {
    data[i] = 0;
  }
  return NULL;
}

void numaInitWriteZeroes(numaInit_t config,
                         size_t dataTypeSize,
                         void *data,
                         size_t numBytes) {
  pthread_t * workers =
    static_cast<pthread_t *>(malloc(sizeof(pthread_t)*config.numWorkers));
  for (int i = 0; i < config.numWorkers; i++) {
    chunkInit_t chunk(config, i, dataTypeSize, data, numBytes);
    assert(pthread_create(&workers[i], NULL, writeZeroes, &chunk) == 0);
  }
  for (int i = 0; i < config.numWorkers; i++) {
    assert(pthread_join(workers[i], NULL) == 0);
  }
}

void * numaCalloc(numaInit_t config, size_t dataTypeSize, size_t numElements) {
  void * data = malloc(numElements*dataTypeSize);
  madvise(data, numElements*dataTypeSize, MADV_NOHUGEPAGE);
  if (config.numaInitFlag == true) {
    numaInitWriteZeroes(config, dataTypeSize, data, numElements*dataTypeSize);
  } else {
    memset(data, 0, numElements*dataTypeSize);
  }
  return data;
}
