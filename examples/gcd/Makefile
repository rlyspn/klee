CC=llvm-gcc
INC=../../klee/include
FLAGS=-emit-llvm -g -c

.PHONY: klee

all: gcd.o klee

klee: gcd.o
	klee gcd.o

gcd.o: gcd.c
	$(CC) -I $(INC) $(FLAGS) gcd.c

clean:
	rm -rf klee-* gcd.o
