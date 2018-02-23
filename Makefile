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
LIB += -lcrypto # for testing only

_: main.exe

%.o: %.cpp
	$(CXX) $(INC) -c -o $@ $<

main.o: cb.hpp main.cpp

main.exe: main.o
	$(CXX) $(LIB) -o $@ $^

clean:
	rm main *.o
