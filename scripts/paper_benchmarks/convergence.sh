#!/usr/bin/env bash

# Fail on first error, on undefined variables, and on failures in a pipeline.
set -euo pipefail

# Print each executed line.
set -x

mkdir -p /laika/results

convergence_factor='0.000000075'

default_compute_params='PARALLEL=1 MASS_SPRING_DASHPOT=1'
default_compute_params="$default_compute_params RUN_CONVERGENCE_EXPERIMENT=1"

input_name='rand.2'

hilbert_inputs="./input_data/hilbert-graphs/${input_name}.binadjlist"
hilbert_inputs="$hilbert_inputs ./input_data/hilbert-graphs/${input_name}.node.simple"

unordered_inputs="./input_data/random-graphs/${input_name}.binadjlist"
unordered_inputs="$unordered_inputs ./input_data/random-graphs/${input_name}.node.simple"


make clean
make PARALLEL=1 build-libgraphio
make D1_NUMA=1 NUMA_WORKERS=48 CHUNK_BITS=11 \
     $default_compute_params build-graph-compute
taskset -c 0-47 ./src/graph_compute/compute "$convergence_factor" \
    $hilbert_inputs \
    >./results/${input_name}-hilbert-parallel-convergence-laika.txt


make clean
make PARALLEL=1 build-libgraphio
make D0_BSP=1 $default_compute_params build-graph-compute
taskset -c 0-47 ./src/graph_compute/compute "$convergence_factor" \
    $hilbert_inputs \
    >./results/${input_name}-hilbert-parallel-convergence-bsp.txt


make clean
make PARALLEL=1 build-libgraphio
make D1_CHROM=1 $default_compute_params build-graph-compute
taskset -c 0-47 ./src/graph_compute/compute "$convergence_factor" \
    $hilbert_inputs \
    >./results/${input_name}-hilbert-parallel-convergence-chroma.txt


make clean
make PARALLEL=1 build-libgraphio
make D1_LOCKS=1 $default_compute_params build-graph-compute
taskset -c 0-47 ./src/graph_compute/compute "$convergence_factor" \
    $hilbert_inputs \
    >./results/${input_name}-hilbert-parallel-convergence-locks.txt


make clean
make PARALLEL=1 build-libgraphio
make D1_PRIO=1 $default_compute_params build-graph-compute
taskset -c 0-47 ./src/graph_compute/compute "$convergence_factor" \
    $hilbert_inputs \
    >./results/${input_name}-hilbert-parallel-convergence-jp.txt
