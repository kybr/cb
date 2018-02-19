.SUFFIXES:

CXX  = c++
CXX += -std=c++14
CXX += -Wall
CXX += -g
INC += -I uvw/src
LIB += -l uv

_: broadcast receive udp udp-echo udp-send

%.o: %.cpp
	$(CXX) $(INC) -c -o $@ $<

receive: receive.o
	$(CXX) $(LIB) -o $@ $^

broadcast: broadcast.o
	$(CXX) $(LIB) -o $@ $^

udp: udp.o
	$(CXX) $(LIB) -o $@ $^

udp-echo: udp-echo.o
	$(CXX) $(LIB) -o $@ $^

udp-send: udp-send.o
	$(CXX) $(LIB) -o $@ $^

clean:
	rm broadcast receive udp-echo udp-send udp *.o
