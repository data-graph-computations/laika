#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <climits>
#include <cfloat>
#include <string>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <utility>
#include "./common.h"
#include "./io.h"

using namespace std;

#define NO_ID (static_cast<vid_t>(-1))

static inline double stringToDouble(const string& s) {
  istringstream i(s);
  double x;

  if (!(i >> x)) {
    return -1.0;
  }

  return x;
}

static inline
void initializeNodes(vertex_t * const nodes, const vid_t cntNodes) {
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    nodes[i].id = i;
    nodes[i].reorderId = NO_ID;
    nodes[i].value = 0.0;
  }
}

static inline
vid_t expandToNextNode(const vid_t nodeToExpand, const vid_t reorderId,
                       const double invCompoundDiscountFactor,
                       vertex_t * const nodes, const vid_t cntNodes,
                       priority_queue< pair<double, vid_t> > * const pq) {
  vertex_t * const currentNode = &nodes[nodeToExpand];

  WHEN_DEBUG({
    cerr << "Expanding node " << nodeToExpand << endl;
  })

  // make sure the current node wasn't visited before,
  // and then mark it as visited
  assert(currentNode->reorderId == NO_ID);
  currentNode->reorderId = reorderId;

  // add 1 * invCompoundDiscountFactor to the neighbors of the current vertex
  for (vid_t i = 0; i < currentNode->edgeData.cntEdges; ++i) {
    const vid_t neighbor = currentNode->edgeData.edges[i];
    nodes[neighbor].value += invCompoundDiscountFactor;
    pq->emplace(nodes[neighbor].value, neighbor);
  }

  pair<double, vid_t> next = make_pair(-1.0, NO_ID);
  pair<double, vid_t> considered = next;

  // find and return the unvisited vertex of maximum value
  do {
    assert(!pq->empty());
    considered = pq->top();
    pq->pop();

    WHEN_DEBUG({
      cerr << "Considering (" << considered.first << ", "
           << considered.second << ") as next node." << endl;
    })

    // if the considered node is not yet visited
    if (nodes[considered.second].reorderId == NO_ID) {
      // values are non-decreasing, so the node should have been dequeued
      // at its maximum (and current) value -- assert this
      assert(considered.first == nodes[considered.second].value);
      next = considered;
      break;
    }
  } while (!pq->empty());

  WHEN_DEBUG({
    // ensure that when the queue is empty, all nodes have been visited
    if (pq->empty()) {
      cerr << "pq->empty() == true\n";
      vid_t unvisited_id = NO_ID;
      for (vid_t i = 0; i < cntNodes; ++i) {
        if (nodes[i].reorderId == NO_ID) {
          unvisited_id = i;
          cerr << "unvisited node found, id: " << unvisited_id << endl;
        }
      }
      assert(unvisited_id == NO_ID);
    }
  })

  return next.second;
}

void print_usage() {
  cerr << "Usage: ./dv_reorder <input_edges> <discount_factor>\n";
  cerr << "discount_factor must be between 0 and 1, inclusive.\n";
  cerr << "dv_reorder prints the reordering indexes to stdout." << endl;
}

int main(int argc, char *argv[]) {
  vertex_t * nodes = NULL;
  vid_t cntNodes = NO_ID;

  char * inputEdgeFile;
  double discountFactor;

  if (argc != 3) {
    cerr << "\nERROR: Expected 2 arguments, received " << argc-1 << '\n';
    print_usage();
    return 1;
  }

  inputEdgeFile = argv[1];
  discountFactor = stringToDouble(argv[2]);

  if (discountFactor < 0.0 || discountFactor > 1.0) {
    cerr << "\nERROR: Illegal discount factor or parsing error.\n";
    print_usage();
    return 1;
  }

  cerr << "Input edge file: " << inputEdgeFile << '\n';
  cerr << "Discount factor: " << discountFactor << '\n';

  int result = readEdgesFromFile(inputEdgeFile, &nodes, &cntNodes);
  assert(result == 0);

  initializeNodes(nodes, cntNodes);

  // add all vertices except 0 (the start vertex) to the priority queue
  priority_queue< pair<double, vid_t> > pq;
  for (vid_t i = 1; i < cntNodes; ++i) {
    pq.emplace(0.0, i);
  }

  // start from vertex 0, run cntNodes - 1 times
  vid_t nextNode = 0;
  for (vid_t i = 1; i < cntNodes; ++i) {
    cerr << "Expanding node " << i << " of " << cntNodes << endl;
    nextNode = expandToNextNode(nextNode, i-1, discountFactor, nodes, cntNodes, &pq);
  }
  nodes[nextNode].reorderId = cntNodes - 1;

  // ensure that we are done
  assert(nextNode == NO_ID);

  // print the output to stdout
  cerr << "Producing output..." << endl;
  for (vid_t i = 0; i < cntNodes; ++i) {
    assert(nodes[i].id == i);
    assert(nodes[i].reorderId != NO_ID);
    printf(VID_T_LITERAL " " VID_T_LITERAL "\n", i, nodes[i].reorderId);
  }

  return 0;
}
