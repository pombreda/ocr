OCR_SRC_COMMUNICATION=communication

noinst_LTLIBRARIES += libocr_communication.la
libocr_la_LIBADD += libocr_communication.la

libocr_communication_la_SOURCES = \
$(OCR_SRC_COMMUNICATION)/ocr-communication.c
libocr_communication_la_CFLAGS = $(AM_CFLAGS)

#
# OCR with MPI communication implementation
#
noinst_LTLIBRARIES += libocr_communication_hc_mpi.la
libocr_la_LIBADD += libocr_communication_hc_mpi.la

libocr_communication_hc_mpi_la_SOURCES = \
$(OCR_SRC_COMMUNICATION)/hc-mpi/ocr-hc-mpi.c
libocr_communication_hc_mpi_la_CFLAGS = $(AM_CFLAGS)
