CC=llvm-gcc
INC=../../include
FLAGS=-emit-llvm -g -c

.PHONY: klee

all: filterargs.o klee

klee: filterargs.o
	klee filterargs.o

filterargs.o: filterargs.c
	$(CC) -I $(INC) $(FLAGS) filterargs.c

clean:
	rm -rf klee-* filterargs.o
