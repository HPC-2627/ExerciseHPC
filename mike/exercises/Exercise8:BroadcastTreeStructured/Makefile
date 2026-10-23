CC = mpicc
CFLAGS = -O3
TARGET = broadcast_mpi

all: $(TARGET)

$(TARGET): Exercise8BroadcastTreeStructured.c
	$(CC) $(CFLAGS) Exercise8BroadcastTreeStructured.c -o $(TARGET)

clean:
	rm -f $(TARGET) *.out *.err