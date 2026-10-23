#include <stdio.h>
#include <string.h>
#include <mpi.h>

const int MAX_STRING = 100;

int main (int argc, char** argv) {
    char greeting[MAX_STRING];
    int comm_sz; /* Number of processes */
    int my_rank; /* My process rank */
    char ring[MAX_STRING]; /* Ring to use */

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (argc < 2) {
        fprintf(stderr, "Must specify which ring to use\n");
        MPI_Finalize();
        return 1;
    }
    strcpy(ring, argv[1]);
    if (strcmp(ring, "open") == 0) {
        /* Do something for the open ring */
        if (my_rank == 0) {
            sprintf(greeting, "Greetings from process %d of %d!", my_rank, comm_sz);
            MPI_Send(greeting, strlen(greeting)+1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        } else {
            MPI_Recv(greeting, MAX_STRING, MPI_CHAR, my_rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("%s\n", greeting);
            sprintf(greeting, "Greetings from process %d of %d!", my_rank, comm_sz);
            if (my_rank < comm_sz - 1) {
                MPI_Send(greeting, strlen(greeting)+1, MPI_CHAR, my_rank+1, 0, MPI_COMM_WORLD);
            } 
        }
    } else {
        /* Do something for the closed ring */
        if (my_rank == 0) {
            sprintf(greeting, "Greetings from process %d of %d!", my_rank, comm_sz);
            MPI_Send(greeting, strlen(greeting)+1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
            printf("%s\n", greeting);
            MPI_Recv(greeting, MAX_STRING, MPI_CHAR, comm_sz-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("%s\n", greeting);
        } else {
            MPI_Recv(greeting, MAX_STRING, MPI_CHAR, my_rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("%s\n", greeting);
            sprintf(greeting, "Greetings from process %d of %d!", my_rank, comm_sz);
            if (my_rank < comm_sz - 1) {
                MPI_Send(greeting, strlen(greeting)+1, MPI_CHAR, my_rank+1, 0, MPI_COMM_WORLD);
            } else {
                MPI_Send(greeting, strlen(greeting)+1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
        }
    }
    MPI_Finalize();
    return 0;
}