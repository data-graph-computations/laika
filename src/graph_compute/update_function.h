#ifndef UPDATE_FUNCTION_H_
#define UPDATE_FUNCTION_H_

WHEN_TEST(
  static uint64_t roundUpdateCount = 0;
)

void update(vertex_t * nodes, const vid_t index) {
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

#endif  // UPDATE_FUNCTION_H_
