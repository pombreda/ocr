PROG=helloworld
CC=mpicc
CFLAGS=-O0 -g -Werror
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr

all-test: compile run

compile:
	$(CC) -g $(OCR_FLAGS) -I. $(PROG).c -o $(PROG)

run:
	./$(PROG)

clean:
	-rm -Rf *.o $(PROG)
