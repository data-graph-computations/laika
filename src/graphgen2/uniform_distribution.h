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

static void generateGraph(vertex_t * const nodes,
                          vector<vid_t> * const edges,
                          const vid_t cntNodes) {
  std::random_device rd;
  std::mt19937_64 generator(rd());
  std::uniform_real_distribution<> distrib(MIN_COORD, MAX_COORD);

  cout << "Distribution: Uniform, " << distrib << endl;

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

#endif  // DIST_UNIFORM

#endif  // UNIFORM_DISTRIBUTION_H_
