#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <ctime>
#include <string>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <exception>
#include <vector>
#include "./common.h"
#include "./io.h"
#include "./update_function.h"

using namespace std;

#if BASELINE || D1_PRIO
  #include "./priority_scheduling.h"
#elif D1_CHUNK
  #include "./chunk_scheduling.h"
#elif D0_BSP
  #include "./bsp_scheduling.h"
#else
  #error "No scheduling type defined! Specify one of BASELINE, D0_BSP, D1_PRIO, D1_CHUNK."
#endif

WHEN_TEST(
  static uint64_t roundUpdateCount = 0;
)

// fill in each node with random-looking data
static void fillInNodeData(vertex_t * nodes, const vid_t cntNodes) {
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    nodes[i].data = nodes[i].priority + ((-nodes[i].id) ^ (nodes[i].dependencies * 31));
  }
}

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

int main(int argc, char *argv[]) {
  vertex_t * nodes;
  vid_t cntNodes;
  char * inputEdgeFile;
  int numRounds = 0;

  if (argc != 3) {
    cerr << "\nERROR: Expected 2 arguments, received " << argc-1 << '\n';
    cerr << "Usage: ./compute <num_rounds> <input_edges>" << endl;
    return 1;
  }

  try {
    numRounds = stoi(argv[1]);
  } catch (exception& e) {
    cerr << "\nERROR: " << e.what() << endl;
    return 1;
  }

  inputEdgeFile = argv[2];

  cout << "Input edge file: " << inputEdgeFile << '\n';

  int result = readEdgesFromFile(inputEdgeFile, &nodes, &cntNodes);
  assert(result == 0);

  cout << "Graph size: " << cntNodes << '\n';

#if PARALLEL
  cout << "Cilk workers: " << __cilkrts_get_nworkers() << '\n';
#else
  cout << "Cilk workers: 1\n";
#endif

  scheddata_t scheddata;

  init_scheduling(nodes, cntNodes, &scheddata);

  // our nodes don't have any real data associated with them
  // generate some fake data instead
  fillInNodeData(nodes, cntNodes);

  struct timespec starttime, endtime;
  result = clock_gettime(CLOCK_MONOTONIC, &starttime);
  assert(result == 0);

  // suppress fake GCC warning, seems to be a bug in GCC 4.8/4.9/5.1
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  for (int i = 0; i < numRounds; ++i) {
    WHEN_TEST({
      roundUpdateCount = 0;
    })
    execute_round(i, nodes, cntNodes, &scheddata);
    WHEN_TEST({
      assert(roundUpdateCount == (uint64_t)cntNodes);
    })
  }

  result = clock_gettime(CLOCK_MONOTONIC, &endtime);
  assert(result == 0);
  int64_t ns = endtime.tv_nsec;
  ns -= starttime.tv_nsec;
  double seconds = static_cast<double>(ns) * 1e-9;
  seconds += endtime.tv_sec - starttime.tv_sec;

  cleanup_scheduling(nodes, cntNodes, &scheddata);

  cout << "Done computing " << numRounds << " rounds!\n";
  cout << "Time taken:     " << setprecision(8) << seconds << "s\n";
  cout << "Time per round: " << setprecision(8) << seconds / numRounds << "s\n";
  cout << "Baseline: " << BASELINE << '\n';
  cout << "D0_BSP: " << D0_BSP << '\n';
  cout << "D1_PRIO: " << D1_PRIO << '\n';
  cout << "D1_CHUNK: " << D1_CHUNK << '\n';
  cout << "Parallel: " << PARALLEL << '\n';

  print_execution_data();

  cout << "Debug flag: " << DEBUG << '\n';
  cout << "Test flag: " << TEST << '\n';

  // so GCC doesn't eliminate the rounds loop as unnecessary work
  double data = 0.0;
  for (vid_t i = 0; i < cntNodes; ++i) {
    data += nodes[i].data;
  }
  cout << "Final result (ignore this line): " << data << '\n';

  return 0;
}
