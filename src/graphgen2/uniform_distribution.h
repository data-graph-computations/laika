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
