/*
========================ESERCIZIO 5: REDUCE PERSONALIZZATA======================================================================
Esercitazione sulla comunicazione
Implementare una operazione "reduce" usando chiamate send e receive.
Raccogliere i valori locali di tutti i processi in un processo root e calcolare la somma dei dati raccolti.

======================LOGICA:==========================================================================================================
- Si definisce un processo root (es. il processo 0) che raccoglie e somma i dati.
- Ogni processo calcola o inizializza un valore locale (es. local_val = rank + 1).
- Se rank != root:
    * Invia local_val al processo root con MPI_Send.
- Se rank == root:
    * Inizializza global_sum con il proprio local_val.
    * Scorre tutti gli altri processi (da 0 a size - 1, saltando root).
    * Riceve temp_val dal processo i con MPI_Recv e lo somma a global_sum.
    * Stampa la somma totale finale.
*/

#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int rank, size;
    int root = 0; // Rank del processo root che esegue la riduzione

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

    // Ogni processo genera un valore locale in base al proprio rank (es. rank + 1)
    int local_val = rank + 1;
    printf("Processo %d: valore locale = %d\n", rank, local_val);

    if (rank != root) {
        /*
         * I processi worker inviano il proprio valore locale al processo root
         * Significato dei parametri di MPI_Send:
         * 1. &local_val   : puntatore al buffer con il dato da inviare
         * 2. 1            : numero di elementi
         * 3. MPI_INT      : tipo di dato
         * 4. root         : rank del destinatario
         * 5. 0            : tag del messaggio
         * 6. MPI_COMM_WORLD: communicator
         */
        MPI_Send(&local_val, 1, MPI_INT, root, 0, MPI_COMM_WORLD);
    } else {
        // Il root inizializza la somma globale con il proprio valore locale
        int global_sum = local_val;
        int temp_val = 0;

        // Riceve i valori locali di tutti gli altri processi e li somma
        for (int i = 0; i < size; i++) {
            if (i != root) {
                /*
                 * Significato dei parametri di MPI_Recv:
                 * 1. &temp_val    : puntatore al buffer dove salvare il dato
                 * 2. 1            : numero massimo di elementi da ricevere
                 * 3. MPI_INT      : tipo di dato
                 * 4. i            : rank del mittente atteso
                 * 5. 0            : tag del messaggio
                 * 6. MPI_COMM_WORLD: communicator
                 * 7. MPI_STATUS_IGNORE: puntatore allo status (ignorato)
                 */
                MPI_Recv(&temp_val, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                printf("Processo %d (root): ricevuto il valore %d dal processo %d\n", root, temp_val, i);
                global_sum += temp_val;
            }
        }

        printf("Processo %d (root): somma totale (risultato della reduce) = %d\n", root, global_sum);
    }

    MPI_Finalize();
    return 0;
}
