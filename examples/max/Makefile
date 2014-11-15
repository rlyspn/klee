CC=llvm-gcc
INC=../../include
FLAGS=-emit-llvm -g -c

.PHONY: klee

all: max.o klee

klee: max.o
	klee max.o

max.o: max.c
	$(CC) -I $(INC) $(FLAGS) max.c

clean:
	rm -rf klee-* max.o
