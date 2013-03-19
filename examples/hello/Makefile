PROG=helloworld
CC=mpicc
CFLAGS=-O0 -g -Werror
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr

all-test: compile run
dddf: compile-dddf run-dddf

compile:
	$(CC) -g $(OCR_FLAGS) -I. $(PROG).c -o $(PROG)

run:
	./$(PROG)

compile-dddf:
	$(CC) -g $(OCR_FLAGS) -I. $(PROG)_dddf.c -o $(PROG)_dddf

run-dddf:
	mpirun -n 2 ./$(PROG)_dddf

clean:
	-rm -Rf *.o $(PROG) $(PROG)_dddf
