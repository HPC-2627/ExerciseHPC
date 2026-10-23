#!/bin/bash
#PBS -l select=2:ncpus=1:mem=2gb
# set max execution time
#PBS -l walltime=0:00:30
# set the queue
#PBS -q shortCPUQ

module load OpenMPI/4.1.5-GCC-12.3.0

cd "$PBS_O_WORKDIR" || exit 1
mpirun  -np 2 ./ping-pong