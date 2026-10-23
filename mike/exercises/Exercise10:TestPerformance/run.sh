#!/bin/bash
# Benchmark MPI punto-punto: 2 processi, 1 core ciascuno.
# Il posizionamento (pack / scatter / pack:excl / scatter:excl) si sceglie alla sottomissione:
#   qsub -N pp_scatter -v PLACE=scatter -l place=scatter run.sh
# (vedi submit_all.sh). Il valore di default qui sotto e' scatter.

# Nome del job (visibile in qstat)
#PBS -N mpi_pingpong_bench
# 2 chunk, ciascuno con 1 core e 2 GB di RAM
#PBS -l select=2:ncpus=1:mem=2gb
# Posizionamento di default: chunk su nodi diversi
#PBS -l place=scatter
# Tempo massimo di esecuzione
#PBS -l walltime=00:05:00
# Coda di esecuzione
#PBS -q shortCPUQ
# File di output standard ed errori
#PBS -o mpi_pingpong_bench.log
#PBS -e mpi_pingpong_bench.err

# Torna nella cartella da cui e' stato lanciato qsub
cd $PBS_O_WORKDIR

# Carica OpenMPI con il relativo compilatore GCC
module load OpenMPI/4.1.5-GCC-12.3.0

# Posizionamento scelto (se non passato con -v, vale scatter)
PLACE=${PLACE:-scatter}
TAG=${PLACE//:/_}                 # scatter:excl -> scatter_excl
OUT=results_${TAG}.csv            # file CSV con i risultati di questo job

# NOTA: compilare UNA SOLA VOLTA prima di sottomettere (make). Non eseguire 'make clean' qui:
# i 4 job condividono la stessa cartella e si cancellerebbero a vicenda l'eseguibile.
[ -x ./pingpong_bench ] || { echo 'Eseguire prima make' >&2; exit 1; }

# Nodi assegnati da PBS (utile per il debug nel file .err)
echo "PBS_NODEFILE:" >&2; cat "$PBS_NODEFILE" >&2

# OpenMPI non vede da solo il secondo nodo con place=scatter: gli passiamo esplicitamente
# la lista dei nodi assegnati da PBS (--hostfile). Senza, vede solo 1 slot -> "not enough slots".
# Opzioni usate:
#   -np 2 --map-by node : 2 processi, uno per nodo
#   plm_rsh_args        : ssh senza richieste di conferma sulle chiavi host
#   btl ^ofi            : disabilita il trasporto OFI
# Parametri del benchmark: max_exp=20 (1 MiB), 200 ripetizioni, 20 di riscaldamento.
# Una riga per ogni dimensione del messaggio; l'intestazione '# rank0_host=... same_node=...'
# dimostra il posizionamento effettivo.
# Per il setup solo-Ethernet delle slide usare: mpirun ... ./mpi_wrapper.sh ./pingpong_bench ...
mpirun -np 2 --hostfile "$PBS_NODEFILE" --map-by node \
       --mca plm_rsh_args "-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR" \
       --mca btl ^ofi \
       ./mpi_wrapper.sh ./pingpong_bench 20 200 20 > "$OUT"
