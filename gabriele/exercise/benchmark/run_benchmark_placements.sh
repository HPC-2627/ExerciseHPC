#!/bin/bash
# Submits benchmark.sh once per PBS placement strategy, so results can be
# compared across same-node vs. cross-node and shared vs. exclusive allocation.
#
# Usage: ./run_benchmark_placements.sh
# (run from the directory that holds benchmark.sh, benchmark.c, mpi_wrapper.sh)

STRATEGIES="pack scatter pack:excl scatter:excl"

for strategy in $STRATEGIES; do
    tag=$(echo "$strategy" | tr ':' '_')
    echo "Submitting benchmark with place=$strategy"
    qsub -l place="$strategy" \
         -N "benchmark_${tag}" \
         -o "benchmark_${tag}.out" \
         -e "benchmark_${tag}.err" \
         benchmark.sh
done
