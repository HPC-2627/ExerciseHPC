/*
========================ESERCIZIO 5: REDUCE E ALLREDUCE PERSONALIZZATE==========================================================
Esercitazione sulla comunicazione
1) Implementare una operazione "reduce" con MPI_Send e MPI_Recv, per raccogliere i dati locali in un processo root
   e calcolare le operazioni di riduzione (somma, minimo, massimo, media).
2) Implementare una operazione "allreduce" ridistribuendo il risultato finale dal root a tutti
   gli altri processi, in modo che tutti conoscano il risultato.

======================LOGICA:==========================================================================================================
- FASE 1: REDUCE (raccolta e aggregazione al root)
    * Ogni processo inizializza un valore locale (es. local_val = rank + 1).
    * I worker (rank != root) inviano local_val al root con MPI_Send.
    * Il root (rank == root) inizializza global_sum, global_min, global_max con il proprio local_val.
    * Il root scorre tutti i worker, riceve temp_val con MPI_Recv e aggiorna somma, minimo e massimo.
    * Il root calcola la media = global_sum / size.

- FASE 2: ALLREDUCE (redistribuzione del risultato a tutti i processi)
    * Il root scorre tutti i worker e invia global_sum a ciascuno con MPI_Send.
    * I worker ricevono global_sum dal root con MPI_Recv.
    * Ora tutti i processi hanno global_sum e ne stampano il valore.
*/

#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int rank, size;
    int root = 0; // Rank del processo che raccoglie e ridistribuisce

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

    int local_val = rank + 1;       // valore locale di questo processo
    int global_sum = 0;             // somma di tutti i valori
    int global_min = 0;             // minimo di tutti i valori
    int global_max = 0;             // massimo di tutti i valori
    double global_avg = 0.0;        // media di tutti i valori

    // ==========================================
    // FASE 1: REDUCE PERSONALIZZATA (worker -> root)
    // ==========================================
    if (rank != root) {
        /*
         * I worker inviano il proprio valore locale al processo root
         * Significato dei parametri di MPI_Send:
         * 1. &local_val   : puntatore al buffer con il dato
         * 2. 1            : numero di elementi
         * 3. MPI_INT      : tipo di dato
         * 4. root         : rank del destinatario
         * 5. 0            : tag del messaggio
         * 6. MPI_COMM_WORLD: communicator
         */
        MPI_Send(&local_val, 1, MPI_INT, root, 0, MPI_COMM_WORLD);
    } else {
        // Il root inizializza le metriche di riduzione con il proprio valore
        global_sum = local_val;
        global_min = local_val;
        global_max = local_val;

        int temp_val = 0;
        for (int i = 0; i < size; i++) {
            if (i != root) {
                /*
                 * Il root riceve il valore dal worker i
                 */
                MPI_Recv(&temp_val, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                // Aggrega le operazioni di riduzione
                global_sum += temp_val;
                if (temp_val < global_min) global_min = temp_val;
                if (temp_val > global_max) global_max = temp_val;
            }
        }
        
        // Calcola la media su tutti i dati raccolti
        global_avg = (double)global_sum / size;
        
        printf("--- REDUCE COMPLETATA AL ROOT (Processo %d) ---\n", root);
        printf("Somma globale = %d | Min = %d | Max = %d | Media = %.2f\n\n", global_sum, global_min, global_max, global_avg);
    }

    // ==========================================
    // FASE 2: ALLREDUCE PERSONALIZZATA (root -> tutti)
    // ==========================================
    if (rank == root) {
        /*
         * Il root invia il risultato ridotto a tutti i processi worker
         */
        for (int i = 0; i < size; i++) {
            if (i != root) {
                MPI_Send(&global_sum, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
        }
    } else {
        /*
         * I worker ricevono dal root il risultato finale aggregato
         */
        MPI_Recv(&global_sum, 1, MPI_INT, root, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Conferma che TUTTI i processi possiedono ora lo stesso risultato ridotto
    printf("Processo %d (ALLREDUCE confermata): global_sum = %d\n", rank, global_sum);

    MPI_Finalize();
    return 0;
}
