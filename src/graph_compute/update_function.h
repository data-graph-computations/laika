#ifndef UPDATE_FUNCTION_H_
#define UPDATE_FUNCTION_H_

WHEN_TEST(
  static uint64_t roundUpdateCount = 0;
)

struct edges_t {
  vid_t *edges;
  vid_t cntEdges;
};
typedef struct edges_t edges_t;
#if PAGERANK
  struct data_t {
    double pagerank;
  };
#else  //  MASS_SPRING_DASHPOT
  struct data_t {
    float position[3];
    float velocity[3];
  };
#endif
typedef struct data_t data_t;

//  Every scheduling algorithm is required to define 
//  a sched_t datatype which includes whatever per-vertex data
//  they need to perform their scheduling (e.g., priority, satisfied, dependencies etc.)
#if PAGERANK
  struct vertex_t {
    vid_t id;
    vid_t priority;
    vid_t dependencies;
    volatile vid_t satisfied;
    vid_t cntEdges;
    vid_t * edges;
    double data;
  };
  typedef struct vertex_t vertex_t;

  void update(vertex_t * nodes, const vid_t index, const int round = 0) {
    // ensuring that the number of updates in total is correct per round
    WHEN_TEST({
      __sync_add_and_fetch(&roundUpdateCount, 1);
    })

    vertex_t * current = &nodes[index];
    // recalculate this node's data
    double data = current->data;
    for (size_t i = 0; i < current->cntEdges; ++i) {
      data += nodes[current->edges[i]].data;
    }
    data /= (current->cntEdges + 1);
    current->data = data;
  }

#else  // MASS_SPRING_DASHPOT
  struct vertex_t {
    vid_t id;
    vid_t priority;
    vid_t dependencies;
    volatile vid_t satisfied;
    vid_t cntEdges;
    vid_t * edges;
    double data;
  };
  typedef struct vertex_t vertex_t;

  void update(vertex_t * nodes, const vid_t index, const int round = 0) {
    // ensuring that the number of updates in total is correct per round
    WHEN_TEST({
      __sync_add_and_fetch(&roundUpdateCount, 1);
    })

    vertex_t * current = &nodes[index];
    // recalculate this node's data
    double data = current->data;
    for (size_t i = 0; i < current->cntEdges; ++i) {
      data += nodes[current->edges[i]].data;
    }
    data /= (current->cntEdges + 1);
    current->data = data;
  }
#endif


#endif  // UPDATE_FUNCTION_H_
