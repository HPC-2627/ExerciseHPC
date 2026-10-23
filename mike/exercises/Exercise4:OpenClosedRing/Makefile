CC = mpicc
CFLAGS = -O3
TARGET = open_close_ring_mpi

all: $(TARGET)

$(TARGET): Exercise4OpenClosedRing.c
	$(CC) $(CFLAGS) Exercise4OpenClosedRing.c -o $(TARGET)

clean:
	rm -f $(TARGET) *.out *.err