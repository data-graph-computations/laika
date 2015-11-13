#ifndef UPDATE_FUNCTION_H_
#define UPDATE_FUNCTION_H_

#include <math.h>
//  #include <stdint.h>
#include <string.h>
#include <string>
#include "./common.h"

WHEN_TEST(
  extern volatile uint64_t roundUpdateCount;
)

using namespace std;

#if PAGERANK
  typedef double pagerank_t;
  struct data_t {
    pagerank_t pagerank;
    pagerank_t contrib;
  };
  struct global_t {
    pagerank_t d;
  #if TEST_CONVERGENCE
    pagerank_t * averageDiff;
  #endif
  };
#else  //  MASS_SPRING_DASHPOT - will be initialized in fillInNodeData
  #ifndef DIMENSIONS
    #define DIMENSIONS 3
  #endif
  typedef float phys_t;
  struct data_t {
    phys_t position[DIMENSIONS];
    phys_t velocity[DIMENSIONS];
    phys_t dashpotResistance;
    phys_t mass;
    bool fixed;
  };
  struct global_t {
    phys_t restLength;
    phys_t springStiffness;
    phys_t timeStep;
  #if TEST_CONVERGENCE
    phys_t * averageSpeed;
  #endif
  };
#endif
typedef struct data_t data_t;
typedef struct global_t global_t;

struct vertex_t {
  vid_t *edges;
  vid_t cntEdges;
#if IN_PLACE
  data_t data;
#else
  data_t data[2];
#endif
  sched_t sched;
};
typedef struct vertex_t vertex_t;

//  Every scheduling algorithm is required to define
//  a sched_t datatype which includes whatever per-vertex data
//  they need to perform their scheduling (e.g., priority, satisfied, dependencies etc.)
#if PAGERANK
  inline static uint64_t hashOfVertexData(data_t * vertex) {
    uint64_t result = 0;
    memcpy(static_cast<void *>(&result),
           static_cast<void *>(&vertex->pagerank),
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
    for (int i = 0; i < numRounds; i++) {
      cout << globaldata->averageDiff[i] << endl;
    }
  #endif
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
    pagerank_t averageDiff = std::abs(pagerank - current->pagerank);
    globaldata->averageDiff[round] += averageDiff;
    assert(PARALLEL == 0);
  #endif
    next->pagerank = pagerank;
    next->contrib = pagerank / static_cast<pagerank_t>(nodes[index].cntEdges);
  }
#else  // MASS_SPRING_DASHPOT
  inline static uint64_t hashOfVertexData(data_t * vertex) {
    uint64_t result = 0;
    for (int i = 0; i < DIMENSIONS; i++) {
      uint64_t tmpResult = 0;
      memcpy(static_cast<void *>(&tmpResult),
             static_cast<void *>(&vertex->position[i]),
             sizeof(phys_t));
      result ^= tmpResult;
    }
    return result;
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
      data_t * node = &nodes[v].data;
    #else
      data_t * node = &nodes[v].data[0];
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
      }
    }
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
    phys_t mass = 1.0;
    phys_t dashpotResistance = 0.5;
    for (vid_t i = 0; i < cntNodes; i++) {
    #if IN_PLACE
      nodes[i].data.dashpotResistance = dashpotResistance;
      nodes[i].data.mass = mass;
      nodes[i].data.fixed = false;
    #else
      nodes[i].data[0].dashpotResistance = dashpotResistance;
      nodes[i].data[0].mass = mass;
      nodes[i].data[0].fixed = false;
      nodes[i].data[1].dashpotResistance = dashpotResistance;
      nodes[i].data[1].mass = mass;
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

  inline static void fillInGlobalData(vertex_t * const nodes,
                                      const vid_t cntNodes,
                                      global_t * const globaldata,
                                      int numRounds) {
    phys_t totalLength = 0;
    vid_t cntEdges = 0;
    for (vid_t i = 0; i < cntNodes; i++) {
      for (vid_t edge = 0; edge < nodes[i].cntEdges; edge++) {
        cntEdges++;
      #if IN_PLACE
        totalLength += distance(nodes[i].data.position, nodes[edge].data.position);
      #else
        totalLength += distance(nodes[i].data[0].position, nodes[edge].data[0].position);
      #endif
      }
    }
    globaldata->restLength = totalLength / static_cast<phys_t>(cntEdges);
    globaldata->springStiffness = 1;
    globaldata->timeStep = 0.01;
  #if TEST_CONVERGENCE
    globaldata->averageSpeed = new (std::nothrow) phys_t[numRounds]();
  #endif
  }

  inline static void printConvergenceData(vertex_t * const nodes,
                                          const vid_t cntNodes,
                                          global_t * const globaldata,
                                          int numRounds) {
  #if TEST_CONVERGENCE
    for (int i = 0; i < numRounds; i++) {
      cout << sqrt(globaldata->averageSpeed[i] / static_cast<phys_t>(cntNodes)) << endl;
    }
  #endif
  }

  const inline void springForce(phys_t (&a)[DIMENSIONS],
                                 phys_t (&b)[DIMENSIONS],
                                 phys_t (&delta)[DIMENSIONS],
                                 global_t * const globaldata) {
    for (int i = 0; i < DIMENSIONS; i++) {
      delta[i] = a[i] - b[i];
    }
    phys_t coefficient = globaldata->restLength - length(delta);
    for (int i = 0; i < DIMENSIONS; i++) {
      delta[i] *= coefficient;
    }
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
    if (nodes[index].data.fixed) {
      return;
    }
    data_t * current = &nodes[index].data;
    data_t * next = &nodes[index].data;
  #else
    if (nodes[index].data[0].fixed) {
      return;
    }
    data_t * current = &nodes[index].data[round & 1];
    data_t * next = &nodes[index].data[(round + 1) & 1];
  #endif

    phys_t acceleration[DIMENSIONS];
    for (int d = 0; d < DIMENSIONS; d++) {
      acceleration[d] = 0;
    }
    for (vid_t i = 0; i < nodes[index].cntEdges; i++) {
  #if IN_PLACE
      data_t * neighbor = &nodes[nodes[index].edges[i]].data;
  #else
      data_t * neighbor = &nodes[nodes[index].edges[i]].data[round & 1];
  #endif
      phys_t delta[DIMENSIONS];
      springForce(current->position, neighbor->position, delta, globaldata);
      for (int d = 0; d < DIMENSIONS; d++) {
        acceleration[d] += delta[d] / current->mass;
      }
    }
    for (int d = 0; d < DIMENSIONS; d++) {
      acceleration[d] *= globaldata->springStiffness;
      //  acceleration[d] -= current->dashpotResistance*current->velocity[d];
      next->velocity[d] += globaldata->timeStep*acceleration[d];
      next->velocity[d] *= (1 - current->dashpotResistance);
      next->position[d] += globaldata->timeStep*next->velocity[d];
    }
  #if TEST_CONVERGENCE
    phys_t averageSpeed = length(next->velocity);
    globaldata->averageSpeed[round] += averageSpeed*averageSpeed;
    assert(PARALLEL == 0);
  #endif
  }
#endif

// 1. Calculate the net force on the vertex F: this is the sum of the
// forces applied by the springs between neighbors - this is the
// instantaneous force calculated based on the vertex / neighbor current
// positions.  Then, add the dashpot force, which is just the negative
// current velocity, times a choosable constant.
// 2. Update the velocity
// v += k1*F*dt, where k1 is a choosable constant and dt is the size of
// the time step.
// 3. Update the position p += k2*v*dt, where k2 is a
// choosable constant.

// Step 1 dominates the work -

// F(w) = -k0*v(w) + sum_{u in N(w)} (L0-|p(u)-p(w)|)*(p(u)-p(w))

// The term L0-|p(u)-p(w)| involves doing a cartesian distance - at first
// glance, three square operations and a square root.  Technically,
// though, this is linear in the data, but it will be important to make
// this calculation fast.  After all, we are not animals.

#endif  // UPDATE_FUNCTION_H_
