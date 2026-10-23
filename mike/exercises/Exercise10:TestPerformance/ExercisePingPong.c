/*
 * ============================================================================
 *  BENCHMARK DI COMUNICAZIONE PUNTO-PUNTO (MPI_Send / MPI_Recv)
 * ============================================================================
 *
 *  OBIETTIVO
 *  Misurare il tempo necessario per inviare messaggi di dimensione crescente
 *  (1, 2, 4, ..., 2^MAX_EXP byte) da un processo all'altro e, da questo,
 *  stimare la LATENZA (messaggi piccoli) e la BANDA (messaggi grandi).
 *
 *  COME FUNZIONA
 *  Due processi (rank 0 e rank 1) eseguono un "ping-pong":
 *      rank 0 --- messaggio di n byte ---> rank 1
 *      rank 0 <--- messaggio di n byte --- rank 1
 *  Per ogni dimensione il ping-pong viene ripetuto molte volte e ogni
 *  ripetizione viene cronometrata separatamente con MPI_Wtime, solo sul
 *  rank 0 (cosi' non serve sincronizzare gli orologi di nodi diversi).
 *
 *  FORMULE
 *      tempo di sola andata   t = (tempo di andata e ritorno) / 2
 *      banda                  B = n / t        [MB/s, con 1 MB = 10^6 byte]
 *
 *  USO
 *      mpirun -np 2 ./pingpong_bench [max_exp=20] [reps=200] [warmup=20]
 *        max_exp : la dimensione massima e' 2^max_exp byte (20 -> 1 MiB)
 *        reps    : numero di ripetizioni misurate per ogni dimensione
 *        warmup  : ripetizioni iniziali scartate (riscaldamento)
 *
 *  OUTPUT (stdout, formato CSV; le righe che iniziano con '#' sono metadati)
 *      n_bytes,reps,t_mean_s,t_std_s,t_min_s,t_median_s,rate_mean_MBps,rate_peak_MBps
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Quanti byte corrispondono a "1 MB" nel calcolo della banda.
 * - 1.0e6 -> MB decimale; per i MiB usare 1048576.0.
 * - (La convenzione usata va dichiarata nel report.) */
#define MB_BYTES 1.0e6

/* Funzione di confronto per qsort: serve a ordinare i tempi misurati
 * (necessario per calcolare minimo e mediana). */
static int cmp_double(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);   /* -1 se x<y, 0 se uguali, +1 se x>y */
}

int main(int argc, char **argv) {
    int rank, size;

    /* ---- Inizializzazione dell'ambiente MPI ---- */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);   /* il mio identificativo (0 o 1) */
    MPI_Comm_size(MPI_COMM_WORLD, &size);   /* numero totale di processi */

    /* ---- Controllo: il benchmark richiede ESATTAMENTE 2 processi ---- */
    if (size != 2) {
        if (rank == 0) fprintf(stderr, "Errore: eseguire con esattamente 2 processi (-np 2).\n");
        MPI_Finalize();
        return 1;
    }

    /* ---- Lettura dei parametri da riga di comando (con valori di default) ---- */
    int max_exp = (argc > 1) ? atoi(argv[1]) : 20;     /* 2^20 byte = 1 MiB */
    int reps    = (argc > 2) ? atoi(argv[2]) : 200;    /* ripetizioni misurate */
    int warmup  = (argc > 3) ? atoi(argv[3]) : 20;     /* ripetizioni di riscaldamento */

    /* Validazione dei parametri (max_exp <= 30 evita buffer > 1 GiB;
     * reps >= 2 serve perche' la deviazione standard divide per reps-1). */
    if (max_exp < 0 || max_exp > 30 || reps < 2 || warmup < 0) {
        if (rank == 0) fprintf(stderr, "Argomenti non validi.\n");
        MPI_Finalize();
        return 1;
    }

    /* ---- Allocazione dei buffer ---- */
    size_t max_size = (size_t)1 << max_exp;                 /* byte del messaggio piu' grande */
    char *buf = malloc(max_size);                           /* buffer usato per tutti i messaggi */
    double *times = malloc((size_t)reps * sizeof(double));  /* tempi di sola andata delle ripetizioni */
    if (!buf || !times) {
        fprintf(stderr, "Rank %d: malloc fallita\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* "Tocca" tutte le pagine del buffer PRIMA di misurare: la prima scrittura
     * in memoria causa page fault che altererebbero i tempi. */
    memset(buf, rank + 1, max_size);

    /* ---- Metadati: dove girano i due rank? ----
     * Serve a verificare che il posizionamento PBS (pack / scatter) sia
     * quello voluto: con "pack" entrambi i rank sono sullo stesso nodo,
     * con "scatter" su nodi diversi. */
    char name[MPI_MAX_PROCESSOR_NAME] = {0}, name1[MPI_MAX_PROCESSOR_NAME] = {0};
    int len;
    MPI_Get_processor_name(name, &len);      /* nome dell'host su cui sto girando */
    if (rank == 1) {
        /* il rank 1 invia il proprio hostname al rank 0 (tag 99 per non
         * confonderlo con i messaggi del benchmark, che usano il tag 0) */
        MPI_Send(name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, 99, MPI_COMM_WORLD);
    } else {
        /* il rank 0 riceve l'hostname del rank 1 e stampa l'intestazione */
        MPI_Recv(name1, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 1, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("# rank0_host=%s rank1_host=%s same_node=%s\n", name, name1,
               strcmp(name, name1) == 0 ? "yes" : "no");
        /* MPI_Wtick = risoluzione del timer: i tempi misurati devono essere
         * molto piu' grandi di questo valore per essere affidabili */
        printf("# wtick=%.3e max_exp=%d reps=%d warmup=%d\n", MPI_Wtick(), max_exp, reps, warmup);
        /* intestazioni delle colonne del CSV */
        printf("n_bytes,reps,t_mean_s,t_std_s,t_min_s,t_median_s,rate_mean_MBps,rate_peak_MBps\n");
    }

    /* ======================= CICLO SULLE DIMENSIONI ======================= */
    for (int e = 0; e <= max_exp; e++) {
        int n = 1 << e;     /* dimensione corrente: 1, 2, 4, ..., 2^max_exp byte */

        /* Per i messaggi grandi (> 64 KiB) si usano meno ripetizioni (reps/5, ma
         * almeno 20): ogni ripetizione dura di piu' e il rumore relativo e' minore. */
        int r = (n > 65536) ? (reps / 5 > 20 ? reps / 5 : 20) : reps;
        if (r > reps) r = reps;     /* non superare mai le ripetizioni richieste */

        /* Barriera: entrambi i processi partono insieme da questa dimensione */
        MPI_Barrier(MPI_COMM_WORLD);

        /* ---- Ripetizioni: le prime 'warmup' vengono scartate, poi 'r' misurate ---- */
        for (int i = 0; i < warmup + r; i++) {
            if (rank == 0) {
                /* PING: il rank 0 invia e poi attende la risposta. */
                double t0 = MPI_Wtime();                                      /* inizio */
                MPI_Send(buf, n, MPI_BYTE, 1, 0, MPI_COMM_WORLD);             /* invio al rank 1 */
                MPI_Recv(buf, n, MPI_BYTE, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); /* attesa del ritorno */
                double t1 = MPI_Wtime();                                      /* fine */
                /* Salva il tempo solo dopo il riscaldamento.
                 * Si divide per 2 perche' t1-t0 e' andata e ritorno. */
                if (i >= warmup) times[i - warmup] = (t1 - t0) / 2.0;
            } else {
                /* PONG: il rank 1 riceve e rispedisce lo stesso messaggio. */
                MPI_Recv(buf, n, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(buf, n, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
            }
        }

        /* ---- Statistiche e stampa (solo il rank 0, che possiede i tempi) ---- */
        if (rank == 0) {
            /* Media aritmetica dei tempi di sola andata */
            double sum = 0.0, sq = 0.0;
            for (int i = 0; i < r; i++) sum += times[i];
            double mean = sum / r;

            /* Deviazione standard campionaria (divisione per r-1) */
            for (int i = 0; i < r; i++) sq += (times[i] - mean) * (times[i] - mean);
            double sd = sqrt(sq / (r - 1));

            /* Ordina i tempi per ottenere minimo e mediana
             * (la mediana e' robusta rispetto a eventuali outlier) */
            qsort(times, r, sizeof(double), cmp_double);
            double tmin = times[0];     /* caso migliore: tempo minimo */
            double tmed = (r % 2) ? times[r / 2]                              /* r dispari: elemento centrale */
                                  : 0.5 * (times[r / 2 - 1] + times[r / 2]);  /* r pari: media dei due centrali */

            /* Riga CSV: banda "media" = n / tempo medio,
             *           banda "di picco" = n / tempo minimo */
            printf("%d,%d,%.9e,%.9e,%.9e,%.9e,%.6f,%.6f\n", n, r, mean, sd, tmin, tmed,
                   n / MB_BYTES / mean, n / MB_BYTES / tmin);
            fflush(stdout);     /* svuota subito il buffer, cosi' nulla va perso se il job termina */
        }
    }

    /* ---- Pulizia e chiusura ---- */
    free(buf);
    free(times);
    MPI_Finalize();
    return 0;
}
