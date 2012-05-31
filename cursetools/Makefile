.PHONY: all clean

KERNELDIR = ../linux
ASMDIR = $(KERNELDIR)/arch/x86

CC = gcc
INC = -I$(ASMDIR)/include -I$(KERNELDIR)/include
LINK = -L.
CFLAGS = -Wall -O2
DEFS = -D__i386__ -D_GNU_SOURCE

all: libcurse.a curse drain fio-torture uncache waste-memory

libcurse.a: libcurse.o
	ar r libcurse.a libcurse.o

libcurse.o: libcurse.c libcurse.h
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

curse: curse.c libcurse.h libcurse.a
	$(CC) $(CFLAGS) $(INC) $(LINK) -o $@ $< -lcurse

fio-torture: fio-torture.c
	$(CC) $(CFLAGS) -o $@ $<

drain: drain.c
	$(CC) $(CFLAGS) -o $@ $<

uncache: uncache.c
	$(CC) $(CFLAGS) -o $@ $<

waste-memory: waste-memory.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f libcurse.o libcurse.a curse drain fio-torture uncache waste-memory
