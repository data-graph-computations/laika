#ifndef UNIFORM_DISTRIBUTION_H_
#define UNIFORM_DISTRIBUTION_H_

#if DIST_UNIFORM

#include <cmath>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <random>
#include <tuple>
#include <algorithm>
#include <vector>
#include "./common.h"

#ifndef MIN_COORD
  #define MIN_COORD 0.0
#endif

#ifndef MAX_COORD
  #define MAX_COORD 1023.0
#endif

#ifndef MAX_EDGE_LENGTH
  #define MAX_EDGE_LENGTH 16.0
#endif

static void generatePoints(std::mt19937_64 generator,
                           std::uniform_real_distribution<> distrib,
                           vertex_t * const nodes, const vid_t cntNodes) {
  for (vid_t i = 0; i < cntNodes; ++i) {
    double x, y, z;
    x = distrib(generator);
    y = distrib(generator);
    z = distrib(generator);
    nodes[i].id = i;
    nodes[i].x = x;
    nodes[i].y = y;
    nodes[i].z = z;
  }
}

static inline double squaredDistance(const vertex_t * const a,
                                     const vertex_t * const b) {
  double dx, dy, dz;
  dx = (a->x - b->x);
  dy = (a->y - b->y);
  dz = (a->z - b->z);
  return (dx * dx) + (dy * dy) + (dz * dz);
}

static void generateEdges(vertex_t * const nodes, vector<vid_t> * const edges,
                          const vid_t cntNodes) {
  const double maxSquaredDistance = MAX_EDGE_LENGTH * MAX_EDGE_LENGTH;
  std::tuple<double, vid_t> * coordSumOrder = new tuple<double, vid_t>[cntNodes];
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    coordSumOrder[i] = std::make_tuple(nodes[i].x + nodes[i].y + nodes[i].z, i);
  }

  std::stable_sort(coordSumOrder, coordSumOrder + cntNodes);

  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    vertex_t * currentNode = &nodes[i];
    vector<vid_t> * currentEdges = &edges[i];

    double coordSum = nodes[i].x + nodes[i].y + nodes[i].z;

    // adding 1e-6 fudge factor to slightly increase search space
    // and ward against precision problems
    double lowerSum = coordSum - (3 * MAX_EDGE_LENGTH) - 1e-6;
    double upperSum = coordSum + (3 * MAX_EDGE_LENGTH) + 1e-6;

    std::tuple<double, vid_t> * lowerPtr;
    std::tuple<double, vid_t> * upperPtr;

    auto lowerBound = std::make_tuple(lowerSum, 0);
    auto upperBound = std::make_tuple(upperSum, cntNodes);

    lowerPtr = std::lower_bound(coordSumOrder, coordSumOrder + cntNodes, lowerBound);
    upperPtr = std::upper_bound(coordSumOrder, coordSumOrder + cntNodes, upperBound);

    for (std::tuple<double, vid_t> * it = lowerPtr; it != upperPtr; ++it) {
      vid_t destIndex = get<1>(*it);
      const vertex_t * const dest = &nodes[destIndex];

      if (squaredDistance(currentNode, dest) <= maxSquaredDistance) {
        currentEdges->push_back(destIndex);
      }
    }
  }
}

WHEN_TEST(
static void ensureEdgesAreBidirectional(const vector<vid_t> * const edges,
                                        vid_t cntNodes) {
  for (vid_t current = 0; current < cntNodes; ++current) {
    for (auto& neighbor : edges[current]) {
      bool found = false;
      for (auto& backEdge : edges[neighbor]) {
        if (current == backEdge) {
          found = true;
          break;
        }
      }
      assert(found);
    }
  }
})

static inline double runtime(clock_t first, clock_t second) {
  return (static_cast<double>(second - first)) / CLOCKS_PER_SEC;
}

static void generateGraph(vertex_t * const nodes,
                          vector<vid_t> * const edges,
                          const vid_t cntNodes) {
  std::random_device rd;
  std::mt19937_64 generator(rd());
  std::uniform_real_distribution<> distrib(MIN_COORD, MAX_COORD);

  cout << "Distribution: Uniform, " << distrib << endl;
  clock_t before, after;

  before = clock();
  generatePoints(generator, distrib, nodes, cntNodes);
  after = clock();

  cout << "Point generation complete in: " << runtime(before, after) << "s\n";

  before = after;
  generateEdges(nodes, edges, cntNodes);
  after = clock();

  cout << "Edge generation complete in: " << runtime(before, after) << "s\n";

  WHEN_TEST({
    before = after;
    ensureEdgesAreBidirectional(edges, cntNodes);
    after = clock();
    cout << "Edges verified to exist in both directions: " <<
            runtime(before, after) << "s\n";
  })
}

#endif  // DIST_UNIFORM

#endif  // UNIFORM_DISTRIBUTION_H_
