#include "./io.h"
#include <cstdio>
#include <cassert>
#include <string>
#include <iostream>
#include "./common.h"

using namespace std;

int readNodesFromFile(string filepath, vertex_t ** out_nodes, int * out_count) {
  FILE * input = fopen(filepath.c_str(), "r");
  if (input == NULL) {
    cerr << "Couldn't open file " << filepath << endl;
    return -1;
  }

  int n, ndims, zero1, zero2, result;
  result = fscanf(input, "%d %d %d %d\n", &n, &ndims, &zero1, &zero2);
  if (result != 4) {
    cerr << "Illegal node file format on line 1" << endl;
    return -1;
  }
  assert(n >= 1);
  assert(ndims == 3);
  assert(zero1 == 0);
  assert(zero2 == 0);

  vertex_t * nodes = new (std::nothrow) vertex_t[n];
  assert(nodes != 0);

  for (int i = 0; i < n; ++i) {
    result = fscanf(input, "%lu %lf %lf %lf\n",
                    &nodes[i].id, &nodes[i].x, &nodes[i].y, &nodes[i].z);
    if (result != 4) {
      cerr << "Illegal node file format on line " << (i + 1) << endl;
      return -1;
    }
  }

  *out_count = n;
  *out_nodes = nodes;
  return 0;
}
