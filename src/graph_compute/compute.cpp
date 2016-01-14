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

uint64_t hashOfGraphData(vertex_t * const nodes,
                         const vid_t cntNodes) {
  uint64_t result = 0;
  for (vid_t i = 0; i < cntNodes; i++) {
  #if IN_PLACE
    result ^= hashOfVertexData(&nodes[i].data);
  #else
    result ^= hashOfVertexData(&nodes[i].data[0]);
  #endif
  }
  return result;
}

vid_t getEdgeCount(vertex_t * const nodes,
                   const vid_t cntNodes) {
  vid_t result = 0;
  for (vid_t i = 0; i < cntNodes; i++) {
    result += nodes[i].cntEdges;
  }
  return result;
}

WHEN_TEST(
  volatile uint64_t roundUpdateCount = 0;
)

void test_queue() {
  cout << "Testing Concurrent Queue" << endl;
  static const vid_t numBits = 20;
  numaInit_t numaInit(NUMA_WORKERS, CHUNK_BITS, static_cast<bool>(NUMA_INIT));
  volatile vid_t * tmpData = static_cast<vid_t *>(numaCalloc(numaInit,
      sizeof(vid_t), (1 << numBits)));
  volatile vid_t * tmpResult = static_cast<vid_t *>(numaCalloc(numaInit,
      sizeof(vid_t), (1 << numBits)));

  mrmw_queue_t Q(tmpData, numBits);

  cilk_for (vid_t i = 0; i < (1 << numBits); i++) {
    Q.push(i);
    tmpResult[i] = static_cast<vid_t>(-1);
    while (tmpResult[i] == static_cast<vid_t>(-1)) {
      tmpResult[i] = Q.pop();
    }
  }

  vid_t sum = 0;
  vid_t sum2 = 0;
  for (vid_t i = 0; i < (1 << numBits); i++) {
    sum += tmpResult[i];
    assert(tmpResult[i] != static_cast<vid_t>(-1));
    sum2 += i;
  }

  cout << "numBits = " << numBits << endl;
  cout << "Sum1 = " << sum << endl;
  cout << "Sum2 = " << sum2 << endl;
}

int main(int argc, char *argv[]) {
  vertex_t * nodes;
  vid_t cntNodes;
  vid_t cntEdges;
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
  cntEdges = getEdgeCount(nodes, cntNodes);
  //  This function asserts that there are
  //  no self-edges and that every edge is
  //  reciprocated (i.e., if (v,w) exists, then
  //  so does (w,v))
#if TEST_SIMPLE_AND_UNDIRECTED
  testSimpleAndUndirected(nodes, cntNodes);
#endif

#if VERBOSE
  cout << "Input edge file: " << inputEdgeFile << '\n';
  cout << "Graph size: " << cntNodes << '\n';

  #if NUMA_WORKERS
    cout << "pthread workers: " << NUMA_WORKERS << '\n';
  #elif PARALLEL
    cout << "Cilk workers: " << __cilkrts_get_nworkers() << '\n';
  #else
    cout << "Cilk workers: 1\n";
  #endif
#endif

  scheddata_t scheddata;
  global_t globaldata;

  init_scheduling(nodes, cntNodes, &scheddata);

  fillInGlobalData(nodes, &globaldata, cntNodes, numRounds);

//  This switch indicates whether the app needs an auxiliary
//  file to initialize node data
#if VERTEX_META_DATA
  char * vertexMetaDataFile = argv[3];
  fillInNodeData(nodes, &globaldata, cntNodes, vertexMetaDataFile);
#else
  fillInNodeData(nodes, &globaldata, cntNodes);
#endif

#if PRINT_EDGE_LENGTH_HISTOGRAM
  initialEdgeLengthHistogram(nodes, cntNodes, &globaldata);
#endif

  const double initialConvergenceData = getConvergenceData(nodes, cntNodes, &globaldata);

  struct timespec starttime, endtime;
  result = clock_gettime(CLOCK_MONOTONIC, &starttime);
  assert(result == 0);

  // suppress fake GCC warning, seems to be a bug in GCC 4.8/4.9/5.1
  execute_rounds(numRounds, nodes, cntNodes, &scheddata, &globaldata);
  WHEN_TEST({
    cout << "roundUpdateCount: " << roundUpdateCount << endl;
    cout << "cntNodes: " << cntNodes*numRounds << endl;
    assert(roundUpdateCount == static_cast<uint64_t>(cntNodes)*numRounds);
  })

  result = clock_gettime(CLOCK_MONOTONIC, &endtime);
  assert(result == 0);
  int64_t ns = endtime.tv_nsec;
  ns -= starttime.tv_nsec;
  double seconds = static_cast<double>(ns) * 1e-9;
  seconds += endtime.tv_sec - starttime.tv_sec;

  double timePerMillionEdges = seconds * static_cast<double>(1000000);
  timePerMillionEdges /= static_cast<double>(cntEdges) * static_cast<double>(numRounds);

#if PRINT_EDGE_LENGTH_HISTOGRAM
  //  pagerank does not consume the nodes file containing positions
  assert(MASS_SPRING_DASHPOT == 1);
  finalEdgeLengthHistogram(nodes, cntNodes, &globaldata);
  printEdgeLengthHistograms(nodes, cntNodes, &globaldata);
#endif

#if EXECUTION_ORDER_SORT
  for (int i = 0; i < cntNodes; ++i) {
    cout << i << ' ' << nodes[i].data.execution_number << '\n';
  }
  cout << endl;
#endif

#if TEST_CONVERGENCE
  printConvergenceData(nodes, cntNodes, &globaldata, numRounds);
#elif VERBOSE
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  cout << "Done computing " << numRounds << " rounds!\n";
  cout << "Time taken:     " << setprecision(8) << seconds << "s\n";
  cout << "Time per round: " << setprecision(8) << seconds / numRounds << "s\n";
  cout << "Time per million edges: " << setprecision(8) << timePerMillionEdges << "s\n";
  cout << "Scheduler name: " << SCHEDULER_NAME << '\n';
  cout << "Parallel: " << PARALLEL << '\n';
  cout << "Distance: " << DISTANCE << '\n';
  cout << "Convergence: ";

  #if MASS_SPRING_DASHPOT || PAGERANK || SPMV
    const double convergence =
      getConvergenceData(nodes, cntNodes, &globaldata)/initialConvergenceData;
    cout << convergence << '\n';
  #else
    cout << 0 << '\n';
  #endif

  print_execution_data();

  cout << "Debug flag: " << DEBUG << '\n';
  cout << "Test flag: " << TEST << '\n';
#else
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  cout << APP_NAME << ", ";
  cout << SCHEDULER_NAME << ", ";
  cout << IN_PLACE << ", ";

  #if MASS_SPRING_DASHPOT || PAGERANK || SPMV
    const double convergence =
        getConvergenceData(nodes, cntNodes, &globaldata)/initialConvergenceData;
    cout << convergence << ", ";
  #else
    cout << 0 << ", ";
  #endif

  cout << PARALLEL << ", ";

  #if D1_NUMA
    cout << NUMA_WORKERS << ", ";
  #elif PARALLEL
    cout << (__cilkrts_get_nworkers()) << ", ";
  #else
    cout << "1, ";
  #endif

  cout << setprecision(8) << seconds << ", ";
  cout << setprecision(8) << timePerMillionEdges << ", ";
  cout << sizeof(vertex_t) << ", ";
  cout << sizeof(sched_t) << ", ";
  cout << sizeof(data_t) << ", ";
  cout << hashOfGraphData(nodes, cntNodes) << ", ";
  cout << numRounds << ", ";
  cout << inputEdgeFile << ", ";
  cout << cntNodes << ", ";
  cout << cntEdges << ", ";
  cout << CHUNK_BITS << ", ";
  cout << NUMA_INIT << ", ";
  cout << NUMA_STEAL << ", ";
  cout << DISTANCE << ", ";
  cout << __DATE__ << ", ";
  cout << __TIME__ << endl;
#endif

  cleanup_scheduling(nodes, cntNodes, &scheddata);

  return 0;
}
