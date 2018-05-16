#!/usr/bin/env bash

# Fail on first error, on undefined variables, and on failures in a pipeline.
set -euo pipefail

# Print each executed line.
set -x

results_path='/laika/results/scalability_range'
mkdir -p "$results_path"

default_compute_params='MASS_SPRING_DASHPOT=1 RUN_EXTRA_WARMUP=1 RUN_FIXED_ROUNDS_EXPERIMENT=1'
iterations='10'


hilbert_data_prefix='./input_data/hilbert-graphs'
unordered_data_prefix='./input_data/random-graphs'


size_4_inputs=('rand.4' 'bunny.4' 'cube.4' 'dragon.4')

cpuset_12_workers='0-5,24-29'
cpuset_24_workers='0-11,24-35'
cpuset_36_workers='0-17,24-41'


measure() {
    input_file=$1
    scheduler=$2
    workers=$3
    cpuset=$4
    info=$5

    hilbert_input="${hilbert_data_prefix}/${input_file}.binadjlist "
    hilbert_input+="${hilbert_data_prefix}/${input_file}.node.simple"

    unordered_input="${unordered_data_prefix}/${input_file}.binadjlist "
    unordered_input+="${unordered_data_prefix}/${input_file}.node.simple"

    taskset -c "${cpuset}" \
        ./src/graph_compute/compute "$iterations" $hilbert_input 2>&1 \
        >"${results_path}/${input_file}-hilbert-${scheduler}${info}-scalability-${workers}.txt"
    taskset -c "${cpuset}" \
        ./src/graph_compute/compute "$iterations" $unordered_input 2>&1 \
        >"${results_path}/${input_file}-unordered-${scheduler}${info}-scalability-${workers}.txt"
}


# #######
# To measure Laika, we have to set CHUNK_BITS appropriately and recompile for each number of cores
# #######

make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D1_NUMA=1 NUMA_WORKERS=36 CHUNK_BITS=16 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "laika" "36" "$cpuset_36_workers" "36"
done

make clean
make PARALLEL=0 build-libgraphio
make PARALLEL=0 D1_NUMA=1 NUMA_WORKERS=1 CHUNK_BITS=16 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "laika" "1" "$cpuset_36_workers" "36"
done


make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D1_NUMA=1 NUMA_WORKERS=24 CHUNK_BITS=16 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "laika" "24" "$cpuset_24_workers" "24"
done

make clean
make PARALLEL=0 build-libgraphio
make PARALLEL=0 D1_NUMA=1 NUMA_WORKERS=1 CHUNK_BITS=16 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "laika" "1" "$cpuset_24_workers" "24"
done


make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D1_NUMA=1 NUMA_WORKERS=12 CHUNK_BITS=17 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "laika" "12" "$cpuset_12_workers" "12"
done


make clean
make PARALLEL=0 build-libgraphio
make PARALLEL=0 D1_NUMA=1 NUMA_WORKERS=1 CHUNK_BITS=17 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "laika" "1" "$cpuset_12_workers" "12"
done


# ### Done measuring Laika ###


make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D0_BSP=1 $default_compute_params build-graph-compute
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "bsp" "36" "$cpuset_36_workers" ""
done
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "bsp" "24" "$cpuset_24_workers" ""
done
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "bsp" "12" "$cpuset_12_workers" ""
done


make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D1_LOCKS=1 $default_compute_params build-graph-compute
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "locks" "36" "$cpuset_36_workers" ""
done
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "locks" "24" "$cpuset_24_workers" ""
done
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "locks" "12" "$cpuset_12_workers" ""
done


make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D1_PRIO=1 $default_compute_params build-graph-compute
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "jp" "36" "$cpuset_36_workers" ""
done
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "jp" "24" "$cpuset_24_workers" ""
done
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "jp" "12" "$cpuset_12_workers" ""
done


make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D1_CHROM=1 $default_compute_params build-graph-compute
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "chroma" "36" "$cpuset_36_workers" ""
done
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "chroma" "24" "$cpuset_24_workers" ""
done
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "chroma" "12" "$cpuset_12_workers" ""
done
