#include "./edge_generator.h"
#include <cstring>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <tuple>
#include <exception>

#include "./common.h"

static inline double squaredDistance(const vertex_t * const a,
                                     const vertex_t * const b) {
  double dx, dy, dz;
  dx = (a->x - b->x);
  dy = (a->y - b->y);
  dz = (a->z - b->z);
  return (dx * dx) + (dy * dy) + (dz * dz);
}

static inline
std::vector<vid_t> ** getVectorInGrid(std::vector<vid_t> ** const grid,
                                      const vid_t gridSize,
                                      const vid_t x, const vid_t y, const vid_t z) {
  WHEN_DEBUG({
    std::cout << "Getting vector for grid cell: ("
              << x << ", " << y << ", " << z << ")" << std::endl;
  })

  // vid_t are unsigned, can't meaningfully compare against zero
  // but < gridSize also ensures they aren't "negative"
  assert(x < gridSize);
  assert(y < gridSize);
  assert(z < gridSize);

  vid_t index = (x * gridSize + y) * gridSize + z;
  assert(index < (gridSize * gridSize * gridSize));
  return &grid[index];
}

static inline vid_t toGrid(const double x, const vid_t gridSize) {
  assert(x >= MIN_COORD);
  assert(x <= MAX_COORD);
  const double cellSize = ((MAX_COORD - MIN_COORD) / gridSize);
  vid_t result = static_cast<vid_t>((x - MIN_COORD) / cellSize);

  assert(result < gridSize);
  return result;
}

static inline void appendToGridCell(std::vector<vid_t> ** const grid,
                                    const vid_t gridSize,
                                    const vid_t x, const vid_t y, const vid_t z,
                                    const vid_t node) {
  std::vector<vid_t> ** gridVectorPtr = getVectorInGrid(grid, gridSize, x, y, z);
  if (*gridVectorPtr == NULL) {
    *gridVectorPtr = new std::vector<vid_t>;
  }
  (*gridVectorPtr)->push_back(node);
}

static void distributeNodesToGrid(vertex_t * const nodes,
                                  const vid_t cntNodes,
                                  std::vector<vid_t> ** const grid,
                                  const vid_t gridSize) {
  for (vid_t i = 0; i < cntNodes; ++i) {
    vid_t gridX, gridY, gridZ;
    gridX = toGrid(nodes[i].x, gridSize);
    gridY = toGrid(nodes[i].y, gridSize);
    gridZ = toGrid(nodes[i].z, gridSize);
    appendToGridCell(grid, gridSize, gridX, gridY, gridZ, i);
  }
}

static void generateEdgesForNode(vid_t index,
                                 vertex_t * const nodes,
                                 std::vector<vid_t> * const edges,
                                 const vid_t cntNodes,
                                 const double maxEdgeLength,
                                 std::vector<vid_t> ** const grid,
                                 const vid_t gridSize) {
  vid_t gridX, gridY, gridZ;
  gridX = toGrid(nodes[index].x, gridSize);
  gridY = toGrid(nodes[index].y, gridSize);
  gridZ = toGrid(nodes[index].z, gridSize);

  vid_t lowX, lowY, lowZ, highX, highY, highZ;
  // vid_t are unsigned, can't use max because of underflow
  lowX = (gridX == 0) ? 0 : (gridX - 1);
  lowY = (gridY == 0) ? 0 : (gridY - 1);
  lowZ = (gridZ == 0) ? 0 : (gridZ - 1);
  highX = std::min(static_cast<vid_t>(gridSize-1), gridX + 1);
  highY = std::min(static_cast<vid_t>(gridSize-1), gridY + 1);
  highZ = std::min(static_cast<vid_t>(gridSize-1), gridZ + 1);

  assert(lowX <= highX);
  assert(lowY <= highY);
  assert(lowZ <= highZ);

  WHEN_DEBUG({
    std::cout << "Current point in grid: ("
              << gridX << ", " << gridY << ", " << gridZ << ")\n";
    std::cout << "X bounds: " << lowX << " --> " << highX << '\n';
    std::cout << "Y bounds: " << lowY << " --> " << highY << '\n';
    std::cout << "Z bounds: " << lowZ << " --> " << highZ << std::endl;
  })

  auto currentEdges = &edges[index];
  auto currentNode = &nodes[index];
  const double maxSquaredDistance = maxEdgeLength * maxEdgeLength;

  for (vid_t x = lowX; x <= highX; ++x) {
    for (vid_t y = lowY; y <= highY; ++y) {
      for (vid_t z = lowZ; z <= highZ; ++z) {
        auto gridCell = *getVectorInGrid(grid, gridSize, x, y, z);
        if (gridCell != NULL) {
          for (vid_t cellNodeId : *gridCell) {
            // ensure we don't create self-edges
            if (cellNodeId != index) {
              auto cellNode = &nodes[cellNodeId];
              double sqDist = squaredDistance(currentNode, cellNode);
              if (sqDist <= maxSquaredDistance) {
                currentEdges->push_back(cellNodeId);
              }
            }
          }
        }
      }
    }
  }
}

static void freeGrid(std::vector<vid_t> ** const grid, const vid_t gridSize) {
  const vid_t gridTotalCount = gridSize * gridSize * gridSize;
  for (vid_t i = 0; i < gridTotalCount; ++i) {
    if (grid[i] != NULL) {
      delete grid[i];
    }
  }
}

static void calculateGridStats(const vid_t cntNodes,
                               const double maxEdgeLength,
                               std::vector<vid_t> ** const grid,
                               const vid_t gridSize) {
  vid_t emptyCells = 0;
  int maxNodesInCell = 0;
  vid_t totalNodesInCells = 0;
  const double gridTotalCount = gridSize * gridSize * gridSize;

  for (vid_t i = 0; i < gridTotalCount; ++i) {
    if (grid[i] != NULL) {
      int size = grid[i]->size();
      totalNodesInCells += size;
      maxNodesInCell = std::max(maxNodesInCell, size);
    } else {
      ++emptyCells;
    }
  }

  assert(totalNodesInCells == cntNodes);

  double avgNodesPerCell = static_cast<double>(cntNodes);
  avgNodesPerCell /= (gridTotalCount - emptyCells);

  std::cout << "\nGrid stats:\n";
  std::cout << "  max edge length: " << maxEdgeLength << '\n';
  std::cout << "  grid dimension size: " << gridSize << '\n';
  std::cout << "  empty cells: " << emptyCells << '\n';
  std::cout << "  max nodes in cell: " << maxNodesInCell << '\n';
  std::cout << "  avg nodes per nonempty cell: " << avgNodesPerCell << '\n';
  std::cout << std::endl;
}

static double calculateMaxEdgeLength(const vid_t cntNodes, const vid_t nodeAvgDegree) {
  // formula:
  // max edge length = bound. box side * (3 * desired avg degree / 4 * pi * nodes)^(1/3)
  // see paper for explanation
  // the formula tends to undershoot by a small amount, so add a small fudge factor
  const double FUDGE_FACTOR = 0.1;
  assert(nodeAvgDegree > 0);

  const double parens = static_cast<double>(3 * nodeAvgDegree) / (M_PI * cntNodes * 4);
  return FUDGE_FACTOR + ((MAX_COORD - MIN_COORD) * std::pow(parens, 1.0/3.0));
}

int generateEdges(vertex_t * const nodes,
                  std::vector<vid_t> * const edges,
                  const vid_t cntNodes,
                  const vid_t nodeAvgDegree) {
  const double maxEdgeLength = calculateMaxEdgeLength(cntNodes, nodeAvgDegree);
  const vid_t gridSize = std::min(static_cast<vid_t>(1000),
    static_cast<vid_t>(((MAX_COORD-MIN_COORD)/maxEdgeLength)-1));
  const vid_t gridTotalCount = gridSize * gridSize * gridSize;

  // ensure no node can have an edge more than one grid cell away
  assert(maxEdgeLength < ((MAX_COORD - MIN_COORD) / gridSize));

  std::vector<vid_t> ** const grid = new std::vector<vid_t> * [gridTotalCount];
  memset(grid, 0, gridTotalCount * sizeof(std::vector<vid_t> *));

  distributeNodesToGrid(nodes, cntNodes, grid, gridSize);

  calculateGridStats(cntNodes, maxEdgeLength, grid, gridSize);

  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    generateEdgesForNode(i, nodes, edges, cntNodes, maxEdgeLength, grid, gridSize);
  }

  freeGrid(grid, gridSize);
  delete[] grid;

  return 0;
}
