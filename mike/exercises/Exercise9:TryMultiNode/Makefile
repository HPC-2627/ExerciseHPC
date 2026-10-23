CC = mpicc
CFLAGS = -O3
TARGET = reduce_mpi

all: $(TARGET)

$(TARGET): Exercise9TryMultiNode.c
	$(CC) $(CFLAGS) Exercise9TryMultiNode.c -o $(TARGET)

clean:
	rm -f $(TARGET) *.out *.err