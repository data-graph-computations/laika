#ifndef PAGERANK_UPDATE_FUNCTION_H_
#define PAGERANK_UPDATE_FUNCTION_H_

#include <cmath>
#include <cstring>
#include <string>
#include <algorithm>
#include "./common.h"

using namespace std;

inline static uint64_t hashOfVertexData(const data_t * vertex) {
  uint64_t result = 0;
  assert(sizeof(pagerank_t) <= sizeof(uint64_t));
  memcpy(&result,
         &vertex->pagerank,
         sizeof(pagerank_t));
  return result;
}

inline static void fillInNodeData(vertex_t * const nodes,
                                  const vid_t cntNodes) {
  for (vid_t i = 0; i < cntNodes; i++) {
  #if IN_PLACE
    nodes[i].data.pagerank = 1.0;
    nodes[i].data.contrib = 1.0/static_cast<pagerank_t>(nodes[i].cntEdges);
  #else
    nodes[i].data[0].pagerank = 1.0;
    nodes[i].data[0].contrib = 1.0/static_cast<pagerank_t>(nodes[i].cntEdges);
    nodes[i].data[1].pagerank = 1.0;
    nodes[i].data[1].contrib = 1.0/static_cast<pagerank_t>(nodes[i].cntEdges);
  #endif
  }
}

inline static void fillInGlobalData(vertex_t * const nodes,
                                    const vid_t cntNodes,
                                    global_t * const globaldata,
                                    int numRounds) {
  globaldata->d = .85;
#if TEST_CONVERGENCE
  globaldata->averageDiff = new (std::nothrow) pagerank_t[numRounds]();
#endif
}

inline static void printConvergenceData(const vertex_t * const nodes,
                                        const vid_t cntNodes,
                                        const global_t * const globaldata,
                                        const int numRounds) {
#if TEST_CONVERGENCE
  pagerank_t normalizer = 1/globaldata->sumSquareDelta[0];
  for (int i = 0; i < numRounds; i++) {
    cout << sqrt(globaldata->sumSquareDelta[i]*normalizer) << endl;
  }
#endif
}

static inline pagerank_t getDelta(const vertex_t * const nodes,
                                  const vid_t index,
                                  const global_t * const globaldata,
                                  const int round) {
  // recalculate this node's pagerank
  pagerank_t pagerank = 0;
  for (vid_t i = 0; i < nodes[index].cntEdges; i++) {
    #if IN_PLACE
      pagerank += nodes[nodes[index].edges[i]].data.contrib;
    #else
      pagerank += nodes[nodes[index].edges[i]].data[round & 1].contrib;
    #endif
  }

  pagerank *= globaldata->d;
  pagerank += (1 - globaldata->d);

  #if IN_PLACE
    return pagerank - nodes[index].data.pagerank;
  #else
    return pagerank - nodes[index].data[round & 1].pagerank;
  #endif
}

static inline double getConvergenceData(const vertex_t * const nodes,
                                        const vid_t cntNodes,
                                        const global_t * const globaldata,
                                        const int round) {
  pagerank_t sumSquareDelta = 0.0;
  for (vid_t v = 0; v < cntNodes; v++) {
    pagerank_t delta = getDelta(nodes, v, globaldata, round);
    sumSquareDelta += delta * delta;
  }
  return static_cast<double>(sqrt(sumSquareDelta / static_cast<pagerank_t>(cntNodes)));
}

static inline double getInitialConvergenceData(const vertex_t * const nodes,
                                               const vid_t cntNodes,
                                               const global_t * const globaldata) {
  return getConvergenceData(nodes, cntNodes, globaldata, 0);
}

inline void update(vertex_t * const nodes,
                   const vid_t index,
                   global_t * const globaldata,
                   const int round = 0) {
  // ensuring that the number of updates in total is correct per round
  WHEN_TEST({
    __sync_add_and_fetch(&roundUpdateCount, 1);
  })
#if IN_PLACE
  data_t * next = &nodes[index].data;
#else
  data_t * next = &nodes[index].data[(round + 1) & 1];
#endif

  // recalculate this node's pagerank
  pagerank_t pagerank = 0;
  for (vid_t i = 0; i < nodes[index].cntEdges; i++) {
  #if IN_PLACE
    pagerank += nodes[nodes[index].edges[i]].data.contrib;
  #else
    pagerank += nodes[nodes[index].edges[i]].data[round & 1].contrib;
  #endif
  }
  pagerank *= globaldata->d;
  pagerank += (1 - globaldata->d);
#if TEST_CONVERGENCE
#if IN_PLACE
  data_t * current = &nodes[index].data;
#else
  data_t * current = &nodes[index].data[round & 1];
#endif
  pagerank_t delta = pagerank - current->pagerank;
  globaldata->sumSquareDelta[round] += delta;
  //  Serial access to globaldata->averageSpeed is essential to
  //  avoid data races.  Access to this variable is serialized
  //  in any case, so parallel computation wouldn't gain much,
  //  if anything.
  assert(PARALLEL == 0);
#endif
  next->pagerank = pagerank;
  next->contrib = pagerank / static_cast<pagerank_t>(nodes[index].cntEdges);
}

#endif  // PAGERANK_UPDATE_FUNCTION_H_
