CC = mpicc
CFLAGS = -O3
TARGET = pingpong_game_mpi

all: $(TARGET)

$(TARGET): Exercise7PingPongAd.c
	$(CC) $(CFLAGS) Exercise7PingPongAd.c -o $(TARGET)

clean:
	rm -f $(TARGET) *.out *.err