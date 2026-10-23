CC = mpicc
CFLAGS = -O3
TARGET = allreduce_mpi

all: $(TARGET)

$(TARGET): Exercise6AllReduce.c
	$(CC) $(CFLAGS) Exercise6AllReduce.c -o $(TARGET)

clean:
	rm -f $(TARGET) *.out *.err