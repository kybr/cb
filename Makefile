.SUFFIXES:

CXX  = c++
CXX += -std=c++14
CXX += -Wall
CXX += -g
INC += -I uvw/src
LIB += -l uv

_: udp udp-echo udp-send

%.o: %.cpp
	$(CXX) $(INC) $(LIB) -c $^

udp: udp.o
	$(CXX) $(INC) $(LIB) -o $@ $^

udp-echo: udp-echo.o
	$(CXX) $(INC) $(LIB) -o $@ $^

udp-send: udp-send.o
	$(CXX) $(INC) $(LIB) -o $@ $^

clean:
	rm udp-echo udp-send udp *.o
