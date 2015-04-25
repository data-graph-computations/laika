#!/usr/bin/env bash

# break on the first error code returned
set -e

benchroot=$1
output=$2
originalnodes=$3
originaledges=$4

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

parallel=0 ; while [[ $parallel -le 1 ]] ; do
  (make TMP=$benchroot PARALLEL=$parallel build-graph-compute-baseline) #2>&1 >/dev/null
  rounds=1000 ; while [[ $rounds -le 1000 ]] ; do
    echo ""
    echo "Running original data, baseline, parallel=$parallel"
    echo ""
    make TMP=$benchroot ROUNDS=$rounds OUTPUT=$output ORIGINAL_NODES_FILE=$originalnodes ORIGINAL_EDGES_FILE=$originaledges run-original-concat ;
    echo "" >>$output
    ((rounds = $rounds * 10)) ;
  done ;
  ((parallel = $parallel + 1)) ;
done ;

# reordering options:
# number of hilbert bits per dimension: [1,10]
hilbert=5 ; while [[ $hilbert -le 9 ]] ; do
  echo "Reordering with $hilbert Hilbert bits per dimension"

  (make TMP=$benchroot clean-hilbert-reorder) 2>&1 >/dev/null;
  make TMP=$benchroot PARALLEL=1 HILBERTBITS=$hilbert ORIGINAL_NODES_FILE=$originalnodes ORIGINAL_EDGES_FILE=$originaledges reorder-graph ;

  # runtime options:
  # priority bits: [1,24]
  # parallelism: on / off
  priority=4 ; while [[ $priority -le 16 ]] ; do
    parallel=0 ; while [[ $parallel -le 1 ]] ; do
      (make TMP=$benchroot clean-graph-compute) 2>&1 >/dev/null ;

      rounds=1000 ; while [[ $rounds -le 1000 ]] ; do
        (make TMP=$benchroot PARALLEL=$parallel PRIORITY_GROUP_BITS=$priority build-graph-compute-optimized) # 2>&1 >/dev/null

        # echo ""
        # echo "Running original data, priority=$priority, parallel=$parallel, rounds=$rounds"
        # echo ""
        # make TMP=$benchroot PRIORITY_GROUP_BITS=$priority PARALLEL=$parallel ROUNDS=$rounds OUTPUT=$output ORIGINAL_NODES_FILE=$originalnodes ORIGINAL_EDGES_FILE=$originaledges run-original-concat ;
        # echo "" >>$output;

        echo ""
        echo "Running reordered data, priority=$priority, parallel=$parallel, rounds=$rounds"
        echo ""
        make TMP=$benchroot ROUNDS=$rounds OUTPUT=$output run-reordered-concat ;
        echo "" >>$output;
        ((rounds = $rounds * 10)) ;
      done ;
      ((parallel = $parallel + 1)) ;
      done ;
    ((priority = $priority + 1)) ;
  done ;
  ((hilbert = $hilbert + 1)) ;
done;
