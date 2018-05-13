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

uint64_t hashOfGraphData(const vertex_t * const nodes,
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

static inline void printVerboseOutput(const vertex_t * const nodes,
                                      const vid_t cntNodes, const int numRounds,
                                      const global_t * const globaldata,
                                      const double seconds,
                                      const double timePerMillionEdges,
                                      const double initialConvergenceData) {
  cout << "Done computing " << numRounds << " rounds!\n";
  cout << "Time taken:     " << setprecision(8) << seconds << "s\n";
  cout << "Time per round: " << setprecision(8) << seconds / numRounds << "s\n";
  cout << "Time per million edges: " << setprecision(8) << timePerMillionEdges << "s\n";
  cout << "Scheduler name: " << SCHEDULER_NAME << '\n';
  cout << "Parallel: " << PARALLEL << '\n';
  cout << "Distance: " << DISTANCE << '\n';
  cout << "Convergence: ";

  #if MASS_SPRING_DASHPOT || PAGERANK
    const double convergence =
      getConvergenceData(nodes, cntNodes, globaldata, numRounds)/initialConvergenceData;
    cout << convergence << '\n';
  #else
    cout << 0 << '\n';
  #endif

  print_execution_data();

  cout << "Debug flag: " << DEBUG << '\n';
  cout << "Test flag: " << TEST << '\n';
}

static inline void printCompactOutput(const string& inputEdgeFile,
                                      const vertex_t * const nodes,
                                      const vid_t cntNodes, const vid_t cntEdges,
                                      const int numRounds,
                                      const global_t * const globaldata,
                                      const double seconds,
                                      const double timePerMillionEdges,
                                      const double initialConvergenceData) {
  cout << APP_NAME << ", ";
  cout << SCHEDULER_NAME << ", ";
  cout << IN_PLACE << ", ";

#if MASS_SPRING_DASHPOT || PAGERANK
  const double convergence =
      getConvergenceData(nodes, cntNodes, globaldata, numRounds)/initialConvergenceData;
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
}

static inline void printConvergenceExperimentHeader(const string& inputEdgeFile,
                                                    const vertex_t * const nodes,
                                                    const vid_t cntNodes,
                                                    const vid_t cntEdges,
                                                    const double initialConvergence,
                                                    const double convergenceCoefficient,
                                                    const int roundsBetweenConvChecks) {
  cout << APP_NAME << ", ";
  cout << SCHEDULER_NAME << ", ";
  cout << IN_PLACE << ", ";

  cout << initialConvergence << ", ";
  cout << convergenceCoefficient << ", ";
  cout << roundsBetweenConvChecks << ", ";

  cout << PARALLEL << ", ";

#if D1_NUMA
  cout << NUMA_WORKERS << ", ";
#elif PARALLEL
  cout << (__cilkrts_get_nworkers()) << ", ";
#else
  cout << "1, ";
#endif

  cout << sizeof(vertex_t) << ", ";
  cout << sizeof(sched_t) << ", ";
  cout << sizeof(data_t) << ", ";
  cout << hashOfGraphData(nodes, cntNodes) << ", ";

  cout << inputEdgeFile << ", ";
  cout << cntNodes << ", ";
  cout << cntEdges << ", ";
  cout << CHUNK_BITS << ", ";
  cout << NUMA_INIT << ", ";
  cout << NUMA_STEAL << ", ";
  cout << DISTANCE << ", ";
  cout << __DATE__ << ", ";
  cout << __TIME__ << endl;
}

static inline void printConvergenceExperimentData(const int roundsExecuted,
                                                  const double seconds,
                                                  const double currentConvergence) {
  cout << roundsExecuted << ", ";
  cout << setprecision(8) << seconds << ", ";
  cout << setprecision(8) << currentConvergence << endl;
}

static void prepareTestRun(const char * const inputEdgeFile,
                           const char * const vertexMetaDataFile,
                           const int numRounds,
                           vertex_t ** const outNodes,
                           vid_t * const outCntNodes,
                           vid_t * const outCntEdges,
                           scheddata_t * const outSchedData,
                           global_t * const outGlobalData) {
  vertex_t * nodes;
  vid_t cntNodes;
  vid_t cntEdges;

  numaInit_t numaInit(NUMA_WORKERS, CHUNK_BITS, static_cast<bool>(NUMA_INIT));

  int result = readEdgesFromFile(inputEdgeFile, &nodes, &cntNodes, numaInit);
  assert(result == 0);
  cntEdges = getEdgeCount(nodes, cntNodes);

#if TEST_SIMPLE_AND_UNDIRECTED
  //  This function asserts that there are
  //  no self-edges and that every edge is
  //  reciprocated (i.e., if (v,w) exists, then
  //  so does (w,v))
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

  init_scheduling(nodes, cntNodes, outSchedData);

//  This switch indicates whether the app needs an auxiliary
//  file to initialize node data
#if VERTEX_META_DATA
  fillInNodeData(nodes, cntNodes, vertexMetaDataFile);
#else
  fillInNodeData(nodes, cntNodes);
#endif

  fillInGlobalData(nodes, cntNodes, outGlobalData, numRounds);

#if PRINT_EDGE_LENGTH_HISTOGRAM
  initialEdgeLengthHistogram(nodes, cntNodes, outGlobalData);
#endif

  *outNodes = nodes;
  *outCntNodes = cntNodes;
  *outCntEdges = cntEdges;
}

int main_fixed_number_of_rounds(int argc, char *argv[]) {
  // main function for executing a fixed number of iterations on a dataset
  vertex_t * nodes;
  vid_t cntNodes;
  vid_t cntEdges;
  char * inputEdgeFile;
  char * vertexMetaDataFile = NULL;
  int result = 0;
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
  vertexMetaDataFile = argv[3];
#else
  if (argc != 3) {
    cerr << "\nERROR: Expected 2 arguments, received " << argc-1 << '\n';
    cerr << "Usage: ./compute <num_rounds> <input_edges>" << endl;
    return 1;
  }
#endif
  inputEdgeFile = argv[2];

  try {
    numRounds = stoi(argv[1]);
  } catch (exception& e) {
    cerr << "\nERROR: " << e.what() << endl;
    return 1;
  }

  scheddata_t scheddata;
  global_t globaldata;

  prepareTestRun(inputEdgeFile, vertexMetaDataFile, numRounds, &nodes, &cntNodes,
                 &cntEdges, &scheddata, &globaldata);

  /////////////////////////////////////////////////////////////////////
  ///                     RUNNING THE EXPERIMENT                    ///
  /////////////////////////////////////////////////////////////////////
  const double initialConvergenceData = getInitialConvergenceData(nodes, cntNodes,
                                                                  &globaldata);

  #if RUN_EXTRA_WARMUP
    // Run a few iterations of extra warmup before starting to count time.
    execute_rounds(2, nodes, cntNodes, &scheddata, &globaldata);
  #endif

  struct timespec starttime, endtime;
  result = clock_gettime(CLOCK_MONOTONIC, &starttime);
  assert(result == 0);

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
  /////////////////////////////////////////////////////////////////////
  ///                     END OF THE EXPERIMENT                     ///
  /////////////////////////////////////////////////////////////////////

#if PRINT_EDGE_LENGTH_HISTOGRAM
  //  pagerank does not consume the nodes file containing positions
  assert(MASS_SPRING_DASHPOT == 1);
  finalEdgeLengthHistogram(nodes, cntNodes, &globaldata);
  printEdgeLengthHistograms(nodes, cntNodes, &globaldata);
#endif

// for the execution_order_sort experiment,
// we want to print the execution order of each vertex in the graph
#if EXECUTION_ORDER_SORT
  for (int i = 0; i < cntNodes; ++i) {
    cout << i << ' ' << nodes[i].data.execution_number << '\n';
  }
  cout << endl;
#endif

// print test run output
#if TEST_CONVERGENCE
  printConvergenceData(nodes, cntNodes, &globaldata, numRounds);
#elif VERBOSE
  printVerboseOutput(nodes, cntNodes, numRounds, &globaldata,
                     seconds, timePerMillionEdges, initialConvergenceData);
#else
  printCompactOutput(inputEdgeFile, nodes, cntNodes, cntEdges, numRounds, &globaldata,
                     seconds, timePerMillionEdges, initialConvergenceData);
#endif

  cleanup_scheduling(nodes, cntNodes, &scheddata);

  return 0;
}

int main_run_to_convergence(int argc, char *argv[]) {
  // main function for executing a fixed number of iterations on a dataset
  vertex_t * nodes;
  vid_t cntNodes;
  vid_t cntEdges;
  char * inputEdgeFile;
  char * vertexMetaDataFile = NULL;
  double convergenceCoefficient;
  int result = 0;

  const int numRounds = 5000000;  // cutoff round number, should never be hit
  const double cutoffTime = 50400.0;  // stop after this many seconds

  // on 48 cores, this means a check about every ~10s on the maximum input size
  const int roundsBetweenConvergenceChecks = 250;

#if VERTEX_META_DATA
  if (argc != 4) {
    cerr << "\nERROR: Expected 3 arguments, received " << argc-1 << '\n';
    cerr << ("Usage: ./compute <convergence_coefficient> "
             "<input_edges> <vertex_meta_data>") << endl;
    return 1;
  }
  vertexMetaDataFile = argv[3];
#else
  if (argc != 3) {
    cerr << "\nERROR: Expected 2 arguments, received " << argc-1 << '\n';
    cerr << "Usage: ./compute <convergence_coefficient> <input_edges>" << endl;
    return 1;
  }
#endif
  inputEdgeFile = argv[2];

  convergenceCoefficient = strtod(argv[1], NULL);
  if (convergenceCoefficient >= 1.0 || convergenceCoefficient <= 0.0) {
    cerr << "\nERROR: Invalid convergence_coefficient, "
            "please use a value in (0.0, 1.0)." << endl;
    return 1;
  }

  scheddata_t scheddata;
  global_t globaldata;

  prepareTestRun(inputEdgeFile, vertexMetaDataFile, numRounds, &nodes, &cntNodes,
                 &cntEdges, &scheddata, &globaldata);

  /////////////////////////////////////////////////////////////////////
  ///                     RUNNING THE EXPERIMENT                    ///
  /////////////////////////////////////////////////////////////////////

  // calculate the initial convergence data, and the final convergence number
  // that signals the end of the experiment
  double currentConvergence = getInitialConvergenceData(nodes, cntNodes, &globaldata);
  const double cutoffConvergence = currentConvergence * convergenceCoefficient;
  int roundsExecuted = 0;
  double totalSeconds = 0.0;

  printConvergenceExperimentHeader(inputEdgeFile, nodes, cntNodes, cntEdges,
                                   currentConvergence, convergenceCoefficient,
                                   roundsBetweenConvergenceChecks);

  while (currentConvergence > cutoffConvergence) {
    struct timespec starttime, endtime;
    result = clock_gettime(CLOCK_MONOTONIC, &starttime);
    assert(result == 0);

    execute_rounds(roundsBetweenConvergenceChecks,
                   nodes, cntNodes, &scheddata, &globaldata);

    result = clock_gettime(CLOCK_MONOTONIC, &endtime);
    assert(result == 0);
    int64_t ns = endtime.tv_nsec;
    ns -= starttime.tv_nsec;
    double seconds = static_cast<double>(ns) * 1e-9;
    seconds += endtime.tv_sec - starttime.tv_sec;
    totalSeconds += seconds;

    roundsExecuted += roundsBetweenConvergenceChecks;

    currentConvergence = getConvergenceData(nodes, cntNodes, &globaldata,
                                            roundsExecuted);
    printConvergenceExperimentData(roundsExecuted, totalSeconds,
                                   currentConvergence);

    // ensure that we are making progress toward convergence, and not just spinning
    if ((roundsExecuted >= numRounds) || (totalSeconds >= cutoffTime)) {
      // we've run out of either time or iterations, stop the computation
      break;
    }
  }

  /////////////////////////////////////////////////////////////////////
  ///                     END OF THE EXPERIMENT                     ///
  /////////////////////////////////////////////////////////////////////

  cleanup_scheduling(nodes, cntNodes, &scheddata);

  return 0;
}

int main(int argc, char *argv[]) {
  #if RUN_CONVERGENCE_EXPERIMENT
    return main_run_to_convergence(argc, argv);
  #elif RUN_FIXED_ROUNDS_EXPERIMENT
    return main_fixed_number_of_rounds(argc, argv);
  #else
    #error "No experiment specified!"
    // shouldn't happen, RUN_FIXED_ROUNDS_EXPERIMENT is default
  #endif
}
