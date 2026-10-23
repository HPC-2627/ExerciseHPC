#include <stdio.h>
#include <string.h>
#include <mpi.h>

const int MAX_STRING = 100;

int main(void) {
    char message[MAX_STRING];
    int comm_sz; /* Number of processes */
    int my_rank; /* My process rank */
    int parent;
    int left_child;
    int right_child;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (comm_sz < 2) {
        if (my_rank == 0) {
            fprintf(stderr, "Must use at least 2 processes for this example\n");
        }
        MPI_Finalize();
        return 1;
    }

    /*
     * Processes form a binary tree:
     *   parent      = (rank - 1) / 2
     *   left child  = 2 * rank + 1
     *   right child = 2 * rank + 2
     */
    parent = (my_rank - 1) / 2;
    left_child = 2 * my_rank + 1;
    right_child = 2 * my_rank + 2;

    if (my_rank == 0) {
        snprintf(message, MAX_STRING,
                 "Broadcast from process %d of %d", my_rank, comm_sz);
    } else {
        MPI_Recv(message, MAX_STRING, MPI_CHAR, parent, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    printf("Process %d received: %s\n", my_rank, message);

    if (left_child < comm_sz) {
        MPI_Send(message, (int)strlen(message) + 1, MPI_CHAR, left_child, 0,
                 MPI_COMM_WORLD);
    }
    if (right_child < comm_sz) {
        MPI_Send(message, (int)strlen(message) + 1, MPI_CHAR, right_child, 0,
                 MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}