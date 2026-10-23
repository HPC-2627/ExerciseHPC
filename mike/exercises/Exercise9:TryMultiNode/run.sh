#!/bin/bash

# Nome del job (visibile in qstat)
#PBS -N mpi_reduce_multinode

# Richiede 2 chunk, ciascuno con 4 CPU e 2 GB di RAM
#PBS -l select=2:ncpus=4:mem=2gb

# =========================================================================
# STRATEGIE DI POSIZIONAMENTO PBS (decommentare UNA SOLA opzione alla volta)
# Attualmente e' attiva scatter
# =========================================================================

# 1. PACK: raggruppa i chunk sui nodi usando il minor numero di nodi possibile
#    (i processi vengono messi vicini, sullo stesso nodo, se possibile)
# #PBS -l place=pack

# 2. SCATTER: distribuisce i chunk sul maggior numero possibile di nodi diversi
#    (massimizza la separazione fisica dei processi su nodi distinti)
#PBS -l place=scatter

# 3. PACK + EXCL: raggruppa i chunk sullo stesso nodo con accesso esclusivo
#    (nessun altro utente puo' condividere le risorse dei nodi assegnati)
# #PBS -l place=pack:excl

# 4. SCATTER + EXCL: distribuisce i chunk su nodi diversi con allocazione esclusiva
#    (massima separazione fisica e hardware interamente dedicato al job)
# #PBS -l place=scatter:excl

# =========================================================================

# Tempo massimo di esecuzione (walltime)
#PBS -l walltime=00:05:00

# Coda di esecuzione sul cluster
#PBS -q shortCPUQ

# Redirige output standard ed errori su file fissi
#PBS -o mpi_reduce_multinode.log
#PBS -e mpi_reduce_multinode.err

# Torna nella cartella da cui e' stato sottomesso il job
cd $PBS_O_WORKDIR

# Carica OpenMPI e il compilatore GCC compatibile
module load OpenMPI/4.1.5-GCC-12.3.0

# Elimina le compilazioni precedenti
make clean

# Compila il sorgente C tramite Makefile
make

# Esegue il programma MPI su 8 processi totali, usando l'hostfile di PBS per mappare correttamente gli slot
mpiexec -n 8 --hostfile $PBS_NODEFILE ./reduce_mpi
