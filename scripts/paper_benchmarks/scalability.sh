#!/usr/bin/env bash

# Fail on first error, on undefined variables, and on failures in a pipeline.
set -euo pipefail

# Print each executed line.
set -x

results_path='/laika/results/scalability'
mkdir -p "$results_path"

default_compute_params='MASS_SPRING_DASHPOT=1 RUN_EXTRA_WARMUP=1 RUN_FIXED_ROUNDS_EXPERIMENT=1'
iterations='10'


hilbert_data_prefix='./input_data/hilbert-graphs'
unordered_data_prefix='./input_data/random-graphs'


size_4_inputs=('rand.4' 'bunny.4' 'cube.4' 'dragon.4')
size_3_inputs=('rand.3' 'bunny.3' 'cube.3' 'dragon.3')
size_2_inputs=('rand.2' 'bunny.2' 'cube.2' 'dragon.2')
size_1_inputs=('rand.1' 'bunny.1' 'cube.1' 'dragon.1')
size_0_inputs=('rand.0' 'bunny.0' 'cube.0' 'dragon.0')


measure() {
    input_file=$1
    scheduler=$2
    workers=$3

    hilbert_input="${hilbert_data_prefix}/${input_file}.binadjlist "
    hilbert_input+="${hilbert_data_prefix}/${input_file}.node.simple"

    unordered_input="${unordered_data_prefix}/${input_file}.binadjlist "
    unordered_input+="${unordered_data_prefix}/${input_file}.node.simple"

    taskset -c 0-47 \
        ./src/graph_compute/compute "$iterations" $hilbert_input 2>&1 \
        >"${results_path}/${input_file}-hilbert-${scheduler}-scalability-${workers}.txt"
    taskset -c 0-47 \
        ./src/graph_compute/compute "$iterations" $unordered_input 2>&1 \
        >"${results_path}/${input_file}-unordered-${scheduler}-scalability-${workers}.txt"
}


# #######
# To measure Laika, we have to set CHUNK_BITS appropriately and recompile for each input size.
# #######

make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D1_NUMA=1 NUMA_WORKERS=48 CHUNK_BITS=15 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "laika" "48"
done

make clean
make PARALLEL=0 build-libgraphio
make PARALLEL=0 D1_NUMA=1 NUMA_WORKERS=1 CHUNK_BITS=15 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "laika" "1"
done


make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D1_NUMA=1 NUMA_WORKERS=48 CHUNK_BITS=13 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_3_inputs[@]}"; do
    measure "$current_input_file" "laika" "48"
done

make clean
make PARALLEL=0 build-libgraphio
make PARALLEL=0 D1_NUMA=1 NUMA_WORKERS=1 CHUNK_BITS=13 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_3_inputs[@]}"; do
    measure "$current_input_file" "laika" "1"
done


make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D1_NUMA=1 NUMA_WORKERS=48 CHUNK_BITS=11 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_2_inputs[@]}"; do
    measure "$current_input_file" "laika" "48"
done

make clean
make PARALLEL=0 build-libgraphio
make PARALLEL=0 D1_NUMA=1 NUMA_WORKERS=1 CHUNK_BITS=11 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_2_inputs[@]}"; do
    measure "$current_input_file" "laika" "1"
done


make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D1_NUMA=1 NUMA_WORKERS=48 CHUNK_BITS=9 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_1_inputs[@]}"; do
    measure "$current_input_file" "laika" "48"
done

make clean
make PARALLEL=0 build-libgraphio
make PARALLEL=0 D1_NUMA=1 NUMA_WORKERS=1 CHUNK_BITS=9 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_1_inputs[@]}"; do
    measure "$current_input_file" "laika" "1"
done


make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D1_NUMA=1 NUMA_WORKERS=48 CHUNK_BITS=7 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_0_inputs[@]}"; do
    measure "$current_input_file" "laika" "48"
done

make clean
make PARALLEL=0 build-libgraphio
make PARALLEL=0 D1_NUMA=1 NUMA_WORKERS=1 CHUNK_BITS=7 \
     $default_compute_params build-graph-compute

for current_input_file in "${size_0_inputs[@]}"; do
    measure "$current_input_file" "laika" "1"
done

# ### Done measuring Laika ###


make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D0_BSP=1 $default_compute_params build-graph-compute
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "bsp" "48"
done
for current_input_file in "${size_3_inputs[@]}"; do
    measure "$current_input_file" "bsp" "48"
done
for current_input_file in "${size_2_inputs[@]}"; do
    measure "$current_input_file" "bsp" "48"
done
for current_input_file in "${size_1_inputs[@]}"; do
    measure "$current_input_file" "bsp" "48"
done
for current_input_file in "${size_0_inputs[@]}"; do
    measure "$current_input_file" "bsp" "48"
done

make clean
make PARALLEL=0 build-libgraphio
make PARALLEL=0 D0_BSP=1 $default_compute_params build-graph-compute
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "bsp" "1"
done
for current_input_file in "${size_3_inputs[@]}"; do
    measure "$current_input_file" "bsp" "1"
done
for current_input_file in "${size_2_inputs[@]}"; do
    measure "$current_input_file" "bsp" "1"
done
for current_input_file in "${size_1_inputs[@]}"; do
    measure "$current_input_file" "bsp" "1"
done
for current_input_file in "${size_0_inputs[@]}"; do
    measure "$current_input_file" "bsp" "1"
done


make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D1_LOCKS=1 $default_compute_params build-graph-compute
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "locks" "48"
done
for current_input_file in "${size_3_inputs[@]}"; do
    measure "$current_input_file" "locks" "48"
done
for current_input_file in "${size_2_inputs[@]}"; do
    measure "$current_input_file" "locks" "48"
done
for current_input_file in "${size_1_inputs[@]}"; do
    measure "$current_input_file" "locks" "48"
done
for current_input_file in "${size_0_inputs[@]}"; do
    measure "$current_input_file" "locks" "48"
done

make clean
make PARALLEL=0 build-libgraphio
make PARALLEL=0 D1_LOCKS=1 $default_compute_params build-graph-compute
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "locks" "1"
done
for current_input_file in "${size_3_inputs[@]}"; do
    measure "$current_input_file" "locks" "1"
done
for current_input_file in "${size_2_inputs[@]}"; do
    measure "$current_input_file" "locks" "1"
done
for current_input_file in "${size_1_inputs[@]}"; do
    measure "$current_input_file" "locks" "1"
done
for current_input_file in "${size_0_inputs[@]}"; do
    measure "$current_input_file" "locks" "1"
done


make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D1_PRIO=1 $default_compute_params build-graph-compute
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "jp" "48"
done
for current_input_file in "${size_3_inputs[@]}"; do
    measure "$current_input_file" "jp" "48"
done
for current_input_file in "${size_2_inputs[@]}"; do
    measure "$current_input_file" "jp" "48"
done
for current_input_file in "${size_1_inputs[@]}"; do
    measure "$current_input_file" "jp" "48"
done
for current_input_file in "${size_0_inputs[@]}"; do
    measure "$current_input_file" "jp" "48"
done

make clean
make PARALLEL=0 build-libgraphio
make PARALLEL=0 D1_PRIO=1 $default_compute_params build-graph-compute
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "jp" "1"
done
for current_input_file in "${size_3_inputs[@]}"; do
    measure "$current_input_file" "jp" "1"
done
for current_input_file in "${size_2_inputs[@]}"; do
    measure "$current_input_file" "jp" "1"
done
for current_input_file in "${size_1_inputs[@]}"; do
    measure "$current_input_file" "jp" "1"
done
for current_input_file in "${size_0_inputs[@]}"; do
    measure "$current_input_file" "jp" "1"
done


make clean
make PARALLEL=1 build-libgraphio
make PARALLEL=1 D1_CHROM=1 $default_compute_params build-graph-compute
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "chroma" "48"
done
for current_input_file in "${size_3_inputs[@]}"; do
    measure "$current_input_file" "chroma" "48"
done
for current_input_file in "${size_2_inputs[@]}"; do
    measure "$current_input_file" "chroma" "48"
done
for current_input_file in "${size_1_inputs[@]}"; do
    measure "$current_input_file" "chroma" "48"
done
for current_input_file in "${size_0_inputs[@]}"; do
    measure "$current_input_file" "chroma" "48"
done

make clean
make PARALLEL=0 build-libgraphio
make PARALLEL=0 D1_CHROM=1 $default_compute_params build-graph-compute
for current_input_file in "${size_4_inputs[@]}"; do
    measure "$current_input_file" "chroma" "1"
done
for current_input_file in "${size_3_inputs[@]}"; do
    measure "$current_input_file" "chroma" "1"
done
for current_input_file in "${size_2_inputs[@]}"; do
    measure "$current_input_file" "chroma" "1"
done
for current_input_file in "${size_1_inputs[@]}"; do
    measure "$current_input_file" "chroma" "1"
done
for current_input_file in "${size_0_inputs[@]}"; do
    measure "$current_input_file" "chroma" "1"
done
