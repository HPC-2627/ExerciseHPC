#!/bin/bash

#PBS -l select=2:ncpus=1:mem=2gb

#PBS -l walltime=0:05:00

#PBS -q shortCPUQ

module load MPICH/3.4.2-GCC-10.3.0

cd "$PBS_O_WORKDIR" || exit 1

mpicc -O2 -o benchmark benchmark.c

mpiexec -n 2 ./mpi_wrapper.sh ./benchmark
