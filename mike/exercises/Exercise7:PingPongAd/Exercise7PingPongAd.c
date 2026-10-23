/*
========================ESERCIZIO 7: PARTITA DI PING PONG=======================================================================
Esercitazione sulla comunicazione
Implementare una partita di "ping pong" con send e receive di MPI.

======================REGOLE DEL GIOCO:=======================================================================================
1. PARTECIPANTI: il processo 0 (Giocatore 1) e il processo 1 (Giocatore 2).
2. DATI SCAMBIATI: un array `ball_data[2]` dove:
   - ball_data[0] = contatore dei rimbalzi (numero di colpi riusciti)
   - ball_data[1] = posizione della pallina sul tavolo (stato visivo)
3. ITERAZIONI E CONDIZIONE DI USCITA:
   - La partita continua avanti e indietro finche' `bounce_count` raggiunge MAX_BOUNCES (10).
   - Raggiunto MAX_BOUNCES il ciclo termina e la partita finisce.
4. RAPPRESENTAZIONE VISIVA:
   - Viene stampata su stdout una rappresentazione ASCII del tavolo e della pallina a ogni colpo.
*/

#include <mpi.h>
#include <stdio.h>

#define MAX_BOUNCES 10   // numero di rimbalzi dopo cui la partita termina
#define TABLE_SIZE 10    // lunghezza del tavolo (in caratteri) nella stampa ASCII

// Funzione che disegna in ASCII il tavolo da ping pong con la pallina
void print_ascii_table(int player_rank, int ball_pos, int bounce_count) {
    printf("Rimbalzo #%2d | ", bounce_count);
    if (player_rank == 0) {
        printf("Giocatore 1 (Rank 0) [ ");
    } else {
        printf("Giocatore 2 (Rank 1) [ ");
    }

    for (int i = 0; i < TABLE_SIZE; i++) {
        if (i == ball_pos) {
            printf("o"); // Posizione della pallina
        } else {
            printf(".");
        }
    }
    printf(" ] -> Colpo verso il Giocatore %d\n", (player_rank == 0) ? 2 : 1);
}

int main(int argc, char** argv) {
    int rank, size;

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

    // L'avversario: il rank 0 gioca contro il rank 1 e viceversa
    int partner_rank = (rank == 0) ? 1 : 0;
    
    /* 
     * Struttura dell'array ball_data:
     * ball_data[0]: bounce_count (rimbalzi)
     * ball_data[1]: ball_position (0 per il lato di P1, TABLE_SIZE - 1 per il lato di P2)
     */
    int ball_data[2] = {0, 0};

    if (rank == 0 || rank == 1) {
        while (ball_data[0] < MAX_BOUNCES) {
            // Tocca a chi ha rank uguale alla parita' del contatore: 0, 1, 0, 1, ...
            if (rank == ball_data[0] % 2) {
                // TURNO DI CHI COLPISCE: incrementa i rimbalzi e imposta la posizione della pallina
                ball_data[0]++;
                ball_data[1] = (rank == 0) ? 0 : (TABLE_SIZE - 1);

                // Stampa la rappresentazione visiva prima di battere/rispondere
                print_ascii_table(rank, ball_data[1], ball_data[0]);

                /*
                 * Significato dei parametri di MPI_Send:
                 * 1. ball_data      : puntatore al buffer con i dati
                 * 2. 2              : numero di interi inviati
                 * 3. MPI_INT        : tipo di dato
                 * 4. partner_rank   : rank del destinatario (0 o 1)
                 * 5. 0              : tag del messaggio
                 * 6. MPI_COMM_WORLD : communicator
                 */
                MPI_Send(ball_data, 2, MPI_INT, partner_rank, 0, MPI_COMM_WORLD);

            } else {
                // TURNO DI CHI RICEVE: attende la pallina in arrivo
                /*
                 * Significato dei parametri di MPI_Recv:
                 * 1. ball_data      : puntatore al buffer dove salvare i dati
                 * 2. 2              : numero massimo di elementi da ricevere
                 * 3. MPI_INT        : tipo di dato
                 * 4. partner_rank   : rank del mittente atteso
                 * 5. 0              : tag del messaggio
                 * 6. MPI_COMM_WORLD : communicator
                 * 7. MPI_STATUS_IGNORE: puntatore allo status (ignorato)
                 */
                MPI_Recv(ball_data, 2, MPI_INT, partner_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }

        if (rank == 0) {
            printf("\n=======================================================\n");
            printf("FINE PARTITA! Scambio di ping pong completato con %d rimbalzi.\n", MAX_BOUNCES);
            printf("=======================================================\n");
        }
    } else {
        // Gli altri processi (rank >= 2) non giocano
        printf("Processo %d: Spettatore (inattivo)\n", rank);
    }

    MPI_Finalize();
    return 0;
}
