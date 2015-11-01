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
#include "./concurrent_queue.h"

using namespace std;

#if BASELINE || D1_PRIO
  #include "./priority_scheduling.h"
#elif D1_CHUNK
  #include "./chunk_scheduling.h"
#elif D1_PHASE
  #include "./phase_scheduling.h"
#elif D1_NUMA
  #include "./numa_scheduling.h"
#elif D0_BSP
  #include "./bsp_scheduling.h"
#else
  #error "No scheduling type defined!"
  #error "Specify one of BASELINE, D0_BSP, D1_PRIO, D1_CHUNK, D1_PHASE, D1_NUMA."
#endif

WHEN_TEST(
  static uint64_t roundUpdateCount = 0;
)

// fill in each node with random-looking data
static void fillInNodeData(vertex_t * nodes, const vid_t cntNodes) {
  for (vid_t i = 0; i < cntNodes; ++i) {
    nodes[i].data = nodes[i].priority + ((-nodes[i].id) ^ (nodes[i].dependencies * 31));
  }
}

//
// from http://www.codeproject.com/Articles/69941/Best-Square-Root-Method-Algorithm-Function-Precisi
// store dot product of each vertex in vertex struct
// then turn sqrt((x0-x1)^2 + (y0-y1)^2 + (z0-z1)^2) into:
// sqrt((x0*x0+y0*y0+z0*z0) + (x1*x1+y1*y1+z1*z1) - 2(x0*x1+y0*y1+z0*z1))
// double inline __declspec (naked) __fastcall sqrt14(double n)
// {
//   _asm fld qword ptr [esp+4]
//   _asm fsqrt
//   _asm ret 8
// }

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

  static const vid_t numBits = 20;
  vid_t tmpData[1 << 20];
  vid_t result[1 << 20];
  mrmw_queue_t * Q = static_case<mrmw_queue_t>(new mrmw_queue_t(tmpData, numBits));

  cilk_for (vid_t i = 0; i < (1 << numBits); i++) {
    Q->push(i);
    result[i] = Q->pop();
  }

  vid_t sum = 0;
  for (vid_t i = 0; i < (1 << numBits); i++) {
    sum += result[i];
  }

  cout << "numBits = " << numBits << endl;
  cout << "Sum = " << sum << endl;

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

  bool numaInitFlag;
#if NUMA_INIT
  numaInitFlag = true;
#else
  numaInitFlag = false;
#endif
  numaInit_t numaInit(NUMA_WORKERS, CHUNK_BITS, numaInitFlag);
  int result = readEdgesFromFile(inputEdgeFile, &nodes, &cntNodes, numaInit);
  assert(result == 0);

  scheddata_t scheddata;

#if D1_NUMA
  scheddata.numaInit = numaInit;
#endif

  init_scheduling(nodes, cntNodes, &scheddata);

  // our nodes don't have any real data associated with them
  // generate some fake data instead
  fillInNodeData(nodes, cntNodes);

  cout << "Input edge file: " << inputEdgeFile << '\n';
  cout << "Graph size: " << cntNodes << '\n';

#if PARALLEL
  cout << "Cilk workers: " << __cilkrts_get_nworkers() << '\n';
#else
  cout << "Cilk workers: 1\n";
#endif


  struct timespec starttime, endtime;
  result = clock_gettime(CLOCK_MONOTONIC, &starttime);
  assert(result == 0);

  // suppress fake GCC warning, seems to be a bug in GCC 4.8/4.9/5.1
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  for (int i = 0; i < numRounds; ++i) {
    WHEN_TEST({
      roundUpdateCount = 0;
    })
    execute_round(numRounds, nodes, cntNodes, &scheddata);
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
  cout << "D1_PHASE: " << D1_PHASE << '\n';
  cout << "D1_NUMA: " << D1_NUMA << '\n';
  cout << "Parallel: " << PARALLEL << '\n';
  cout << "Distance: " << DISTANCE << '\n';

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
