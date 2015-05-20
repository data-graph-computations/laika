#ifndef IO_H_
#define IO_H_

#include <string>
#include "./common.h"

using namespace std;

int readNodesFromFile(const string filepath, vertex_t ** outNodes, vid_t * outCount);

int readEdgesFromFile(const string filepath, vertex_t * nodes, const vid_t cntNodes);

int readUndirectedEdgesFromFile(const string filepath, vertex_t * nodes, vid_t cntNodes);

int outputReorderedGraph(const vertex_t * const reorderedNodes, const vid_t cntNodes,
                         const vid_t * const translationMapping,
                         const string& outputNodeFile, const string& outputEdgeFile);

int outputEdges(const vertex_t * const reorderedNodes, const vid_t cntNodes,
                const vid_t * const translationMapping,
                const string& filepath);

#endif  // IO_H_
