#include <stdio.h>
#include <string.h>
#include <mpi.h>

enum { MAX_STRING = 100 };

int main(int argc, char** argv) {
    char greeting[MAX_STRING];
    int comm_sz; /* Number of processes */
    int my_rank; /* My process rank */
    char message[MAX_STRING];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    message[0] = '\0';

    if (my_rank == 0) {
        strncpy(message, argv[1], sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';
        MPI_Send(message, strlen(message) + 1, MPI_CHAR, 1, 0 , MPI_COMM_WORLD);
    } else {
        MPI_Recv(message, MAX_STRING, MPI_CHAR, my_rank - 1, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        if (my_rank < comm_sz - 1) {
            MPI_Send(message, strlen(message) + 1, MPI_CHAR, my_rank + 1, 0,
                     MPI_COMM_WORLD);
            printf("Process %d sent message: %s\n", my_rank, message);
        } else {
            printf("Process %d received message: %s\n", my_rank, message);
        }
    }

    MPI_Finalize();
    return 0;
}