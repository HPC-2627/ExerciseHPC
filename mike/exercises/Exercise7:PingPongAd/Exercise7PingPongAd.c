/*
========================EXERCISE 7: PING PONG GAME==============================================================================
Communication hands-on
Implement a "ping pong" game using MPI send and receive.

======================RULES OF THE GAME:======================================================================================
1. PARTICIPANTS: Process 0 (Player 1) and Process 1 (Player 2).
2. DATA EXCHANGED: An array `ball_data[2]` where:
   - ball_data[0] = bounce count (tracking number of successful hits)
   - ball_data[1] = ball position on the table (visual state identifier)
3. ITERATIONS & EXIT CONDITION:
   - The game continues back and forth until `bounce_count` reaches MAX_BOUNCES (10).
   - Once MAX_BOUNCES is reached, the loop terminates and the game ends.
4. VISUAL REPRESENTATION:
   - ASCII visual representation printed to stdout showing the table and ball position at each hit.
*/

#include <mpi.h>
#include <stdio.h>

#define MAX_BOUNCES 10
#define TABLE_SIZE 10

// Function to render ASCII visual table of the ping pong table
void print_ascii_table(int player_rank, int ball_pos, int bounce_count) {
    printf("Bounce #%2d | ", bounce_count);
    if (player_rank == 0) {
        printf("Player 1 (Rank 0) [ ");
    } else {
        printf("Player 2 (Rank 1) [ ");
    }

    for (int i = 0; i < TABLE_SIZE; i++) {
        if (i == ball_pos) {
            printf("o"); // Ball position
        } else {
            printf(".");
        }
    }
    printf(" ] -> Hit to Player %d\n", (player_rank == 0) ? 2 : 1);
}

int main(int argc, char** argv) {
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // CHECK number of processes to avoid execution errors
    if (size < 2) {
        if (rank == 0) {
            printf("Error: This program requires at least 2 processes.\n");
        }
        MPI_Finalize();
        return 0;
    }

    int partner_rank = (rank == 0) ? 1 : 0;
    
    /* 
     * ball_data array structure:
     * ball_data[0]: bounce_count
     * ball_data[1]: ball_position (0 for P1 side, TABLE_SIZE - 1 for P2 side)
     */
    int ball_data[2] = {0, 0};

    if (rank == 0 || rank == 1) {
        while (ball_data[0] < MAX_BOUNCES) {
            if (rank == ball_data[0] % 2) {
                // SENDER TURN: increment score/bounce and set ball position
                ball_data[0]++;
                ball_data[1] = (rank == 0) ? 0 : (TABLE_SIZE - 1);

                // Print visual representation before serving/hitting back
                print_ascii_table(rank, ball_data[1], ball_data[0]);

                /*
                 * MPI_Send parameters breakdown:
                 * 1. ball_data      : Pointer to payload buffer
                 * 2. 2              : Number of integers sent
                 * 3. MPI_INT        : Data type
                 * 4. partner_rank   : Destination rank (0 or 1)
                 * 5. 0              : Message tag
                 * 6. MPI_COMM_WORLD : Communicator
                 */
                MPI_Send(ball_data, 2, MPI_INT, partner_rank, 0, MPI_COMM_WORLD);

            } else {
                // RECEIVER TURN: wait for incoming ball
                /*
                 * MPI_Recv parameters breakdown:
                 * 1. ball_data      : Pointer to buffer where data is stored
                 * 2. 2              : Max elements to receive
                 * 3. MPI_INT        : Data type
                 * 4. partner_rank   : Expected sender rank
                 * 5. 0              : Message tag
                 * 6. MPI_COMM_WORLD : Communicator
                 * 7. MPI_STATUS_IGNORE: Status struct ignore pointer
                 */
                MPI_Recv(ball_data, 2, MPI_INT, partner_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }

        if (rank == 0) {
            printf("\n=======================================================\n");
            printf("GAME OVER! Ping-Pong rally completed with %d bounces.\n", MAX_BOUNCES);
            printf("=======================================================\n");
        }
    } else {
        printf("Process %d: Spectator (Idle)\n", rank);
    }

    MPI_Finalize();
    return 0;
}