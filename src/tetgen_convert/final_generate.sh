#!/bin/bash

../graphgen2/graphgen2 105792 15 /scratch/jjthomas/final-graphs/rand.0.node.simple /scratch/jjthomas/final-graphs/rand.0.adjlist
../graphgen2/graphgen2 397165 15 /scratch/jjthomas/final-graphs/rand.1.node.simple /scratch/jjthomas/final-graphs/rand.1.adjlist
../graphgen2/graphgen2 1698509 15 /scratch/jjthomas/final-graphs/rand.2.node.simple /scratch/jjthomas/final-graphs/rand.2.adjlist
../graphgen2/graphgen2 6363260 15 /scratch/jjthomas/final-graphs/rand.3.node.simple /scratch/jjthomas/final-graphs/rand.3.adjlist

(
  cd ../hilbert_reorder
  make clean; make PARALLEL=1 RANDOM=1
)

for type in "rand" "cube" "dragon" "bunny"; do
  for i in {0..3}; do
    ../hilbert_reorder/reorder /scratch/jjthomas/final-graphs/$type.$i.node.simple /scratch/jjthomas/final-graphs/$type.$i.adjlist /scratch/jjthomas/random-graphs/$type.$i.node.simple /scratch/jjthomas/random-graphs/$type.$i.binadjlist
  done
done

(
  cd ../hilbert_reorder
  make clean; make PARALLEL=1 HILBERTBITS=9
)

for type in "rand" "cube" "dragon" "bunny"; do
  for i in {0..3}; do
    ../hilbert_reorder/reorder /scratch/jjthomas/final-graphs/$type.$i.node.simple /scratch/jjthomas/final-graphs/$type.$i.adjlist /scratch/jjthomas/hilbert-graphs/$type.$i.node.simple /scratch/jjthomas/hilbert-graphs/$type.$i.binadjlist
  done
done
