
noinst_LTLIBRARIES += libocr_utils.la
libocr_la_LIBADD += libocr_utils.la

libocr_utils_la_SOURCES = \
utils/ocr-utils.c \
utils/dictionary.c \
utils/iniparser.c \
utils/rangeTracker.c \
utils/thread-binding.c

libocr_utils_la_CFLAGS = $(AM_CFLAGS)