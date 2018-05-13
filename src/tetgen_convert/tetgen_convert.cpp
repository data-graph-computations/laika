#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <unordered_set>

using namespace std;

void rewriteEdges(int numVertices, FILE * eleInputFile, FILE * eleOutputFile) {
  vector< unordered_set<int> > edges;
  edges.resize(numVertices);

  int numTet, g0, g1;
  fscanf(eleInputFile, "%d %d %d\n", &numTet, &g0, &g1);
  int tetId;
  int v[4];
  for (int i = 0; i < numTet; i++) {
    if (i % 1000000 == 0) {
      printf("scanned past %d tetrahedra\n", i);
    }

    fscanf(eleInputFile, "%d %d %d %d %d\n", &tetId, &v[0], &v[1], &v[2], &v[3]);

    for (int j = 0; j < 4; j++) {
      for (int k = j + 1; k < 4; k++) {
        int edgeEndpointA = v[j];
        int edgeEndpointB = v[k];

        edges[edgeEndpointA].insert(edgeEndpointB);
        edges[edgeEndpointB].insert(edgeEndpointA);
      }
    }
  }

  int totalEdges = 0;
  for (vector< unordered_set<int> >::iterator it = edges.begin(); it != edges.end(); ++it) {
    totalEdges += it->size();
  }

  printf("%d nodes, %d edges\n", numVertices, totalEdges);
  fprintf(eleOutputFile, "AdjacencyGraph\n%d\n%d\n", numVertices, totalEdges);

  printf("writing edge offsets\n");
  int offset = 0;
  for (int i = 0; i < numVertices; ++i) {
    fprintf(eleOutputFile, "%d\n", offset);
    offset += edges[i].size();
  }

  printf("writing edges\n");
  for (int i = 0; i < numVertices; ++i) {
    for (unordered_set<int>::iterator it = edges[i].begin(); it != edges[i].end(); ++it) {
      fprintf(eleOutputFile, "%d\n", *it);
    }
  }
}

int main(int argc, char *argv[]) {
  string inputBaseName = argv[1];
  string outputBaseName = argv[2];
  string nodeInputName = inputBaseName + ".node.simple";
  string eleInputName = inputBaseName + ".ele.simple";
  string eleOutputName = outputBaseName + ".adjlist";

  FILE * eleInputFile = fopen(eleInputName.c_str(), "r");
  FILE * eleOutputFile = fopen(eleOutputName.c_str(), "w");

  int numVertices, g0, g1, g2;
  FILE * nodeInputFile = fopen(nodeInputName.c_str(), "r");
  fscanf(nodeInputFile, "%d %d %d %d\n", &numVertices, &g0, &g1, &g2);
  fclose(nodeInputFile);

  rewriteEdges(numVertices, eleInputFile, eleOutputFile);
  fclose(eleInputFile);
  fclose(eleOutputFile);

  return 0;
}
