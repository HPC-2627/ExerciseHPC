CC = mpicc
CFLAGS = -O3
TARGET = pingpong_mpi

all: $(TARGET)

$(TARGET): Exercise3PingPong.c
	$(CC) $(CFLAGS) Exercise3PingPong.c -o $(TARGET)

clean:
	rm -f $(TARGET) *.out *.err