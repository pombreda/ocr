PROG=smithwaterman
PROG_D3F=smithwaterman_dddf
CFLAGS=-O3 -g
OCR_FLAGS=-L${OCR_INSTALL}/lib -I${OCR_INSTALL}/include -locr
OCR_INPUT=/home/cs20/ocr/examples/smith-waterman
	
all-test: compile run
dddf: compile_d3f run_d3f

compile:
	mpicc $(OCR_FLAGS) -I. $(PROG).c -o $(PROG)

compile_d3f:
	mpicc $(OCR_FLAGS) -I. $(PROG_D3F).c -o $(PROG_D3F)

SIZE=tiny
#SIZE=medium
#SIZE=large
#SIZE=huge
#SIZE=very-huge

ifeq ($(SIZE),tiny)
TILE_WIDTH=4
TILE_HEIGHT=4
INNER_TILE_WIDTH=2
INNER_TILE_HEIGHT=2
EXPECTED_RESULT=12
endif
ifeq ($(SIZE),medium)
TILE_WIDTH=232
TILE_HEIGHT=240
INNER_TILE_WIDTH=29
INNER_TILE_HEIGHT=30
EXPECTED_RESULT=3640
endif
ifeq ($(SIZE),large)
TILE_WIDTH=2320
TILE_HEIGHT=2400
INNER_TILE_WIDTH=232
INNER_TILE_HEIGHT=240
EXPECTED_RESULT=36472
endif
ifeq ($(SIZE),huge)
TILE_WIDTH=11600
TILE_HEIGHT=12000
INNER_TILE_WIDTH=725
INNER_TILE_HEIGHT=750
EXPECTED_RESULT=364792
endif

run:
	./$(PROG) ${OCR_INPUT}/string1-${SIZE}.txt ${OCR_INPUT}/string2-${SIZE}.txt ${TILE_WIDTH} ${TILE_HEIGHT} 

run_d3f:
	mpirun -n 2 -bynode ./$(PROG_D3F) ${OCR_INPUT}/string1-${SIZE}.txt ${OCR_INPUT}/string2-${SIZE}.txt ${INNER_TILE_WIDTH} ${INNER_TILE_HEIGHT} ${TILE_WIDTH} ${TILE_HEIGHT}

clean:
	-rm -Rf *.o $(PROG) $(PROG_D3F)
