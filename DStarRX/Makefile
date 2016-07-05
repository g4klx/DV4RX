CC      = gcc
CXX     = g++
CFLAGS  = -g -O3 -Wall -std=c++0x
LIBS    =
LDFLAGS = -g

OBJECTS = AMBEFEC.o CRC.o DStarRX.o DV4mini.o Golay24128.o Log.o SerialController.o Utils.o

all:		DStarRX

DStarRX:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o DStarRX

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

clean:
		$(RM) DStarRX *.o *.d *.bak *~
