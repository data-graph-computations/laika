#!/usr/bin/env bash

# Fail on first error, on undefined variables, and on failures in a pipeline.
set -euo pipefail

# Print each executed line.
set -x

mkdir -p /laika/results/perf_counters

default_compute_params='PARALLEL=1 MASS_SPRING_DASHPOT=1'
default_compute_params="$default_compute_params RUN_FIXED_ROUNDS_EXPERIMENT=1"


# We run each workload twice:
# - Once for 100 iterations, to measure the "warmup" phase.
# - Then, for 1100 iterations, so we can subtract the "warmup"
#   and get 1000 iterations of pure data.
warmup_iterations=100
measurement_iterations=1100


hilbert_data_prefix='./input_data/hilbert-graphs'
unordered_data_prefix='./input_data/random-graphs'


size_4_inputs=('rand.4' 'bunny.4' 'dragon.4')
size_3_inputs=('rand.3' 'bunny.3' 'cube.3' 'dragon.3')
size_2_inputs=('rand.2' 'bunny.2' 'cube.2' 'dragon.2')
size_1_inputs=('rand.1' 'bunny.1' 'cube.1' 'dragon.1')
size_0_inputs=('rand.0' 'bunny.0' 'cube.0' 'dragon.0')


measure() {
    input_file=$1
    scheduler=$2

    hilbert_input="${hilbert_data_prefix}/${input_file}.binadjlist"
    hilbert_input+="${hilbert_input} ${hilbert_data_prefix}/${input_file}.node.simple"

    unordered_input="${unordered_data_prefix}/${input_file}.binadjlist"
    unordered_input+="${unordered_input} ${unordered_data_prefix}/${input_file}.node.simple"

    sudo perf stat -e \
         longest_lat_cache.miss,cycle_activity.stalls_l3_miss,dTLB-load-misses,dTLB-store-misses \
         -o "/laika/results/perf_counters/${input_file}-hilbert-${scheduler}-warmup.txt" \
         taskset -c 0-47 \
         ./src/graph_compute/compute "$warmup_iterations" "$hilbert_input"
    sudo perf stat -e \
         longest_lat_cache.miss,cycle_activity.stalls_l3_miss,dTLB-load-misses,dTLB-store-misses \
         -o "/laika/results/perf_counters/${input_file}-hilbert-${scheduler}-measure.txt" \
         taskset -c 0-47 \
         ./src/graph_compute/compute "$measurement_iterations" "$hilbert_input"

    sudo perf stat -e \
         longest_lat_cache.miss,cycle_activity.stalls_l3_miss,dTLB-load-misses,dTLB-store-misses \
         -o "/laika/results/perf_counters/${input_file}-unordered-${scheduler}-warmup.txt" \
         taskset -c 0-47 \
         ./src/graph_compute/compute "$warmup_iterations" "unordered_input"
    sudo perf stat -e \
         longest_lat_cache.miss,cycle_activity.stalls_l3_miss,dTLB-load-misses,dTLB-store-misses \
         -o "/laika/results/perf_counters/${input_file}-unordered-${scheduler}-measure.txt" \
         taskset -c 0-47 \
         ./src/graph_compute/compute "$measurement_iterations" "unordered_input"
}


# #######
# To measure Laika, we have to set CHUNK_BITS appropriately and recompile for each input size.
# #######

make clean
make PARALLEL=1 build-libgraphio
make D1_NUMA=1 NUMA_WORKERS=48 CHUNK_BITS=15 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "laika"
done


make clean
make PARALLEL=1 build-libgraphio
make D1_NUMA=1 NUMA_WORKERS=48 CHUNK_BITS=13 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_3_inputs[@]}"; do
    measure "$current_input_file" "laika"
done


make clean
make PARALLEL=1 build-libgraphio
make D1_NUMA=1 NUMA_WORKERS=48 CHUNK_BITS=11 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_2_inputs[@]}"; do
    measure "$current_input_file" "laika"
done


make clean
make PARALLEL=1 build-libgraphio
make D1_NUMA=1 NUMA_WORKERS=48 CHUNK_BITS=9 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_1_inputs[@]}"; do
    measure "$current_input_file" "laika"
done


make clean
make PARALLEL=1 build-libgraphio
make D1_NUMA=1 NUMA_WORKERS=48 CHUNK_BITS=7 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_0_inputs[@]}"; do
    measure "$current_input_file" "laika"
done

# ### Done measuring Laika ###

