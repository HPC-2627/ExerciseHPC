CC = mpicc
CFLAGS = -O3
TARGET = ring_mpi

all: $(TARGET)

$(TARGET): Exercise1Communication.c
	$(CC) $(CFLAGS) Exercise1Communication.c -o $(TARGET)

clean:
	rm -f $(TARGET) *.out *.err