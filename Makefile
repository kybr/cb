.SUFFIXES:

CXX  = c++
CXX += -std=c++14
CXX += -Wall
CXX += -g
INC += -I uvw/src
LIB += -l uv

_: udp udp-echo udp-send

%.o: %.cpp
	$(CXX) $(INC) -c -o $@ $<

udp: udp.o
	$(CXX) $(LIB) -o $@ $^

udp-echo: udp-echo.o
	$(CXX) $(LIB) -o $@ $^

udp-send: udp-send.o
	$(CXX) $(LIB) -o $@ $^

clean:
	rm udp-echo udp-send udp *.o
