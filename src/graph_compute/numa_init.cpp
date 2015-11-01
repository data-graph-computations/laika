/*

#include "numa_init.h"

// core_id = 0, 1, ... n-1, where n is the system's number of cores

int bindThreadToCore(int _coreID) {
   int numCores = sysconf(_SC_NPROCESSORS_ONLN);
   if (_coreID < 0 || _coreID >= numCores)
      return EINVAL;

   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(_coreID, &cpuset);

   pthread_t current_thread = pthread_self();    
   return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

void * writeZeroes(void * param) {
  chunkInit_t * config = (chunkInit_t *) param;
  // Initialize the chunk
  bindThreadToCore(config->coreID);
  size_t chunkSize = config->dataTypeSize << config->numaInit.chunkBits;
  size_t numChunks = (config->numBytes + chunkSize - 1) / chunkSize;
  size_t chunksPerThread = (numChunks + config->numaInit.numWorkers - 1) / config->numaInit.numWorkers;
  size_t start = config->coreID*chunksPerThread*chunkSize;
  size_t end = (config->coreID+1)*chunksPerThread*chunkSize;
  end = MIN(end, config->numBytes);
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
  pthread_t workers[config.numWorkers];

  for (int i = 0; i < config.numWorkers; i++) {
    chunkInit_t chunk(config, i, dataTypeSize, data, numBytes);
    assert(pthread_create(&workers[i], NULL, writeZeroes, &chunk) == NULL)
  }

  for (int i = 0; i < config.numWorkers; i++) {
    assert(pthread_join(&workers[i], NULL) == NULL)
  }
}

void * numaCalloc(numaInit_t config, size_t dataTypeSize, size_t numElements) {
  void * data = malloc(numElements*dataTypeSize);
  madvise(data, numElements*dataTypeSize, MADV_NOHUGEPAGE);
  numaInitWriteZeroes(config, dataTypeSize, data, numElements*dataTypeSize);
  return data;
}

*/
