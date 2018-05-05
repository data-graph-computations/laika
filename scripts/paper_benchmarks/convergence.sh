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

make clean
make PARALLEL=1 build-libgraphio
make D1_NUMA=1 NUMA_WORKERS=48 CHUNK_BITS=13 \
     $default_compute_params build-graph-compute
taskset -c 0-47 ./src/graph_compute/compute "$convergence_factor" \
    ./input_data/hilbert-graphs/${input_name}.binadjlist \
    ./input_data/hilbert-graphs/${input_name}.simple \
    >${input_name}-hilbert-parallel-convergence-laika.txt


make clean
make PARALLEL=1 build-libgraphio
make D0_BSP=1 $default_compute_params build-graph-compute
taskset -c 0-47 ./src/graph_compute/compute "$convergence_factor" \
    ./input_data/hilbert-graphs/${input_name}.binadjlist \
    ./input_data/hilbert-graphs/${input_name}.node.simple \
    >${input_name}-hilbert-parallel-convergence-laika.txt

