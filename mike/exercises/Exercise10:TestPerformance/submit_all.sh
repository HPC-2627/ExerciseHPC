#!/bin/bash
# Submits the same benchmark with the 4 PBS placement strategies seen in class:
#   pack, scatter, pack:excl, scatter:excl
# Each job writes its own results_<placement>.csv file.
# Must be run from the project folder (where run.sh is).
for PLACE in pack scatter pack:excl scatter:excl; do
    TAG=${PLACE//:/_}      # ':' is not suitable in file names: scatter:excl -> scatter_excl
    # -N job name; -l resources (2 chunks of 1 core) and placement;
    # -o/-e output and error files; -v passes the PLACE variable to run.sh
    qsub -N pp_${TAG} \
         -l select=2:ncpus=1:mem=2gb -l place=${PLACE} \
         -o pp_${TAG}.log -e pp_${TAG}.err \
         -v PLACE=${PLACE} run.sh
done
