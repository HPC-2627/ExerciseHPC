/*
========================ESERCIZIO=================================================================================================
Esercitazione sulla comunicazione
Implementare un anello usando send e receive.
Scrivere un programma che prende un dato dal processo zero e lo invia a tutti gli altri processi passandolo lungo un anello.
Cioe' il processo i riceve il dato e lo invia al processo i+1, fino a raggiungere l'ultimo processo.


======================LOGICA:==========================================================================================================
- Se sei il processo 0 (rank == 0): inizi tu, inizializzi il dato e lo invii al processo 1 (MPI_Send verso 1). Fine.
- altrimenti:
    1. Ognuno riceve il dato dal proprio predecessore (rank - 1).
    2. Controlli: se non sei l'ultimo (rank < size - 1), lo inoltri subito al successivo (rank + 1).
    3. Se sei l'ultimo, non lo inoltri a nessuno: ricevi il dato, stampi che la catena e' terminata e hai finito.
*/
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int rank, size;
    int data = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);   // identificativo di questo processo
    MPI_Comm_size(MPI_COMM_WORLD, &size);   // numero totale di processi

    // CONTROLLO sul numero di processi, per evitare errori banali
    if (size < 2) {
        if (rank == 0) {
            printf("Errore: questo programma richiede almeno 2 processi.\n");
        }
        MPI_Finalize();
        return 0;
    }

    if (rank == 0) {
        // Il processo 0 inizializza il dato e lo invia al processo 1
        data = 10 ; // Valore di esempio
        printf("Processo 0: invio il dato %d al processo 1\n", data);
        
        /* 
         * Significato dei parametri di MPI_Send:
         * 1. &data        : puntatore al buffer che contiene i dati da inviare
         * 2. 1            : numero di elementi da inviare
         * 3. MPI_INT      : tipo degli elementi nel buffer
         * 4. 1            : rank del processo destinatario
         * 5. 0            : tag del messaggio (serve a distinguere tipi diversi di messaggi)
         * 6. MPI_COMM_WORLD: communicator (il gruppo di processi coinvolti)
         */
        MPI_Send(&data, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        
    } else {
        // I processi intermedi e l'ultimo ricevono dal processo precedente
        
        /* 
         * Significato dei parametri di MPI_Recv:
         * 1. &data        : puntatore al buffer dove verranno salvati i dati ricevuti
         * 2. 1            : numero massimo di elementi da ricevere
         * 3. MPI_INT      : tipo degli elementi nel buffer
         * 4. rank - 1     : rank del processo mittente (da chi ci aspettiamo il messaggio)
         * 5. 0            : tag del messaggio (deve coincidere con quello del mittente)
         * 6. MPI_COMM_WORLD: communicator
         * 7. MPI_STATUS_IGNORE: puntatore allo status con le info sul messaggio (qui ignorato)
         */
        MPI_Recv(&data, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Processo %d: ricevuto il dato %d dal processo %d\n", rank, data, rank - 1);

        // Se non siamo l'ultimo processo, inoltriamo il dato al successivo
        if (rank < size - 1) {
            /* 
             * Parametri di MPI_Send (come sopra):
             * 1. &data        : buffer da inviare
             * 2. 1            : numero di elementi
             * 3. MPI_INT      : tipo di dato
             * 4. rank + 1     : rank del destinatario (prossimo processo dell'anello)
             * 5. 0            : tag del messaggio
             * 6. MPI_COMM_WORLD: communicator
             */
            MPI_Send(&data, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("Processo %d: inoltrato il dato %d al processo %d\n", rank, data, rank + 1);
        } else {
            printf("Processo %d (ultimo): fine della catena, dato finale ricevuto = %d\n", rank, data);
        }
    }

    MPI_Finalize();
    return 0;
}
