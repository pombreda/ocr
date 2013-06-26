OCR_SRC_AFFINITY=affinity

noinst_LTLIBRARIES += libocr_affinity.la
libocr_la_LIBADD += libocr_affinity.la

libocr_affinity_la_SOURCES = \
$(OCR_SRC_AFFINITY)/ocr-affinity.c
libocr_affinity_la_CFLAGS = $(AM_CFLAGS)

noinst_LTLIBRARIES += libocr_affinity_regular.la
libocr_la_LIBADD += libocr_affinity_regular.la

libocr_affinity_regular_la_SOURCES = \
$(OCR_SRC_AFFINITY)/affinity-regular/affinity-regular.c
libocr_affinity_regular_la_CFLAGS = $(AM_CFLAGS)
