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

using namespace std;

#include "./common.h"
#include "./concurrent_queue.h"

void test_queue() {
  static const vid_t numBits = 20;
  numaInit_t numaInit(NUMA_WORKERS, CHUNK_BITS, static_cast<bool>(NUMA_INIT));
  volatile vid_t * tmpData = static_cast<vid_t *>(numaCalloc(numaInit,
      sizeof(vid_t), (1 << numBits)));
  volatile vid_t * tmpResult = static_cast<vid_t *>(numaCalloc(numaInit,
      sizeof(vid_t), (1 << numBits)));

  mrmw_queue_t Q(tmpData, numBits);

  cilk_for (vid_t i = 0; i < (1 << numBits); i++) {
    Q.push(i);
    tmpResult[i] = Q.pop();
  }

  vid_t sum = 0;
  vid_t sum2 = 0;
  for (vid_t i = 0; i < (1 << numBits); i++) {
    sum += tmpResult[i];
    sum2 += i;
  }

  cout << "numBits = " << numBits << endl;
  cout << "Sum1 = " << sum << endl;
  cout << "Sum2 = " << sum2 << endl;
}

int main(int argc, char *argv[]) {
  vertex_t * nodes;
  vid_t cntNodes;
  char * inputEdgeFile;
  int numRounds = 0;

  WHEN_TEST({
    test_queue();
  })

#if VERTEX_META_DATA
  if (argc != 4) {
    cerr << "\nERROR: Expected 3 arguments, received " << argc-1 << '\n';
    cerr << "Usage: ./compute <num_rounds> <input_edges> <vertex_meta_data>" << endl;
    return 1;
  }
#else
  if (argc != 3) {
    cerr << "\nERROR: Expected 2 arguments, received " << argc-1 << '\n';
    cerr << "Usage: ./compute <num_rounds> <input_edges>" << endl;
    return 1;
  }
#endif

  try {
    numRounds = stoi(argv[1]);
  } catch (exception& e) {
    cerr << "\nERROR: " << e.what() << endl;
    return 1;
  }

  inputEdgeFile = argv[2];

  numaInit_t numaInit(NUMA_WORKERS, CHUNK_BITS, static_cast<bool>(NUMA_INIT));
  int result = readEdgesFromFile(inputEdgeFile, &nodes, &cntNodes, numaInit);
  assert(result == 0);

  cout << "Input edge file: " << inputEdgeFile << '\n';
  cout << "Graph size: " << cntNodes << '\n';

#if PARALLEL
  cout << "Cilk workers: " << __cilkrts_get_nworkers() << '\n';
#else
  cout << "Cilk workers: 1\n";
#endif

  scheddata_t scheddata;
  global_t globaldata;

#if D1_NUMA
  scheddata.numaInit = numaInit;
#endif

  init_scheduling(nodes, cntNodes, &scheddata);

#if VERTEX_META_DATA
  char * vertexMetaDataFile = argv[3];
  fillInNodeData(nodes, cntNodes, vertexMetaDataFile);
#else
  fillInNodeData(nodes, cntNodes);
#endif

  struct timespec starttime, endtime;
  result = clock_gettime(CLOCK_MONOTONIC, &starttime);
  assert(result == 0);

  // suppress fake GCC warning, seems to be a bug in GCC 4.8/4.9/5.1
  WHEN_TEST({
    roundUpdateCount = 0;
  })
  execute_rounds(numRounds, nodes, cntNodes, &scheddata, &globaldata);
  WHEN_TEST({
    assert(roundUpdateCount == (uint64_t)cntNodes);
  })

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
  // double data = 0.0;
  // for (vid_t i = 0; i < cntNodes; ++i) {
  //   data += nodes[i].data;
  // }
  // cout << "Final result (ignore this line): " << data << '\n';

  return 0;
}
