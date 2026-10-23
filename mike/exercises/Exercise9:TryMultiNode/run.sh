#!/bin/bash

# Job name visible in qstat
#PBS -N mpi_reduce_multinode

# Request 2 chunks, each with 4 CPUs and 2GB of RAM
#PBS -l select=2:ncpus=4:mem=2gb

# =========================================================================
# PBS PLACEMENT STRATEGIES (Uncomment ONLY ONE option at a time)
# scatter is currently active
# =========================================================================

# 1. PACK: Groups the chunks on nodes to use as few nodes as possible[cite: 2]
#    (Processes are placed close together on the same node if possible)
# #PBS -l place=pack

# 2. SCATTER: Distributes chunks across as many different nodes as possible[cite: 3]
#    (Maximizes the physical separation of processes on distinct nodes)
#PBS -l place=scatter

# 3. PACK + EXCL: Groups chunks on the same node ensuring exclusive access[cite: 4]
#    (No other user can share the resources of the assigned nodes)
# #PBS -l place=pack:excl

# 4. SCATTER + EXCL: Distributes chunks across different nodes with exclusive allocation[cite: 5]
#    (Maximum physical separation and hardware resources entirely dedicated to the job)
# #PBS -l place=scatter:excl

# =========================================================================

# Maximum execution time limit (walltime)
#PBS -l walltime=00:05:00

# Specify the execution queue on the cluster
#PBS -q shortCPUQ

# Redirect standard output and standard error
#PBS -o mpi_reduce_multinode.log
#PBS -e mpi_reduce_multinode.err

# Navigate to the original working directory from which the job was submitted
cd $PBS_O_WORKDIR

# Load the OpenMPI module and the compatible GCC compiler
module load OpenMPI/4.1.5-GCC-12.3.0

# Clean up previous compilations
make clean

# Compile the C source code via Makefile
make

# Run the MPI program across 8 total processes using the PBS hostfile to map slots correctly
mpiexec -n 8 --hostfile $PBS_NODEFILE ./reduce_mpi