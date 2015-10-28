#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <set>

using namespace std;

void rewriteEdges(int numVertices, FILE * eleInputFile, FILE * eleOutputFile) {
  set<pair<int, int> > edges;
  int numTet, g0, g1;
  fscanf(eleInputFile, "%d %d %d\n", &numTet, &g0, &g1);
  int tetId;
  int v[4];
  for (int i = 0; i < numTet; i++) {
    fscanf(eleInputFile, "%d %d %d %d %d\n", &tetId, &v[0], &v[1], &v[2], &v[3]); 
    for (int j = 0; j < 4; j++) {
      for (int k = j + 1; k < 4; k++) {
        edges.insert(pair<int, int>(v[j], v[k]));
        edges.insert(pair<int, int>(v[k], v[j]));
      }
    }
  }
  printf("%d nodes, %d edges\n", numVertices, (int) edges.size());
  fprintf(eleOutputFile, "AdjacencyGraph\n%d\n%d\n", numVertices, (int) edges.size());
  int lastSource = -1; 
  int count = 0;
  for (set<pair<int, int> >::iterator it = edges.begin(); it != edges.end(); it++) {
    if (lastSource != it->first) {
      if (lastSource + 1 != it->first) {
        for (int i = lastSource + 1; i < it->first; i++) {
          fprintf(eleOutputFile, "%d\n", count);
        }
      }
      fprintf(eleOutputFile, "%d\n", count);
    }
    lastSource = it->first;
    count++;
  }
  
  for (int i = lastSource + 1; i < numVertices; i++) {
    fprintf(eleOutputFile, "%d\n", count);
  }

  for (set<pair<int, int> >::iterator it = edges.begin(); it != edges.end(); it++) {
    fprintf(eleOutputFile, "%d\n", it->second);
  }
}

int main(int argc, char *argv[]) {
  string baseName = argv[1];
  string nodeInputName = baseName + ".node.simple";
  string eleInputName = baseName + ".ele.simple";
  string eleOutputName = baseName + ".adjlist";

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
