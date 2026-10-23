/*
========================ESERCIZIO 4: COMUNICAZIONE AD ANELLO====================================================================
Esercitazione sulla comunicazione
Implementare una topologia ad "anello" usando chiamate send e receive.
Ogni processo passa un messaggio/token al vicino di destra, formando un anello logico.

======================LOGICA:==========================================================================================================
- FASE 1: Avvio e propagazione
    * Il processo 0 avvia l'anello inviando il token iniziale al processo 1.
    * Tutti gli altri processi (rank > 0) ricevono dal vicino di sinistra (prev), elaborano/aggiornano il token e lo inviano al vicino di destra (next).
- FASE 2: Completamento
    * Il processo 0 riceve il token finale dall'ultimo processo (size - 1) e stampa il risultato.
*/

#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int rank, size;
    int token = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);   // identificativo di questo processo
    MPI_Comm_size(MPI_COMM_WORLD, &size);   // numero totale di processi

    // CONTROLLO sul numero di processi per evitare errori di esecuzione
    if (size < 2) {
        if (rank == 0) {
            printf("Errore: questo programma richiede almeno 2 processi.\n");
        }
        MPI_Finalize();
        return 0;
    }

    // Individua i vicini di sinistra e di destra nella topologia ad anello
    // (l'operatore % fa si' che l'ultimo processo abbia come successivo il processo 0)
    int next = (rank + 1) % size;
    int prev = (rank - 1 + size) % size;

    // FASE 1: avvio dell'anello (rank 0) e propagazione del token (rank da 1 a size - 1)
    if (rank == 0) {
        token = 100; // Valore iniziale del token
        printf("Processo 0: avvio l'anello con token = %d\n", token);

        /* 
         * Il processo 0 invia il token iniziale al processo 1
         */
        MPI_Send(&token, 1, MPI_INT, next, 0, MPI_COMM_WORLD);
        printf("Processo 0: inviato token = %d al processo %d\n", token, next);
    } else {
        /*
         * I processi da 1 a size-1 attendono di ricevere dal vicino di sinistra (prev),
         * aggiornano il token e lo inoltrano al vicino di destra (next).
         */
        MPI_Recv(&token, 1, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Processo %d: ricevuto token = %d dal processo %d\n", rank, token, prev);

        token += rank; // Modifica il contenuto per tenere traccia del passaggio lungo l'anello

        MPI_Send(&token, 1, MPI_INT, next, 0, MPI_COMM_WORLD);
        printf("Processo %d: inviato token aggiornato = %d al processo %d\n", rank, token, next);
    }

    // FASE 2: chiusura dell'anello
    if (rank == 0) {
        /* 
         * Il processo 0 attende di ricevere il token finale dall'ultimo processo (prev = size - 1)
         */
        MPI_Recv(&token, 1, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Processo 0: ricevuto il token dell'anello completato = %d dal processo %d\n", token, prev);
    }

    MPI_Finalize();
    return 0;
}
