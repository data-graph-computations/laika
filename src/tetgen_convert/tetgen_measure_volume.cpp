#include <math.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <set>
#include <algorithm>

using namespace std;

typedef int64_t vid_t;

#define DIMENSIONS 3
typedef double phys_t;

struct position_t {
  phys_t position[DIMENSIONS];
};
typedef struct position_t position_t;

const inline phys_t length(phys_t (&a)[DIMENSIONS]) {
  phys_t total = 0;
  for (int i = 0; i < DIMENSIONS; i++) {
    total += a[i]*a[i];
  }
  return sqrt(total);
}

const inline phys_t distance(phys_t (&a)[DIMENSIONS], phys_t (&b)[DIMENSIONS]) {
  phys_t delta[DIMENSIONS];
  for (int i = 0; i < DIMENSIONS; i++) {
    delta[i] = a[i] - b[i];
  }
  return length(delta);
}

//  find volume of irregular tetrahedron using Heron's formula
const inline phys_t getVolume(phys_t (&v0)[DIMENSIONS],
                               phys_t (&v1)[DIMENSIONS],
                               phys_t (&v2)[DIMENSIONS],
                               phys_t (&v3)[DIMENSIONS]) {
  //  Calculate edge lengths
  phys_t U = distance(v0, v1);
  phys_t u = distance(v2, v3);
  phys_t V = distance(v0, v2);
  phys_t v = distance(v1, v3);
  phys_t W = distance(v1, v2);
  phys_t w = distance(v0, v3);
  //  Calculate cross-products
  phys_t X = (w - U + v) * (U + v + w);
  phys_t x = (U - v + w) * (v - w + U);
  phys_t Y = (u - V + w) * (V + w + u);
  phys_t y = (V - w + u) * (w - u + V);
  phys_t Z = (v - W + u) * (W + u + v);
  phys_t z = (W - u + v) * (u - v + W);
  //  Calculate derived cross-products
  phys_t a = sqrt(x * Y * Z);
  phys_t b = sqrt(y * Z * X);
  phys_t c = sqrt(z * X * Y);
  phys_t d = sqrt(x * y * z);
  //  Finally, calculate volume
  return sqrt((-a + b + c + d) * (a - b + c + d) * (a + b - c + d) * (a + b + c - d)) /
    (192 * u * v * w);
}

int main(int argc, char *argv[]) {
  string inputBaseName = argv[1];
  string nodeInputName = inputBaseName + ".node.simple";

  vid_t numVertices, g0, g1, g2;
  FILE * nodeInputFile = fopen(nodeInputName.c_str(), "r");
  fscanf(nodeInputFile, "%ld %ld %ld %ld\n", &numVertices, &g0, &g1, &g2);
  position_t * vertices = new (std::nothrow) position_t[numVertices];
  phys_t minPosition[DIMENSIONS];
  phys_t maxPosition[DIMENSIONS];
  for (vid_t i = 0; i < numVertices; i++) {
    vid_t v;
    fscanf(nodeInputFile, "%ld %lf %lf %lf\n", &v, &vertices[i].position[0],
      &vertices[i].position[1], &vertices[i].position[2]);
    if (i == 0) {
      for (int d = 0; d < DIMENSIONS; d++) {
        maxPosition[d] = vertices[i].position[d];
        minPosition[d] = vertices[i].position[d];
      }
    } else {
      for (int d = 0; d < DIMENSIONS; d++) {
        maxPosition[d] = std::max(maxPosition[d], vertices[i].position[d]);
        minPosition[d] = std::min(minPosition[d], vertices[i].position[d]);
      }
    }
  }
  fclose(nodeInputFile);
  phys_t scale = 0;
  phys_t boundingBoxVolume = 1.0;
  for (int d = 0; d < DIMENSIONS; d++) {
    scale = std::max(scale, maxPosition[d] - minPosition[d]);
    cout << maxPosition[d] - minPosition[d] << endl;
    boundingBoxVolume *= (maxPosition[d] - minPosition[d]);
  }
  phys_t inverseScale = 1/scale;
  for (int d = 0; d < DIMENSIONS; d++) {
    boundingBoxVolume *= inverseScale;
  }
  for (vid_t i = 0; i < numVertices; i++) {
    for (int d = 0; d < DIMENSIONS; d++) {
      vertices[i].position[d] -= minPosition[d];
      //  vertices[i].position[d] *= inverseScale;
      vertices[i].position[d] *= inverseScale;
    }
  }

  string eleInputName = inputBaseName + ".ele.simple";

  FILE * eleInputFile = fopen(eleInputName.c_str(), "r");
  vid_t numTet;
  phys_t totalVolume = 0.0;
  fscanf(eleInputFile, "%ld %ld %ld\n", &numTet, &g0, &g1);
  for (vid_t i = 0; i < numTet; i++) {
    vid_t tetID, v0, v1, v2, v3;
    fscanf(eleInputFile, "%ld %ld %ld %ld %ld\n", &tetID, &v0, &v1, &v2, &v3);
    totalVolume += getVolume(vertices[v0].position,
                             vertices[v1].position,
                             vertices[v2].position,
                             vertices[v3].position);
  }
  fclose(eleInputFile);

  cout << totalVolume << ", " << boundingBoxVolume
       << ", " << (totalVolume/boundingBoxVolume) << endl;

  return 0;
}
