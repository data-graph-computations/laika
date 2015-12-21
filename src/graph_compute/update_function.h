#ifndef UPDATE_FUNCTION_H_
#define UPDATE_FUNCTION_H_

#include <math.h>
#include <string.h>
#include <string>
#include <algorithm>
#include "./common.h"

WHEN_TEST(
  extern volatile uint64_t roundUpdateCount;
)

using namespace std;

#if EXECUTION_ORDER_SORT

#elif PAGERANK
  #include "./pagerank_struct_defs.h"
#elif MASS_SPRING_DASHPOT
  #include "./msd_struct_defs.h"
#endif

typedef struct data_t data_t;
typedef struct global_t global_t;

typedef struct vertex_t {
  vid_t * edges;
  vid_t cntEdges;
<<<<<<< 87eebf74b485ab27ac7772ed403d186ea5cba52b
#if IN_PLACE
  data_t data;
#else
  data_t data[2];
#endif
#if NEEDS_SCHEDULER_DATA
  sched_t sched;
#endif
};
typedef struct vertex_t vertex_t;

//  Every scheduling algorithm is required to define
//  a sched_t datatype which includes whatever per-vertex data
//  they need to perform their scheduling (e.g., priority, satisfied, dependencies etc.)
#if PAGERANK
  inline static uint64_t hashOfVertexData(data_t * vertex) {
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

  inline static void printConvergenceData(vertex_t * const nodes,
                                          const vid_t cntNodes,
                                          global_t * const globaldata,
                                          int numRounds) {
  #if TEST_CONVERGENCE
    pagerank_t normalizer = 1/globaldata->sumSquareDelta[0];
    for (int i = 0; i < numRounds; i++) {
      cout << sqrt(globaldata->sumSquareDelta[i]*normalizer) << endl;
    }
  #endif
  }

  inline pagerank_t getDelta(vertex_t * const nodes,
                             const vid_t index,
                             global_t * const globaldata) {
    // recalculate this node's pagerank
    pagerank_t pagerank = 0;
    for (vid_t i = 0; i < nodes[index].cntEdges; i++) {
    #if IN_PLACE
      pagerank += nodes[nodes[index].edges[i]].data.contrib;
    #else
      pagerank += nodes[nodes[index].edges[i]].data[0].contrib;
    #endif
    }
    pagerank *= globaldata->d;
    pagerank += (1 - globaldata->d);
  #if IN_PLACE
    return pagerank - nodes[index].data.pagerank;
  #else
    return pagerank - nodes[index].data[0].pagerank;
  #endif
  }

  inline static double getConvergenceData(vertex_t * const nodes,
                                          const vid_t cntNodes,
                                          global_t * const globaldata) {
    pagerank_t sumSquareDelta = 0.0;
    for (vid_t v = 0; v < cntNodes; v++) {
      pagerank_t delta = getDelta(nodes, v, globaldata);
      sumSquareDelta += delta * delta;
    }
    return static_cast<double>(sqrt(sumSquareDelta / static_cast<pagerank_t>(cntNodes)));
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
    globaldata->sumSquareDelta[round] += (delta * delta);
    //  Serial access to globaldata->averageSpeed is essential to
    //  avoid data races.  Access to this variable is serialized
    //  in any case, so parallel computation wouldn't gain much,
    //  if anything.
    assert(PARALLEL == 0);
  #endif
    next->pagerank = pagerank;
    next->contrib = pagerank / static_cast<pagerank_t>(nodes[index].cntEdges);
  }
#elif MASS_SPRING_DASHPOT

  inline static uint64_t hashOfVertexData(data_t * vertex) {
    uint64_t result = 0;
    for (int i = 0; i < DIMENSIONS; i++) {
      uint64_t tmpResult = 0;
      assert(sizeof(phys_t) <= sizeof(uint64_t));
      memcpy(&tmpResult,
             &vertex->position[i],
             sizeof(phys_t));
      result ^= tmpResult;
    }
    return result;
  }

  inline static void printPosition(phys_t (&a)[DIMENSIONS]) {
    cout << a[0];
    for (int i = 1; i < DIMENSIONS; i++) {
      cout << ", " << a[i];
    }
    cout << endl;
  }

  inline static void fixExtremalPoints(vertex_t * const nodes,
                                       const vid_t cntNodes) {
    phys_t maxPoint[DIMENSIONS];
    vid_t maxOwner[DIMENSIONS];
    phys_t minPoint[DIMENSIONS];
    vid_t minOwner[DIMENSIONS];
  #if IN_PLACE
    data_t * node = &nodes[0].data;
  #else
    data_t * node = &nodes[0].data[0];
  #endif
    for (int i = 0; i < DIMENSIONS; i++) {
      maxPoint[i] = node->position[i];
      maxOwner[i] = 0;
      minPoint[i] = node->position[i];
      minOwner[i] = 0;
    }
    for (vid_t v = 0; v < cntNodes; v++) {
    #if IN_PLACE
      node = &nodes[v].data;
    #else
      node = &nodes[v].data[0];
    #endif
      for (int i = 0; i < DIMENSIONS; i++) {
        if (node->position[i] > maxPoint[i]) {
          maxPoint[i] = node->position[i];
          maxOwner[i] = v;
        }
        if (node->position[i] < minPoint[i]) {
          minPoint[i] = node->position[i];
          minOwner[i] = v;
        }
      }
    }
    phys_t maxScale = 0.0;
    for (int i = 0; i < DIMENSIONS; i++) {
    #if IN_PLACE
      nodes[maxOwner[i]].data.fixed = true;
      nodes[minOwner[i]].data.fixed = true;
    #else
      nodes[maxOwner[i]].data[0].fixed = true;
      nodes[minOwner[i]].data[0].fixed = true;
      nodes[maxOwner[i]].data[1].fixed = true;
      nodes[minOwner[i]].data[1].fixed = true;
    #endif
      if ((maxPoint[i] - minPoint[i]) > maxScale) {
        maxScale = maxPoint[i] - minPoint[i];
      }
    }
  #if NORMALIZE_TO_UNIT_CUBE
    phys_t inverseScale = 1 / maxScale;
    for (vid_t i = 0; i < cntNodes; i++) {
      for (int d = 0; d < DIMENSIONS; d++) {
      #if IN_PLACE
        nodes[i].data.position[d]
          = (nodes[i].data.position[d] - minPoint[d])*inverseScale;
      #else
        nodes[i].data[0].position[d]
          = (nodes[i].data[0].position[d] - minPoint[d])*inverseScale;
        nodes[i].data[1].position[d]
          = (nodes[i].data[1].position[d] - minPoint[d])*inverseScale;
      #endif
      }
    }
  #endif
  }

  inline static void fillInNodeData(vertex_t * const nodes,
                                    const vid_t cntNodes,
                                    const string filepath) {
    FILE * nodeInputFile = fopen(filepath.c_str(), "r");
    uint64_t tmpCntNodes, g0, g1, g2;
    int cntItems = fscanf(nodeInputFile, "%" SCNu64 "%" SCNu64 "%" SCNu64 "%" SCNu64 "\n",
      &tmpCntNodes, &g0, &g1, &g2);
    assert(cntItems == 4);
    assert(static_cast<vid_t>(tmpCntNodes) == cntNodes);
    for (vid_t i = 0; i < cntNodes; i++) {
    #if IN_PLACE
      nodes[i].data.fixed = false;
    #else
      nodes[i].data[0].fixed = false;
      nodes[i].data[1].fixed = false;
    #endif
      uint64_t nodeID;
      cntItems = fscanf(nodeInputFile, "%" SCNu64, &nodeID);
      assert(cntItems == 1);
      assert(static_cast<vid_t>(nodeID) == i);
      for (int d = 0; d < DIMENSIONS; d++) {
        double position;
        cntItems = fscanf(nodeInputFile, "%lf ", &position);
        assert(cntItems == 1);
      #if IN_PLACE
        nodes[i].data.velocity[d] = 0;
        nodes[i].data.position[d] = static_cast<phys_t>(position);
      #else
        nodes[i].data[0].velocity[d] = 0;
        nodes[i].data[1].velocity[d] = 0;
        nodes[i].data[0].position[d] = static_cast<phys_t>(position);
        nodes[i].data[1].position[d] = static_cast<phys_t>(position);
      #endif
      }
    }
    fclose(nodeInputFile);
    fixExtremalPoints(nodes, cntNodes);
  }

  const inline phys_t length(phys_t (&a)[DIMENSIONS]) {
    phys_t total = 0;
    for (int i = 0; i < DIMENSIONS; i++) {
      total += a[i]*a[i];
    }
    return sqrt(total);
  }

  const inline phys_t distance(phys_t (&a)[DIMENSIONS], phys_t (&b)[DIMENSIONS]) {
    phys_t delta[DIMENSIONS];
    for (int i = 0; i < DIMENSIONS; i++) {
      delta[i] = a[i] - b[i];
    }
    return length(delta);
  }

  inline static void printConvergenceData(vertex_t * const nodes,
                                          const vid_t cntNodes,
                                          global_t * const globaldata,
                                          int numRounds) {
  #if TEST_CONVERGENCE
    phys_t normalizer = sqrt(static_cast<phys_t>(cntNodes)
      / globaldata->sumSquareNetForce[0]);
    cout << 1.0 << endl;
    for (int i = 1; i < numRounds; i++) {
      cout << (normalizer*sqrt(globaldata->sumSquareNetForce[i]
        / static_cast<phys_t>(cntNodes))) << endl;
    }
  #endif
  }

  inline static void findEdgeLengthHistogram(vertex_t * const nodes,
                                             const vid_t cntNodes,
                                             phys_t * const histogram,
                                             global_t * const globaldata) {
  #if PRINT_EDGE_LENGTH_HISTOGRAM
    const phys_t inverseBucketSize = globaldata->inverseBucketSize;
    for (int i = 0; i < NUM_BUCKETS; i++) {
      histogram[i] = 0.0;
    }
    vid_t cntEdges = 0;
    for (vid_t i = 0; i < cntNodes; i++) {
      for (vid_t edge = 0; edge < nodes[i].cntEdges; edge++) {
        cntEdges++;
        vid_t neighbor = nodes[i].edges[edge];
      #if IN_PLACE
        phys_t edgeLength = distance(nodes[i].data.position,
                                     nodes[neighbor].data.position);
      #else
        phys_t edgeLength = distance(nodes[i].data[0].position,
                                     nodes[neighbor].data[0].position);
      #endif
        phys_t normalized = std::max(static_cast<phys_t>(0.0),
                                     edgeLength*inverseBucketSize);
        int index = std::min(static_cast<int>(normalized), NUM_BUCKETS - 1);
        histogram[index] += 1.0;
      }
    }
    for (int i = 0; i < NUM_BUCKETS; i++) {
      histogram[i] /= static_cast<phys_t>(cntEdges);
    }
  #endif
  }

#if PRINT_EDGE_LENGTH_HISTOGRAM
  inline static void initialEdgeLengthHistogram(vertex_t * const nodes,
                                                const vid_t cntNodes,
                                                global_t * const globaldata) {
    findEdgeLengthHistogram(nodes, cntNodes, globaldata->edgeLengthsBefore, globaldata);
  }
=======
>>>>>>> Split up update_function.h into separate files. Fixed some minor bugs.

  #if IN_PLACE
    data_t data;
  #else
    data_t data[2];
  #endif

  #if NEEDS_SCHEDULER_DATA
    sched_t sched;
  #endif
} vertex_t;

//  Every scheduling algorithm is required to define
//  a sched_t datatype which includes whatever per-vertex data
//  they need to perform their scheduling (e.g., priority, satisfied, dependencies etc.)
#if EXECUTION_ORDER_SORT

#elif PAGERANK
  #include "./pagerank_update_function.h"
#elif MASS_SPRING_DASHPOT
  #include "./msd_update_function.h"
#endif

#endif  // UPDATE_FUNCTION_H_
