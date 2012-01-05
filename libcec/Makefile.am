lib_LTLIBRARIES = libcec.la

CEC_BACKEND_SRC =

if LINUX_REALTEK_SOC
CEC_BACKEND_SRC += linux_realtek_soc.c
endif

EXTRA_DIST = $(CEC_BACKEND_SRC)

libcec_la_CFLAGS = $(VISIBILITY_CFLAGS) $(AM_CFLAGS)
libcec_la_LDFLAGS = $(LTLDFLAGS)
libcec_la_SOURCES = libceci.h libcec.c decoder.h decoder.c $(CEC_BACKEND_SRC)

hdrdir = $(includedir)/libcec
hdr_HEADERS = libcec.h decoder.h

