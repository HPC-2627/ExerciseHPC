#!/bin/bash
#PBS -l select=8:ncpus=1:mpiprocs=8:mem=2gb
# set max execution time
#PBS -l walltime=0:00:30
# set the queue
#PBS -q shortCPUQ

module load OpenMPI/4.1.5-GCC-12.3.0
mpirun --oversubscribe -np 8 ./open-closed-ring 'open'