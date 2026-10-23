CC     = mpicc
CFLAGS = -O3

all: pingpong_bench

pingpong_bench: ExercisePingPong.c
	$(CC) $(CFLAGS) ExercisePingPong.c -o $@ -lm

clean:
	rm -f pingpong_bench