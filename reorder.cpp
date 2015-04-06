#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <climits>
#include <cfloat>
#include "./common.h"

int main() {
  printf("Hello world!\n");
  return 0;
}

// This function populates the hilbertId field of each vertex_t in nodes, by
// remapping every vertex onto an n^3 lattice using appropriate scaling factors,
// and then traversing the lattice using a 3-D Hilbert curve.
//
// hilbertGridN must be a power of 2
void assignHilbertIds(vertex_t * nodes, int cntNodes, uint64_t hilbertGridN) {
  // We first traverse all vertices to find the maximal and minimal coordinates
  // along each axis: xMin, xMax, yMin, yMax, zMin, zMax.

  double xMin, xMax, yMin, yMax, zMin, zMax;
  xMin = yMin = zMin = DBL_MAX;
  xMax = yMax = zMax = DBL_MIN;
  for (int i = 0; i < cntNodes; ++i) {
    xMin = fmin(xMin, nodes[i].x);
    yMin = fmin(yMin, nodes[i].y);
    zMin = fmin(zMin, nodes[i].z);
    xMax = fmin(xMax, nodes[i].x);
    yMax = fmin(yMax, nodes[i].y);
    zMax = fmin(zMax, nodes[i].z);
  }

  // We now create a mapping that maps:
  //   (xMin, yMin, zMin) to lattice point (0, 0, 0)
  //   (xMax, yMax, zMax) to lattice point (n-1, n-1, n-1)
  // where n = hilbertGridN
  //
  // This mapping is the following: for the T axis,
  // tLattice = round((t - tMin) * (hilbertGridN - 1) / tMax);

  for (int i = 0; i < cntNodes; ++i) {
    uint64_t xLattice, yLattice, zLattice;
    xLattice = (uint64_t) round((nodes[i].x - xMin) * (hilbertGridN - 1) / xMax);
    yLattice = (uint64_t) round((nodes[i].y - yMin) * (hilbertGridN - 1) / yMax);
    zLattice = (uint64_t) round((nodes[i].z - zMin) * (hilbertGridN - 1) / zMax);

    // TODO(predrag): map to Hilbert curve
  }
}
