#Makefile

CC=gcc
CFLAGS=-Wall -I. -O2 -std=gnu99
DFLAGS=-Wall -I. -Og -g -std=gnu99
OFLAGS=-o
XFLAG=-pthread
EXE=awget
EXE2=ss
TAR=awget.tar

all:  awget.o ss.o
	$(CC) awget.o $(OFLAGS) $(EXE)
	$(CC) ss.o $(OFLAGS) $(EXE2) $(XFLAG)
awget.o: awget.c awget.h
	$(CC) $(CFLAGS) -c awget.c
ss: ss.o
	$(CC) ss.o $(OFLAGS) $(EXE2) $(XFLAG)
ss.o: ss.c awget.h
	$(CC) $(CFLAGS) -c ss.c $(XFLAG)
clean:
	rm -f *.o $(EXE) $(EXE2) $(TAR)
tar:
	tar cf $(TAR) awget.c awget.h ss.c Makefile README
debug:
	$(CC) awget.c $(DFLAGS) $(OFLAGS) $(EXE)
	$(CC) ss.c $(DFLAGS) $(OFLAGS) $(EXE2) $(XFLAG)