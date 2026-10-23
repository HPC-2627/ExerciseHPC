#!/bin/bash
# Sottomette lo stesso benchmark con le 4 strategie di posizionamento PBS viste a lezione:
#   pack, scatter, pack:excl, scatter:excl
# Ogni job scrive il proprio file results_<posizionamento>.csv.
# Va eseguito dalla cartella del progetto (dove si trova run.sh).
for PLACE in pack scatter pack:excl scatter:excl; do
    TAG=${PLACE//:/_}      # i ':' non vanno bene nei nomi di file: scatter:excl -> scatter_excl
    # -N nome del job; -l risorse (2 chunk da 1 core) e posizionamento;
    # -o/-e file di output ed errori; -v passa la variabile PLACE a run.sh
    qsub -N pp_${TAG} \
         -l select=2:ncpus=1:mem=2gb -l place=${PLACE} \
         -o pp_${TAG}.log -e pp_${TAG}.err \
         -v PLACE=${PLACE} run.sh
done
