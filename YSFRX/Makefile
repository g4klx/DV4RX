CC      = gcc
CXX     = g++
CFLAGS  = -g -O3 -Wall -std=c++0x
LIBS    =
LDFLAGS = -g

OBJECTS = AMBEFEC.o CRC.o DV4mini.o Golay24128.o Hamming.o Log.o SerialController.o UDPSocket.o Utils.o YSFConvolution.o YSFFICH.o YSFPayload.o YSFRX.o

all:		YSFRX

YSFRX:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o YSFRX

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

clean:
		$(RM) YSFRX *.o *.d *.bak *~
 