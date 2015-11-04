#ifndef UPDATE_FUNCTION_H_
#define UPDATE_FUNCTION_H_

#include <math.h>

WHEN_TEST(
  static uint64_t roundUpdateCount = 0;
)

#if PAGERANK
  typedef double pagerank_t;
  struct data_t {
    pagerank_t pagerank;
    pagerank_t contrib;
  #if TEST_CONVERGENCE
    pagerank_t pagerankOld;
  #endif
  };
  struct global_t { };
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
  };
  struct global_t {
    phys_t restLength;
    phys_t springStiffness;
    phys_t timeStep;
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
// struct vertex_t {
//   vid_t id;
//   vid_t priority;
//   vid_t dependencies;
//   volatile vid_t satisfied;
//   vid_t cntEdges;
//   vid_t * edges;
//   double data;
// };

//  Every scheduling algorithm is required to define
//  a sched_t datatype which includes whatever per-vertex data
//  they need to perform their scheduling (e.g., priority, satisfied, dependencies etc.)
#if PAGERANK
  static void fillInNodeData(vertex_t * nodes, const vid_t cntNodes) {
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

  void update(vertex_t * nodes, const vid_t index,
              const global_t global, const int round = 0) {
    // ensuring that the number of updates in total is correct per round
    WHEN_TEST({
      __sync_add_and_fetch(&roundUpdateCount, 1);
    })

    vertex_t * current = &nodes[index];
    // recalculate this node's data
    pagerank_t pagerank = 0;
    for (size_t i = 0; i < current->cntEdges; ++i) {
    #if IN_PLACE
      pagerank += nodes[current->edges[i]].data.contrib;
    #else
      pagerank += nodes[current->edges[i]].data[round & 1].contrib;
    #endif
    }
  #if IN_PLACE
    #if TEST_CONVERGENCE
      nodes[index].data.pagerankOld = nodes[index].data.pagerank;
    #endif
    nodes[index].data.pagerank = pagerank;
    nodes[index].data.contrib = pagerank/static_cast<pagerank_t>(current->cntEdges);
  #else
    nodes[index].data[(round+1) & 1].pagerank = pagerank;
    nodes[index].data[(round+1) & 1].contrib =
      pagerank/static_cast<pagerank_t>(current->cntEdges);
  #endif
  }
#else  // MASS_SPRING_DASHPOT
//
// from http://www.codeproject.com/Articles/69941/Best-Square-Root-Method-Algorithm-Function-Precisi
// store dot product of each vertex in vertex struct
// then turn sqrt((x0-x1)^2 + (y0-y1)^2 + (z0-z1)^2) into:
// sqrt((x0*x0+y0*y0+z0*z0) + (x1*x1+y1*y1+z1*z1) - 2(x0*x1+y0*y1+z0*z1))
// double inline __declspec (naked) __fastcall sqrt14(double n) {
//     _asm fld qword ptr [esp+4]  // load parameter n into a FP register
//     _asm fsqrt // do sqrt() on that register, and store the result back there
//     _asm ret 8 // return, and pop an extra 8 bytes (the n parameter) off the stack
// }
// const inline phys_t sqrt(phys_t a) {
//   return sqrt(a);
// }

  const inline phys_t length(phys_t (&a)[DIMENSIONS]) {
    phys_t total = 0;
    for (int i = 0; i < DIMENSIONS; i++) {
      total += a[i]*a[i];
    }
    return sqrt(total);
  }

  const inline void spring_force(phys_t (&a)[DIMENSIONS],
                                 phys_t (&b)[DIMENSIONS],
                                 phys_t (&delta)[DIMENSIONS],
                                 const global_t global) {
    for (int i = 0; i < DIMENSIONS; i++) {
      delta[i] = a[i] - b[i];
    }
    phys_t coefficient = global.restLength - length(delta);
    for (int i = 0; i < DIMENSIONS; i++) {
      delta[i] *= coefficient;
    }
  }

  void update(vertex_t * nodes, const vid_t index,
              const global_t global, const int round = 0) {
    // ensuring that the number of updates in total is correct per round
    WHEN_TEST({
      __sync_add_and_fetch(&roundUpdateCount, 1);
    })

  #if IN_PLACE
    data_t * current = &nodes[index].data;
    data_t * next = &nodex[index].data;
  #else
    data_t * current = &nodes[index].data[round & 1];
    data_t * next = &nodex[index].data[(round + 1) & 1];
  #endif

    phys_t acceleration[DIMENSIONS];
    for (int d = 0; d < DIMENSIONS; d++) {
      acceleration[d] = 0;
    }
    for (vid_t i = 0; i < nodes[index].cntEdges; i++) {
  #if IN_PLACE
      data_t * neighbor = &node[node[index].edges[i]].data;
  #else
      data_t * neighbor = &node[node[index].edges[i]].data[round & 1];
  #endif
      phys_t delta[DIMENSIONS];
      spring_force(current->position, neighbor->position, delta, global);
      for (int d = 0; d < DIMENSIONS; d++) {
        acceleration[d] += delta[d];
      }
    }
    for (int d = 0; d < DIMENSIONS; d++) {
      acceleration[d] *= global.springStiffness;
      acceleration[d] += -current->dashpotResistance*current->velocity[d];
      next->velocity[d] += global.timeStep*acceleration[d];
      next->position[d] += global.timeStep*next->velocity[d];
    }
  }

  phys_t average_speed(vertex_t * nodes, const vid_t cntNodes) {
    phys_t speed = 0.0;
    for (vid_t i = 0; i < cntNodes; i++) {
      speed += length(nodes[i].data.velocity);
    }
    return speed/static_cast<phys_t>(cntNodes);
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
