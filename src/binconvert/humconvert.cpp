#include <ctime>
#include <string>
#include <iostream>
#include "./common.h"
#include "../libgraphio/libgraphio.h"

static inline double runtime(clock_t first, clock_t second) {
  return (static_cast<double>(second - first)) / CLOCKS_PER_SEC;
}

int main(int argc, char *argv[]) {
  char * inputEdgeFile;
  char * outputEdgeFile;

  const int numArgs = 2;

  std::cout << '\n';

  if (argc != (numArgs + 1)) {
    std::cerr << "ERROR: Expected " << numArgs <<
                 " arguments, received " << argc-1 << '\n';
    std::cerr << "\nThis program converts binary binadjlist files" <<
                 " to human-readable adjlist files.\n";
    std::cerr << "Usage: ./humconvert <binadjlist_input> <adjlist_output>" << std::endl;
    return 1;
  }

  inputEdgeFile = argv[1];
  outputEdgeFile = argv[2];

  std::cout << "Input binadjlist file: " << inputEdgeFile << '\n';
  std::cout << "Output adjlist file:   " << outputEdgeFile << std::endl;

  clock_t start = clock();
  clock_t end;

  EdgeListBuilder * output = adjlistfile_write(outputEdgeFile);
  int result = binadjlistfile_read(inputEdgeFile, output);
  assert(result == 0);

  end = clock();
  std::cout << "Done in " << runtime(start, end) << "s." << std::endl;

  return 0;
}
