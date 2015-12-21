#ifndef MSD_STRUCT_DEFS_H_
#define MSD_STRUCT_DEFS_H_

#include <cmath>
#include <cstring>
#include <string>
#include <algorithm>
#include "./common.h"

#ifndef DIMENSIONS
  #define DIMENSIONS 3
#endif

typedef float phys_t;

//  will be initialized in fillInNodeData
struct data_t {
  phys_t position[DIMENSIONS];
  phys_t velocity[DIMENSIONS];
  phys_t restLength;
  bool fixed;
};

struct global_t {
  phys_t dashpotResistance;
  phys_t inverseMass;
  phys_t springStiffness;
  phys_t timeStep;
  phys_t restLength;

  #if TEST_CONVERGENCE
    phys_t * sumSquareNetForce;
  #endif

  #if PRINT_EDGE_LENGTH_HISTOGRAM
    phys_t * edgeLengthsBefore;
    phys_t * edgeLengthsAfter;
    phys_t inverseBucketSize;
  #endif
};

#endif  // MSD_STRUCT_DEFS_H_
