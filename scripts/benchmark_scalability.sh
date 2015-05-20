#!/usr/bin/env bash

# break on the first error code returned
set -e

benchroot=$1
output=$2
originalnodes=$3
originaledges=$4

chunkbits=16
hilbert=10

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

nworkers=2 ; while [[ $nworkers -le 12 ]] ; do
  export CILK_NWORKERS=$nworkers ;

  (make TMP=$benchroot clean-graph-compute) 2>&1 >/dev/null ;
  (make TMP=$benchroot BASELINE=1 PARALLEL=1 build-graph-compute) #2>&1 >/dev/null
  echo ""
  echo "Running original data, baseline, parallel=1, nworkers=$nworkers"
  echo ""
  make TMP=$benchroot ROUNDS=$rounds OUTPUT=$output ORIGINAL_NODES_FILE=$originalnodes ORIGINAL_EDGES_FILE=$originaledges run-original-concat ;
  echo "" >>$output

  (make TMP=$benchroot clean-graph-compute) 2>&1 >/dev/null ;
  (make TMP=$benchroot D0_BSP=1 PARALLEL=1 build-graph-compute) #2>&1 >/dev/null
  echo ""
  echo "Running original data, d0-bsp, parallel=1, nworkers=$nworkers"
  echo ""
  make TMP=$benchroot ROUNDS=$rounds OUTPUT=$output ORIGINAL_NODES_FILE=$originalnodes ORIGINAL_EDGES_FILE=$originaledges run-original-concat ;
  echo "" >>$output

  (make TMP=$benchroot clean-graph-compute) 2>&1 >/dev/null ;
  (make TMP=$benchroot D1_CHUNK=1 CHUNK_BITS=$chunkbits PARALLEL=1 build-graph-compute) #2>&1 >/dev/null
  echo ""
  echo "Running original data, chunk ($chunkbits bits), parallel=1, nworkers=$nworkers"
  echo ""
  make TMP=$benchroot ROUNDS=$rounds OUTPUT=$output ORIGINAL_NODES_FILE=$originalnodes ORIGINAL_EDGES_FILE=$originaledges run-original-concat ;
  echo "" >>$output

  ((nworkers = $nworkers + 1)) ;
done ;

(make TMP=$benchroot clean-hilbert-reorder) 2>&1 >/dev/null;
make TMP=$benchroot PARALLEL=1 HILBERTBITS=$hilbert ORIGINAL_NODES_FILE=$originalnodes ORIGINAL_EDGES_FILE=$originaledges reorder-graph ;

nworkers=2 ; while [[ $nworkers -le 12 ]] ; do
  export CILK_NWORKERS=$nworkers ;

  (make TMP=$benchroot clean-graph-compute) 2>&1 >/dev/null ;
  (make TMP=$benchroot BASELINE=1 PARALLEL=1 build-graph-compute) #2>&1 >/dev/null
  echo ""
  echo "Running reordered data, baseline, parallel=1, nworkers=$nworkers"
  echo ""
  make TMP=$benchroot ROUNDS=$rounds OUTPUT=$output run-reordered-concat ;
  echo "Hilbert bits: $hilbert" >>$output;
  echo "" >>$output

  (make TMP=$benchroot clean-graph-compute) 2>&1 >/dev/null ;
  (make TMP=$benchroot D0_BSP=1 PARALLEL=1 build-graph-compute) #2>&1 >/dev/null
  echo ""
  echo "Running reordered data, d0-bsp, parallel=1, nworkers=$nworkers"
  echo ""
  make TMP=$benchroot ROUNDS=$rounds OUTPUT=$output run-reordered-concat ;
  echo "Hilbert bits: $hilbert" >>$output;
  echo "" >>$output

  (make TMP=$benchroot clean-graph-compute) 2>&1 >/dev/null ;
  (make TMP=$benchroot D1_CHUNK=1 CHUNK_BITS=$chunkbits PARALLEL=1 build-graph-compute) #2>&1 >/dev/null
  echo ""
  echo "Running reordered data, chunk ($chunkbits bits), parallel=1, nworkers=$nworkers"
  echo ""
  make TMP=$benchroot ROUNDS=$rounds OUTPUT=$output run-reordered-concat ;
  echo "Hilbert bits: $hilbert" >>$output;
  echo "" >>$output

  ((nworkers = $nworkers + 1)) ;
done ;

unset CILK_NWORKERS ;
