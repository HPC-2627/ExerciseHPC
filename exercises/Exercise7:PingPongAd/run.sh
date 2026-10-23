#!/bin/bash

# Set the name of the batch job (visible in qstat)
#PBS -N mpi_pingpong_game

# Request 1 compute node with 8 processor cores on that single node
#PBS -l nodes=1:ppn=8

# Set the maximum wall-clock time limit for the job
#PBS -l walltime=00:02:00

# Specify the target execution queue on the cluster
#PBS -q shortCPUQ

# Redirect standard output to a fixed log file
#PBS -o mpi_pingpong_game.log

# Redirect standard error to a fixed error file
#PBS -e mpi_pingpong_game.err

# Navigate back to the exact directory where the qsub command was submitted from
cd $PBS_O_WORKDIR

# Load the required OpenMPI module and its matching GCC compiler toolchain
module load OpenMPI/4.1.5-GCC-12.3.0

# Remove any previously compiled binaries or leftover log files for a clean build
make clean

# Compile the C source code using the Makefile and mpicc
make

# Launch the compiled MPI program across 2 parallel processes
mpirun -np 2 ./pingpong_game_mpi