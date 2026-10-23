/*
========================ESERCIZIO=================================================================================================
Esercitazione sulla comunicazione
Progettare un broadcast usando solo chiamate send e receive.
Cosa ci si aspetta: un processo "p" vuole condividere il contenuto di una variabile "a" con tutti gli altri processi.


======================LOGICA:==========================================================================================================
- Si definisce un processo radice "p" (es. il processo 0) che possiede la variabile "a".
- Se sei il processo "p" (rank == p):
    1. Inizializzi la variabile "a".
    2. Scorri tutti gli altri processi (da 0 a size - 1, saltando p stesso).
    3. Invii il contenuto di "a" a ciascuno con MPI_Send.
- Altrimenti (rank != p):
    1. Ricevi il contenuto di "a" direttamente dal processo "p" con MPI_Recv.
    2. Stampi il valore ricevuto per confermare la ricezione.
*/
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int rank, size;
    int a = 0;
    int root = 0; // Rank del processo "p" che esegue il broadcast

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
        // Il processo "p" inizializza la variabile "a"
        a = 42; // Valore di esempio da trasmettere
        printf("Processo %d (root): trasmetto la variabile a = %d a tutti gli altri processi\n", root, a);

        // Scorre tutti i processi e invia il dato a tutti tranne che a se stesso
        for (int i = 0; i < size; i++) {
            if (i != root) {
                /* 
                 * Significato dei parametri di MPI_Send:
                 * 1. &a           : puntatore al buffer che contiene i dati da inviare
                 * 2. 1            : numero di elementi da inviare
                 * 3. MPI_INT      : tipo degli elementi nel buffer
                 * 4. i            : rank del processo destinatario
                 * 5. 0            : tag del messaggio (serve a distinguere tipi diversi di messaggi)
                 * 6. MPI_COMM_WORLD: communicator (il gruppo di processi coinvolti)
                 */
                MPI_Send(&a, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                printf("Processo %d: inviata la variabile a = %d al processo %d\n", root, a, i);
            }
        }
    } else {
        // Tutti gli altri processi ricevono il dato direttamente dal processo "p"
        
        /* 
         * Significato dei parametri di MPI_Recv:
         * 1. &a           : puntatore al buffer dove verranno salvati i dati ricevuti
         * 2. 1            : numero massimo di elementi da ricevere
         * 3. MPI_INT      : tipo degli elementi nel buffer
         * 4. root         : rank del processo mittente (da chi ci aspettiamo il messaggio)
         * 5. 0            : tag del messaggio (deve coincidere con quello del mittente)
         * 6. MPI_COMM_WORLD: communicator
         * 7. MPI_STATUS_IGNORE: puntatore allo status con le info sul messaggio (qui ignorato)
         */
        MPI_Recv(&a, 1, MPI_INT, root, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Processo %d: ricevuta la variabile a = %d dal processo %d\n", rank, a, root);
    }

    MPI_Finalize();
    return 0;
}
