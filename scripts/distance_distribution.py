#!/usr/bin/python

import argparse
import math
import itertools

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('edge_file', help='edge file to analyze')
    args = parser.parse_args()

    with open(args.edge_file, 'r') as inp:
        process_edge_file(inp)

def process_edge_file(edge_file):
    adjlist = "AdjacencyGraph\n"
    n = 0
    m = 0

    first = edge_file.readline()
    assert adjlist == first
    n = int(edge_file.readline())
    m = int(edge_file.readline())

    edge_indexes = [0] * (n+1)
    for i in xrange(n):
        edge_indexes[i] = int(edge_file.readline())
    edge_indexes[n] = m

    edges = [0] * m
    for i in xrange(m):
        edges[i] = int(edge_file.readline())

    for i in xrange(n):
        for j in xrange(edge_indexes[i], edge_indexes[i+1]):
            edges[j] = abs(edges[j] - i)

    process_edge_lengths(edges)

def process_edge_lengths(edges):
    zeroes = sum([1 for x in edges if x == 0])
    print "self edges: %d" % zeroes

    ceillog = [math.ceil(math.log(x, 2)) for x in edges if x != 0]
    ceillog.sort()

    result = dict()
    for k, g in itertools.groupby(ceillog):
        count = 0
        for _ in g:
            count += 1
        print k, count
        result[k] = count

    return result

if __name__ == '__main__':
    main()
