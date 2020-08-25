########################################################################
#
#              --- CAEN SpA - Computing Division ---
#
#   CAENVMElib Software Project
#
#   Created  :  March 2004      (Rel. 1.0)
#
#   Auth: S. Coluccini
#
########################################################################

EXE = fadcread

CC	=	g++

ROOTFLAGS = $(shell root-config --cflags)
ROOTLIBS  = $(shell root-config --libs) 

COPTS	=	-fPIC -DLINUX -Wall

FLAGS	=	-Wall -O2 -s
CFLAGS = -D_FILE_OFFSET_BITS=64

DEPLIBS	=       -l CAENComm -l curses

LIBS	=	$(ROOTLIBS)

INCLUDEDIR =	-I.

OBJS	=	$(EXE).o

INCLUDES =	CAENVMElib.h CAENVMEtypes.h Struct.h Date.h

#########################################################################

all : $(EXE)

clean:
	rm -f $(OBJS) $(EXE)

$(EXE) :$(OBJS)
	rm -f $(EXE)
	$(CC) $(FLAGS) -o $(EXE) $(OBJS) $(DEPLIBS) $(LIBS)

$(OBJS) : $(INCLUDES) Makefile

%.o : %.cc
	$(CC) $(COPTS) $(INCLUDEDIR) $(ROOTFLAGS) -c -o $@ $<
