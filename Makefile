.SUFFIXES:

CXX  = c++
CXX += -std=c++14
CXX += -Wall
CXX += -g
INC += -I uvw/src
LIB += -l uv

# this is macOS/Homebrew-specific
INC += -I/usr/local/opt/openssl/include
LIB += -L/usr/local/opt/openssl/lib
LIB += -lcrypto

_: main
#_: main broadcast receive udp udp-echo udp-send

%.o: %.cpp
	$(CXX) $(INC) -c -o $@ $<

main.o: cb.hpp main.cpp

main: main.o
	$(CXX) $(LIB) -o $@ $^

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
