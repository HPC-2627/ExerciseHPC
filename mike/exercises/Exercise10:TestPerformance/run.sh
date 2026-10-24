#!/bin/bash
# Point-to-point MPI benchmark: 2 processes, 1 core each.
# The placement (pack / scatter / pack:excl / scatter:excl) is chosen at submission time:
#   qsub -N pp_scatter -v PLACE=scatter -l place=scatter run.sh
# (see submit_all.sh). The default value below is scatter.

# Job name (visible in qstat)
#PBS -N mpi_pingpong_bench
# 2 chunks, each with 1 core and 2 GB of RAM
#PBS -l select=2:ncpus=1:mem=2gb
# Default placement: chunks on different nodes
#PBS -l place=scatter
# Maximum run time
#PBS -l walltime=00:05:00
# Execution queue
#PBS -q shortCPUQ
# Standard output and error files
#PBS -o mpi_pingpong_bench.log
#PBS -e mpi_pingpong_bench.err

# Go back to the folder qsub was launched from
cd $PBS_O_WORKDIR

# Load OpenMPI with the matching GCC compiler
module load OpenMPI/4.1.5-GCC-12.3.0

# Chosen placement (if not passed with -v, it is scatter)
PLACE=${PLACE:-scatter}
TAG=${PLACE//:/_}                 # scatter:excl -> scatter_excl
OUT=results_${TAG}.csv            # CSV file with the results of this job

# NOTE: compile ONLY ONCE before submitting (make). Do not run 'make clean' here:
# the 4 jobs share the same folder and would delete each other's executable.
[ -x ./pingpong_bench ] || { echo 'Eseguire prima make' >&2; exit 1; }

# Nodes assigned by PBS (useful for debugging in the .err file)
echo "PBS_NODEFILE:" >&2; cat "$PBS_NODEFILE" >&2

# OpenMPI does not see the second node by itself with place=scatter: we explicitly pass it
# the list of nodes assigned by PBS (--hostfile). Without it, it sees only 1 slot -> "not enough slots".
# Options used:
#   -np 2 --map-by node : 2 processes, one per node
#   plm_rsh_args        : ssh without host-key confirmation prompts
#   btl ^ofi            : disables the OFI transport
# Benchmark parameters: max_exp=20 (1 MiB), 200 repetitions, 20 warm-up.
# One row per message size; the header '# rank0_host=... same_node=...'
# shows the actual placement.
# For the Ethernet-only setup of the slides use: mpirun ... ./mpi_wrapper.sh ./pingpong_bench ...
mpirun -np 2 --hostfile "$PBS_NODEFILE" --map-by node \
       --mca plm_rsh_args "-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR" \
       --mca btl ^ofi \
       ./mpi_wrapper.sh ./pingpong_bench 20 200 20 > "$OUT"
