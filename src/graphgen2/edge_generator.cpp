#include "./edge_generator.h"
#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>
#include <tuple>
#include <exception>

#include "./common.h"

#define CELL_SIZE ((MAX_COORD - MIN_COORD) / GRID_SIZE)
#define GRID_TOTAL_COUNT (GRID_SIZE * GRID_SIZE * GRID_SIZE)

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
                                      const vid_t x, const vid_t y, const vid_t z) {
  WHEN_DEBUG({
    std::cout << "Getting vector for grid cell: ("
              << x << ", " << y << ", " << z << ")" << std::endl;
  })

  // vid_t are unsigned, can't meaningfully compare against zero
  // but < GRID_SIZE also ensures they aren't "negative"
  assert(x < GRID_SIZE);
  assert(y < GRID_SIZE);
  assert(z < GRID_SIZE);

  vid_t index = (x * GRID_SIZE + y) * GRID_SIZE + z;
  assert(index < GRID_TOTAL_COUNT);
  return &grid[index];
}

static inline vid_t toGrid(const double x) {
  assert(x >= MIN_COORD);
  assert(x <= MAX_COORD);
  vid_t result = static_cast<vid_t>((x - MIN_COORD) / CELL_SIZE);

  assert(result < GRID_SIZE);
  return result;
}

static inline void appendToGridCell(std::vector<vid_t> ** const grid,
                                    const vid_t x, const vid_t y, const vid_t z,
                                    const vid_t node) {
  std::vector<vid_t> ** gridVectorPtr = getVectorInGrid(grid, x, y, z);
  if (*gridVectorPtr == NULL) {
    *gridVectorPtr = new std::vector<vid_t>;
  }
  (*gridVectorPtr)->push_back(node);
}

static void distributeNodesToGrid(vertex_t * const nodes,
                                  const vid_t cntNodes,
                                  std::vector<vid_t> ** const grid) {
  for (vid_t i = 0; i < cntNodes; ++i) {
    vid_t gridX, gridY, gridZ;
    gridX = toGrid(nodes[i].x);
    gridY = toGrid(nodes[i].y);
    gridZ = toGrid(nodes[i].z);
    appendToGridCell(grid, gridX, gridY, gridZ, i);
  }
}

static void generateEdgesForNode(vid_t index,
                                 vertex_t * const nodes,
                                 std::vector<vid_t> * const edges,
                                 const vid_t cntNodes,
                                 const double maxEdgeLength,
                                 std::vector<vid_t> ** const grid) {
  vid_t gridX, gridY, gridZ;
  gridX = toGrid(nodes[index].x);
  gridY = toGrid(nodes[index].y);
  gridZ = toGrid(nodes[index].z);

  vid_t lowX, lowY, lowZ, highX, highY, highZ;
  // vid_t are unsigned, can't use max because of underflow
  lowX = (gridX == 0) ? 0 : (gridX - 1);
  lowY = (gridY == 0) ? 0 : (gridY - 1);
  lowZ = (gridZ == 0) ? 0 : (gridZ - 1);
  highX = std::min(static_cast<vid_t>(GRID_SIZE-1), gridX + 1);
  highY = std::min(static_cast<vid_t>(GRID_SIZE-1), gridY + 1);
  highZ = std::min(static_cast<vid_t>(GRID_SIZE-1), gridZ + 1);

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
        auto gridCell = *getVectorInGrid(grid, x, y, z);
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

static void freeGrid(std::vector<vid_t> ** const grid) {
  for (vid_t i = 0; i < GRID_TOTAL_COUNT; ++i) {
    if (grid[i] != NULL) {
      delete grid[i];
    }
  }
}

static void calculateGridStats(const vid_t cntNodes,
                               std::vector<vid_t> ** const grid) {
  vid_t emptyCells = 0;
  int maxNodesInCell = 0;
  vid_t totalNodesInCells = 0;

  for (vid_t i = 0; i < GRID_TOTAL_COUNT; ++i) {
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
  avgNodesPerCell /= (GRID_TOTAL_COUNT - emptyCells);

  std::cout << "\nGrid stats:\n";
  std::cout << "  empty cells: " << emptyCells << '\n';
  std::cout << "  max nodes in cell: " << maxNodesInCell << '\n';
  std::cout << "  avg nodes per nonempty cell: " << avgNodesPerCell << '\n';
  std::cout << std::endl;
}

int generateEdges(vertex_t * const nodes,
                  std::vector<vid_t> * const edges,
                  const vid_t cntNodes,
                  const double maxEdgeLength) {
  // ensure no node can have an edge more than one grid cell away
  assert(maxEdgeLength < ((MAX_COORD - MIN_COORD) / GRID_SIZE));

  static std::vector<vid_t> * grid[GRID_TOTAL_COUNT];
  memset(grid, 0, GRID_TOTAL_COUNT * sizeof(std::vector<vid_t> *));

  distributeNodesToGrid(nodes, cntNodes, grid);

  calculateGridStats(cntNodes, grid);

  cilk_for (vid_t i = 0; i < cntNodes; ++i) {
    generateEdgesForNode(i, nodes, edges, cntNodes, maxEdgeLength, grid);
  }

  freeGrid(grid);

  return 0;
}
