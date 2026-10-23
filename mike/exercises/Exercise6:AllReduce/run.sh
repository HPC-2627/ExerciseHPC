#!/bin/bash

# Nome del job (visibile in qstat)
#PBS -N mpi_allreduce_ex

# Richiede 1 nodo di calcolo con 8 core su quell'unico nodo
#PBS -l nodes=1:ppn=8

# Tempo massimo di esecuzione (walltime)
#PBS -l walltime=00:02:00

# Coda del cluster in cui eseguire il job
#PBS -q shortCPUQ

# Redirige l'output standard su un file di log fisso
#PBS -o mpi_allreduce_ex.log

# Redirige lo standard error su un file di errori fisso
#PBS -e mpi_allreduce_ex.err

# Torna nella cartella esatta da cui e' stato lanciato qsub
cd $PBS_O_WORKDIR

# Carica il modulo OpenMPI con il relativo compilatore GCC
module load OpenMPI/4.1.5-GCC-12.3.0

# Elimina eseguibili e file precedenti per ripartire da una compilazione pulita
make clean

# Compila il sorgente C usando il Makefile (mpicc)
make

# Lancia il programma MPI compilato su 8 processi in parallelo
mpirun -np 8 ./allreduce_mpi
