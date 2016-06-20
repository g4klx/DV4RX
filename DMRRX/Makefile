CC      = gcc
CXX     = g++
CFLAGS  = -g -O3 -Wall -std=c++0x
LIBS    =
LDFLAGS = -g

OBJECTS = BPTC19696.o CRC.o DMRCSBK.o DMRDataHeader.o DMREmbeddedLC.o DMREMB.o DMRFullLC.o DMRLC.o DMRRX.o DMRShortLC.o DMRSlotType.o DV4mini.o Golay2087.o Hamming.o \
					Log.o QR1676.o RS129.o SerialController.o Utils.o

all:		DMRRX

DMRRX:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o DMRRX

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

clean:
		$(RM) DMRRX *.o *.d *.bak *~
 