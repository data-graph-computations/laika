#include "./edge_generator.h"
#include <vector>
#include <algorithm>
#include <tuple>
#include "./common.h"

static inline double squaredDistance(const vertex_t * const a,
                                     const vertex_t * const b) {
  double dx, dy, dz;
  dx = (a->x - b->x);
  dy = (a->y - b->y);
  dz = (a->z - b->z);
  return (dx * dx) + (dy * dy) + (dz * dz);
}

void generateEdges(vertex_t * const nodes,
                   std::vector<vid_t> * const edges,
                   const vid_t cntNodes,
                   const double maxEdgeLength) {
  const double maxSquaredDistance = maxEdgeLength * maxEdgeLength;
  std::tuple<double, vid_t> * coordSumOrder = new std::tuple<double, vid_t>[cntNodes];
  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    coordSumOrder[i] = std::make_tuple(nodes[i].x + nodes[i].y + nodes[i].z, i);
  }

  std::stable_sort(coordSumOrder, coordSumOrder + cntNodes);

  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    vertex_t * currentNode = &nodes[i];
    std::vector<vid_t> * currentEdges = &edges[i];

    double coordSum = nodes[i].x + nodes[i].y + nodes[i].z;

    // adding 1e-6 fudge factor to slightly increase search space
    // and ward against precision problems
    double lowerSum = coordSum - (3 * maxEdgeLength) - 1e-6;
    double upperSum = coordSum + (3 * maxEdgeLength) + 1e-6;

    std::tuple<double, vid_t> * lowerPtr;
    std::tuple<double, vid_t> * upperPtr;

    auto lowerBound = std::make_tuple(lowerSum, 0);
    auto upperBound = std::make_tuple(upperSum, cntNodes);

    lowerPtr = std::lower_bound(coordSumOrder, coordSumOrder + cntNodes, lowerBound);
    upperPtr = std::upper_bound(coordSumOrder, coordSumOrder + cntNodes, upperBound);

    for (std::tuple<double, vid_t> * it = lowerPtr; it != upperPtr; ++it) {
      vid_t destIndex = std::get<1>(*it);
      const vertex_t * const dest = &nodes[destIndex];

      if (squaredDistance(currentNode, dest) <= maxSquaredDistance) {
        currentEdges->push_back(destIndex);
      }
    }
  }
}
