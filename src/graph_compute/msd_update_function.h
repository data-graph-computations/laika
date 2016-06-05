#ifndef MSD_UPDATE_FUNCTION_H_
#define MSD_UPDATE_FUNCTION_H_

#include <cmath>
#include <cstring>
#include <string>
#include <algorithm>
#include "./common.h"

using namespace std;

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

static inline uint64_t hashOfVertexData(const data_t * vertex) {
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

static inline void printPosition(phys_t (&a)[DIMENSIONS]) {
  cout << a[0];
  for (int i = 1; i < DIMENSIONS; i++) {
    cout << ", " << a[i];
  }
  cout << endl;
}

static inline void fixExtremalPoints(vertex_t * const nodes,
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

static inline void fillInNodeData(vertex_t * const nodes,
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

const inline phys_t length(const phys_t (&a)[DIMENSIONS]) {
  phys_t total = 0;
  for (int i = 0; i < DIMENSIONS; i++) {
    total += a[i]*a[i];
  }
  return sqrt(total);
}

const inline phys_t distance(const phys_t (&a)[DIMENSIONS],
                             const phys_t (&b)[DIMENSIONS]) {
  phys_t delta[DIMENSIONS];
  for (int i = 0; i < DIMENSIONS; i++) {
    delta[i] = a[i] - b[i];
  }
  return length(delta);
}

static inline void printConvergenceData(const vertex_t * const nodes,
                                        const vid_t cntNodes,
                                        const global_t * const globaldata,
                                        const int numRounds) {
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

static inline void findEdgeLengthHistogram(vertex_t * const nodes,
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
static inline void initialEdgeLengthHistogram(vertex_t * const nodes,
                                              const vid_t cntNodes,
                                              global_t * const globaldata) {
  findEdgeLengthHistogram(nodes, cntNodes, globaldata->edgeLengthsBefore, globaldata);
}

static inline void finalEdgeLengthHistogram(vertex_t * const nodes,
                                            const vid_t cntNodes,
                                            global_t * const globaldata) {
  findEdgeLengthHistogram(nodes, cntNodes, globaldata->edgeLengthsAfter, globaldata);
}
#endif

static inline void printEdgeLengthHistograms(vertex_t * const nodes,
                                            const vid_t cntNodes,
                                            global_t * const globaldata) {
#if PRINT_EDGE_LENGTH_HISTOGRAM
  phys_t bucketSize = 1 / globaldata->inverseBucketSize;
  phys_t currentBucket = bucketSize / 2.0;
  for (int i = 0; i < NUM_BUCKETS; i++) {
    cout << currentBucket << ", ";
    currentBucket += bucketSize;
    cout << globaldata->edgeLengthsBefore[i] << ", ";
    cout << globaldata->edgeLengthsAfter[i] << endl;
  }
#endif
}

const inline void springForce(const phys_t (&a)[DIMENSIONS],
                              const phys_t (&b)[DIMENSIONS],
                              phys_t (&delta)[DIMENSIONS],
                              const phys_t restLength) {
  for (int d = 0; d < DIMENSIONS; ++d) {
    delta[d] = a[d] - b[d];
  }
  phys_t coefficient = (restLength - length(delta)) / restLength;
  for (int i = 0; i < DIMENSIONS; ++i) {
    delta[i] *= coefficient;
  }
}

static inline phys_t springInternalEnergy(const phys_t (&a)[DIMENSIONS],
                                          const phys_t (&b)[DIMENSIONS],
                                          const phys_t restLength,
                                          const phys_t springStiffness) {
  phys_t delta[DIMENSIONS];
  for (int d = 0; d < DIMENSIONS; ++d) {
    delta[d] = a[d] - b[d];
  }
  phys_t springCompression = restLength - length(delta);

  // spring internal energy = 1/2 * k * X^2
  // where k is the Hooke constant of the spring,
  // and X is the displacement from the "relaxed" length of the spring
  return 0.5 * springStiffness * springCompression * springCompression;
}

static inline void getNetForce(const vertex_t * const nodes,
                               const vid_t index,
                               phys_t (&acceleration)[DIMENSIONS],
                               const global_t * const globaldata,
                               const int round = 0,
                               const bool leapfrog = false) {
#if IN_PLACE
  const data_t * current = &nodes[index].data;
#else
  const data_t * current = &nodes[index].data[round & 1];
#endif
  phys_t myPosition[DIMENSIONS];
  for (int d = 0; d < DIMENSIONS; d++) {
    myPosition[d] = current->position[d];
    if (leapfrog) {
      myPosition[d] += current->velocity[d]*globaldata->timeStep*0.5;
    }
    acceleration[d] = 0;
  }
  for (vid_t i = 0; i < nodes[index].cntEdges; i++) {
#if IN_PLACE
    const data_t * neighbor = &nodes[nodes[index].edges[i]].data;
#else
    const data_t * neighbor = &nodes[nodes[index].edges[i]].data[round & 1];
#endif
    phys_t position[DIMENSIONS];
    for (int d = 0; d < DIMENSIONS; d++) {
      position[d] = neighbor->position[d];
      if (leapfrog) {
        position[d] += neighbor->velocity[d]*globaldata->timeStep*0.5;
      }
    }
    phys_t delta[DIMENSIONS];
    springForce(myPosition, position, delta, current->restLength);
    for (int d = 0; d < DIMENSIONS; d++) {
      acceleration[d] += (delta[d] * globaldata->inverseMass);
    }
  }
}

static inline double getConvergenceData(const vertex_t * const nodes,
                                        const vid_t cntNodes,
                                        const global_t * const globaldata,
                                        const int round,
                                        const bool includeSpringEnergy = true) {
  double totalEnergy = 0.0;
  for (vid_t i = 0; i < cntNodes; ++i) {
    const vertex_t& current = nodes[i];

    #if IN_PLACE
      const data_t& currentData = current.data;
    #else
      const data_t& currentData = current.data[round & 1];
    #endif

    // add the vertex's energy due to velocity: 1/2 * mv^2
    // since we store inverse mass, we divide by the inverse mass to multiply by mass
    const phys_t v = static_cast<double>(length(currentData.velocity));
    totalEnergy += (v * v) / (2 * globaldata->inverseMass);

    if (includeSpringEnergy) {
      // add the spring energy of all springs between
      // the current vertex and its neighbors of higher ID number
      // (to only count once -- there are no self-edges)
      for (vid_t j = 0; j < current.cntEdges; ++j) {
        const vid_t neighborId = current.edges[j];
        if (neighborId > i) {
          const vertex_t& neighbor = nodes[neighborId];

          #if IN_PLACE
            const data_t& neighborData = neighbor.data;
          #else
            const data_t& neighborData = neighbor.data[round & 1];
          #endif

          const phys_t springEnergy = springInternalEnergy(currentData.position,
                                                           neighborData.position,
                                                           globaldata->restLength,
                                                           globaldata->springStiffness);
          totalEnergy += static_cast<double>(springEnergy);
        }
      }
    }
  }
  return totalEnergy;
}

static inline double getInitialConvergenceData(const vertex_t * const nodes,
                                               const vid_t cntNodes,
                                               const global_t * const globaldata) {
  return getConvergenceData(nodes, cntNodes, globaldata, 0, true);
}

static inline double getForceBasedConvergenceData(const vertex_t * const nodes,
                                                  const vid_t cntNodes,
                                                  const global_t * const globaldata) {
  phys_t meanSquare = 0.0;
  for (vid_t v = 0; v < cntNodes; v++) {
    phys_t acceleration[DIMENSIONS];
    getNetForce(nodes, v, acceleration, globaldata);
    phys_t tmpMeanSquare = length(acceleration);
    meanSquare += tmpMeanSquare*tmpMeanSquare;
  }
  return static_cast<double>(sqrt(meanSquare / static_cast<phys_t>(cntNodes)));
}

static inline void fillInGlobalData(vertex_t * const nodes,
                                    const vid_t cntNodes,
                                    global_t * const globaldata,
                                    int numRounds) {
  globaldata->timeStep = 0.1;
  globaldata->springStiffness = 1.0;
  globaldata->dashpotResistance = 1.0;
  globaldata->inverseMass = 1.0;
#if TEST_CONVERGENCE
  globaldata->sumSquareNetForce = new (std::nothrow) phys_t[numRounds]();
#endif
  phys_t globalRestLength = 0.0;
  vid_t globalCntEdges = 0;
  for (vid_t i = 0; i < cntNodes; i++) {
    phys_t restLength = 0.0;
    vid_t cntEdges = 0;
    for (vid_t edge = 0; edge < nodes[i].cntEdges; edge++) {
      cntEdges++;
      vid_t neighbor = nodes[i].edges[edge];
    #if IN_PLACE
      restLength += distance(nodes[i].data.position,
                             nodes[neighbor].data.position);
    #else
      restLength += distance(nodes[i].data[0].position,
                             nodes[neighbor].data[0].position);
    #endif
    }
    globalRestLength += restLength;
    globalCntEdges += cntEdges;
  #if IN_PLACE
    nodes[i].data.restLength = restLength / static_cast<phys_t>(cntEdges);
  #else
    nodes[i].data[0].restLength = restLength / static_cast<phys_t>(cntEdges);
    nodes[i].data[1].restLength = restLength / static_cast<phys_t>(cntEdges);
  #endif
  }
  globaldata->restLength = globalRestLength / static_cast<phys_t>(globalCntEdges);
#if USE_GLOBAL_REST_LENGTH
  for (vid_t i = 0; i < cntNodes; i++) {
  #if IN_PLACE
    nodes[i].data.restLength = globaldata->restLength;
  #else
    nodes[i].data[0].restLength = globaldata->restLength;
    nodes[i].data[1].restLength = globaldata->restLength;
  #endif
  }
#endif
#if PRINT_EDGE_LENGTH_HISTOGRAM
  globaldata->edgeLengthsBefore = new (std::nothrow) phys_t[NUM_BUCKETS];
  globaldata->edgeLengthsAfter = new (std::nothrow) phys_t[NUM_BUCKETS];
  globaldata->inverseBucketSize = NUM_BUCKETS / (2*globaldata->restLength);
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
  static const bool useLeapFrogMethod = true;
  getNetForce(nodes, index, acceleration, globaldata, round, useLeapFrogMethod);
#if TEST_CONVERGENCE
  phys_t tmpNetForce = length(acceleration);
  globaldata->sumSquareNetForce[round] += tmpNetForce*tmpNetForce;
  //  Serial access to globaldata->averageSpeed is essential to
  //  avoid data races.  Access to this variable is serialized
  //  in any case, so parallel computation wouldn't gain much,
  //  if anything.
  assert(PARALLEL == 0);
#endif
  for (int d = 0; d < DIMENSIONS; d++) {
    acceleration[d] *= globaldata->springStiffness;
    acceleration[d] -= globaldata->dashpotResistance*current->velocity[d];
    next->velocity[d] = acceleration[d]*globaldata->timeStep + current->velocity[d];
    next->position[d] = current->position[d] + next->velocity[d]*globaldata->timeStep;
  }
}

#endif  // MSD_UPDATE_FUNCTION_H_
