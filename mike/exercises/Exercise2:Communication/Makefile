CC = mpicc
CFLAGS = -O3
TARGET = broadcast_mpi

all: $(TARGET)

$(TARGET): Exercise2Communication.c
	$(CC) $(CFLAGS) Exercise2Communication.c -o $(TARGET)

clean:
	rm -f $(TARGET) *.out *.err