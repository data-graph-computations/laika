#!/usr/bin/python

from scipy.spatial import cKDTree
import numpy as np
import argparse
import random

# See http://www.cs.cmu.edu/~pbbs/benchmarks/graphIO.html for edge data format info.
# Uses tetgen node data format (with less whitespace).
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('node_file', help='file to save node data to')
    parser.add_argument('edge_file', help='file to save edge data to')

    args = parser.parse_args()

    NUM_POINTS = 1000
    MIN_COORDINATE = 0.0
    MAX_COORDINATE = 1023.0
    MIN_EDGES = 5
    MAX_EDGES = 15
    def gen_point():
        x = random.uniform(MIN_COORDINATE, MAX_COORDINATE)
        y = random.uniform(MIN_COORDINATE, MAX_COORDINATE)
        z = random.uniform(MIN_COORDINATE, MAX_COORDINATE)
        return (x, y, z)

    def gen_edge_count():
        return random.randint(MIN_EDGES, MAX_EDGES)

    with open(args.node_file, 'w') as point_stream:
        with open(args.edge_file, 'w') as edge_stream:
            generate_graph(NUM_POINTS, gen_point, gen_edge_count, \
                           edge_stream, point_stream)

# Generates a graph according to the specified parameters and saves it to the given files.
def generate_graph(num_points, gen_point, gen_edge_count, edge_stream, point_stream):
    points = [gen_point() for i in xrange(num_points)]
    edge_counts = [gen_edge_count() for i in xrange(num_points)]

    write_edge_file_header(num_points, edge_counts, edge_stream)
    write_point_file(points, point_stream)

    kdtree = cKDTree(points)
    for pt, num_edges in zip(points, edge_counts):
        _, ids = kdtree.query(pt, k=num_edges)
        write_edges(ids, edge_stream)

# Writes the header and offset indices for the edge file.
def write_edge_file_header(num_points, edge_counts, edge_stream):
    num_edges = sum(edge_counts)

    edge_stream.write('AdjacencyGraph\n')
    edge_stream.write('{}\n'.format(num_points))
    edge_stream.write('{}\n'.format(num_edges))

    offset_counter = 0
    for x in edge_counts:
        edge_stream.write(str(offset_counter) + '\n')
        offset_counter += x

# Writes all edge destinations for a node.
# (must be called once per node id, with consecutive ids)
def write_edges(edge_destinations, edge_stream):
    for dest in edge_destinations:
        edge_stream.write('{}\n'.format(dest))

# Write point data
def write_point_file(points, point_stream):
    point_stream.write('{} 3 0 0\n'.format(len(points)))
    for i, pt in enumerate(points):
        point_stream.write('{0} {1[0]} {1[1]} {1[2]}\n'.format(i, pt))
    point_stream.write('# Generated by graphgen.py')


if __name__ == '__main__':
    main()
