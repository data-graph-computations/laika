#ifndef LOCKS_SCHEDULING_H_
#define LOCKS_SCHEDULING_H_

#if D1_LOCKS

#include <pthread.h>
#include <algorithm>
#include "./common.h"

struct scheddata_t { };
typedef struct scheddata_t scheddata_t;

//  This per-vertex struct includes the vertex's lock
struct sched_t {
  pthread_mutex_t mutex;
};
typedef struct sched_t sched_t;

// update_function.h depends on sched_t being defined
#include "./update_function.h"

static inline void init_scheduling(vertex_t * const nodes,
                                   const vid_t cntNodes,
                                   scheddata_t * const scheddata) {
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    // initialize the mutex for vertex i
    int result = pthread_mutex_init(&nodes[i].sched.mutex, NULL);
    assert(result == 0);

    // sort the neighbor list of vertex i,
    // to allow ordered acquisition of locks and prevent deadlocks
    stable_sort(nodes[i].edges, nodes[i].edges + nodes[i].cntEdges);
  }
}

static inline void acquire_locks(vertex_t * const nodes,
                                 const vid_t cntNodes,
                                 const vid_t currentIndex) {
  vertex_t * const current = &nodes[currentIndex];
  int result;
  vid_t nextNeighbor;

  // lock all neighbors with IDs lower than currentIndex
  for (nextNeighbor = 0;
       nextNeighbor < current->cntEdges && current->edges[nextNeighbor] <= currentIndex;
       ++nextNeighbor) {
    pthread_mutex_t * const mutex = &nodes[current->edges[nextNeighbor]].sched.mutex;
    result = pthread_mutex_lock(mutex);
    assert(result == 0);
    ++nextNeighbor;
  }

  // lock the current vertex
  result = pthread_mutex_lock(&current->sched.mutex);
  assert(result == 0);

  // lock all remaining neighbors
  for (; nextNeighbor < current->cntEdges; ++nextNeighbor) {
    pthread_mutex_t * const mutex = &nodes[current->edges[nextNeighbor]].sched.mutex;
    result = pthread_mutex_lock(mutex);
    assert(result == 0);
  }
}

static inline void release_locks(vertex_t * const nodes,
                                 const vid_t cntNodes,
                                 const vid_t currentIndex) {
  vertex_t * const current = &nodes[currentIndex];
  int result;
  vid_t nextNeighbor;

  // lock all neighbors with IDs lower than currentIndex
  for (nextNeighbor = 0;
       nextNeighbor < current->cntEdges && current->edges[nextNeighbor] <= currentIndex;
       ++nextNeighbor) {
    pthread_mutex_t * const mutex = &nodes[current->edges[nextNeighbor]].sched.mutex;
    result = pthread_mutex_unlock(mutex);
    assert(result == 0);
    ++nextNeighbor;
  }

  // lock the current vertex
  result = pthread_mutex_unlock(&current->sched.mutex);
  assert(result == 0);

  // lock all remaining neighbors
  for (; nextNeighbor < current->cntEdges; ++nextNeighbor) {
    pthread_mutex_t * const mutex = &nodes[current->edges[nextNeighbor]].sched.mutex;
    result = pthread_mutex_unlock(mutex);
    assert(result == 0);
  }
}

static inline void execute_rounds(const int numRounds,
                                  vertex_t * const nodes,
                                  const vid_t cntNodes,
                                  scheddata_t * const scheddata,
                                  global_t * const globaldata) {
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  for (int round = 0; round < numRounds; ++round) {
    WHEN_DEBUG({
      cout << "Running locks round " << round << endl;
    })

    cilk_for (vid_t i = 0; i < cntNodes; ++i) {
      acquire_locks(nodes, cntNodes, i);
      update(nodes, i, globaldata, round);
      release_locks(nodes, cntNodes, i);
    }
  }
}

static inline void cleanup_scheduling(vertex_t * const nodes,
                                      const vid_t cntNodes,
                                      scheddata_t * const scheddata) {
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    int result = pthread_mutex_destroy(&nodes[i].sched.mutex);
    assert(result == 0);
  }
}

static inline void print_execution_data() {
  // no-op
}

#endif  // D1_LOCKS

#endif  // LOCKS_SCHEDULING_H_
