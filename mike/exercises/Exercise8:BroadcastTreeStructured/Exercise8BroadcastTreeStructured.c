/*
Implementare un "broadcast" usando send e receive
*//*
========================ESERCIZIO 8: BROADCAST PERSONALIZZATO====================================================================
Esercitazione sulla comunicazione
Implementare una operazione di "broadcast" usando chiamate MPI punto-punto send e receive.
Il processo "p" (root) condivide il contenuto di una variabile "a" con tutti gli altri processi.

======================LOGICA:==========================================================================================================
- Si definisce un processo radice "root" (es. il processo 0) che possiede la variabile "a".
- Se rank == root:
    1. Inizializza la variabile "a".
    2. Scorre tutti gli altri processi (da 0 a size - 1, saltando root stesso).
    3. Invia il contenuto di "a" a ciascuno con MPI_Send.
- Altrimenti (rank != root):
    1. Riceve il contenuto di "a" direttamente dal processo "root" con MPI_Recv.
    2. Stampa il valore ricevuto per confermare la ricezione del broadcast.
*/

#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int rank, size;
    int a = 0;
    int root = 0; // Rank del processo root che esegue il broadcast

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

    if (rank == root) {
        // Il root inizializza la variabile "a"
        a = 100; // Valore da trasmettere
        printf("Processo %d (root): trasmetto la variabile a = %d a tutti i processi\n", root, a);

        // Scorre tutti i processi e invia il dato a tutti tranne che al root
        for (int i = 0; i < size; i++) {
            if (i != root) {
                /* 
                 * Significato dei parametri di MPI_Send:
                 * 1. &a           : puntatore al buffer con i dati
                 * 2. 1            : numero di elementi
                 * 3. MPI_INT      : tipo di dato
                 * 4. i            : rank del destinatario
                 * 5. 0            : tag del messaggio
                 * 6. MPI_COMM_WORLD: communicator
                 */
                MPI_Send(&a, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                printf("Processo %d (root): inviata la variabile a = %d al processo %d\n", root, a, i);
            }
        }
    } else {
        // I processi worker ricevono il dato del broadcast direttamente dal root
        /* 
         * Significato dei parametri di MPI_Recv:
         * 1. &a           : puntatore al buffer dove salvare i dati
         * 2. 1            : numero massimo di elementi da ricevere
         * 3. MPI_INT      : tipo di dato
         * 4. root         : rank del mittente atteso
         * 5. 0            : tag del messaggio
         * 6. MPI_COMM_WORLD: communicator
         * 7. MPI_STATUS_IGNORE: puntatore allo status (ignorato)
         */
        MPI_Recv(&a, 1, MPI_INT, root, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Processo %d: ricevuta la variabile di broadcast a = %d dal processo root %d\n", rank, a, root);
    }

    MPI_Finalize();
    return 0;
}
