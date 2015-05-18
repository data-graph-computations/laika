#!/usr/bin/env bash

# break on the first error code returned
set -e

benchroot=$1
output=$2
originalnodes=$3
originaledges=$4

# optimized for 100M graph
chunkbits=19

echo "Benchmark root directory: $benchroot"
echo "Output into: $output"
echo "Input files:"
echo "  $originalnodes"
echo "  $originaledges"

echo "Benchmark root directory: $benchroot" >$output
echo "Output into: $output" >>$output
echo "Input files:" >>$output
echo "  $originalnodes" >>$output
echo "  $originaledges" >>$output
echo "" >>$output

rounds=10

# benchmark the unordered input, parallel and not
parallel=1 ; while [[ $parallel -le 1 ]] ; do
  (make TMP=$benchroot clean-graph-compute) 2>&1 >/dev/null ;
  (make TMP=$benchroot BASELINE=1 PARALLEL=$parallel build-graph-compute) #2>&1 >/dev/null
  echo ""
  echo "Running original data, baseline, parallel=$parallel"
  echo ""
  make TMP=$benchroot ROUNDS=$rounds OUTPUT=$output ORIGINAL_NODES_FILE=$originalnodes ORIGINAL_EDGES_FILE=$originaledges run-original-concat ;
  echo "" >>$output

  (make TMP=$benchroot clean-graph-compute) 2>&1 >/dev/null ;
  (make TMP=$benchroot D0_BSP=1 PARALLEL=$parallel build-graph-compute) #2>&1 >/dev/null
  echo ""
  echo "Running original data, d0-bsp, parallel=$parallel"
  echo ""
  make TMP=$benchroot ROUNDS=$rounds OUTPUT=$output ORIGINAL_NODES_FILE=$originalnodes ORIGINAL_EDGES_FILE=$originaledges run-original-concat ;
  echo "" >>$output

  (make TMP=$benchroot clean-graph-compute) 2>&1 >/dev/null ;
  (make TMP=$benchroot D1_CHUNK=1 CHUNK_BITS=$chunkbits PARALLEL=$parallel build-graph-compute) #2>&1 >/dev/null
  echo ""
  echo "Running original data, chunk ($chunkbits bits), parallel=$parallel"
  echo ""
  make TMP=$benchroot ROUNDS=$rounds OUTPUT=$output ORIGINAL_NODES_FILE=$originalnodes ORIGINAL_EDGES_FILE=$originaledges run-original-concat ;
  echo "" >>$output

  ((parallel = $parallel + 1)) ;
done ;

# for each Hilbert granularity,
# benchmark the baseline code, the fake BSP, and the best optimized one,
# both in parallel and in series
hilbert=1 ; while [[ $hilbert -le 9 ]] ; do
  echo "Reordering with $hilbert Hilbert bits per dimension"

  (make TMP=$benchroot clean-hilbert-reorder) 2>&1 >/dev/null;
  make TMP=$benchroot PARALLEL=1 HILBERTBITS=$hilbert ORIGINAL_NODES_FILE=$originalnodes ORIGINAL_EDGES_FILE=$originaledges reorder-graph ;

  parallel=1 ; while [[ $parallel -le 1 ]] ; do
    (make TMP=$benchroot clean-graph-compute) 2>&1 >/dev/null ;
    (make TMP=$benchroot BASELINE=1 PARALLEL=$parallel build-graph-compute) #2>&1 >/dev/null
    echo ""
    echo "Running reordered (hilbert=$hilbert) data, baseline, parallel=$parallel"
    echo ""
    make TMP=$benchroot ROUNDS=$rounds OUTPUT=$output run-reordered-concat ;
    echo "Hilbert bits: $hilbert" >>$output;
    echo "" >>$output

    (make TMP=$benchroot clean-graph-compute) 2>&1 >/dev/null ;
    (make TMP=$benchroot D0_BSP=1 PARALLEL=$parallel build-graph-compute) #2>&1 >/dev/null
    echo ""
    echo "Running reordered (hilbert=$hilbert) data, d0-bsp, parallel=$parallel"
    echo ""
    make TMP=$benchroot ROUNDS=$rounds OUTPUT=$output run-reordered-concat ;
    echo "Hilbert bits: $hilbert" >>$output;
    echo "" >>$output

    (make TMP=$benchroot clean-graph-compute) 2>&1 >/dev/null ;
    (make TMP=$benchroot D1_CHUNK=1 CHUNK_BITS=$chunkbits PARALLEL=$parallel build-graph-compute) #2>&1 >/dev/null
    echo ""
    echo "Running reordered (hilbert=$hilbert) data, chunk ($chunkbits bits), parallel=$parallel"
    echo ""
    make TMP=$benchroot ROUNDS=$rounds OUTPUT=$output run-reordered-concat ;
    echo "Hilbert bits: $hilbert" >>$output;
    echo "" >>$output

    ((parallel = $parallel + 1)) ;
  done ;

  ((hilbert = $hilbert + 1)) ;
done;
