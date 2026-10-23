#include <stdio.h>
#include <string.h>
#include <mpi.h>

const int MAX_STRING = 100;

int main (void) {
    char greeting[MAX_STRING];
    int comm_sz; /* Number of processes */
    int my_rank; /* My process rank */

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (comm_sz < 2) {
        fprintf(stderr, "Must use at least 2 processes for this example\n");
        MPI_Finalize();
        return 1;
    }

   while(1 == 1) { if (my_rank != 0) {
        sprintf(greeting, "Ping");
        MPI_Send(greeting, strlen(greeting)+1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        MPI_Recv(greeting, MAX_STRING, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Ball received");
    } else {
        MPI_Recv(greeting, MAX_STRING, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Ball received");
        sprintf(greeting, "Pong");
        MPI_Send(greeting, strlen(greeting)+1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
    }}
    MPI_Finalize();
    return 0;
}