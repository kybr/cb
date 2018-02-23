# http://en.cppreference.com/w/c
# cc -pthread -fno-omit-frame-pointer -Wall -g -o run async-cb.c -luv
CC = cc -std=c11 
CFLAGS = -Wall -g
CFLAGS = -pthread -fno-omit-frame-pointer
LIBS = -I /usr/local/include -L /usr/local/lib -l uv

_:
	$(CC) $(CFLAGS) $(LIBS) -o udp-dhcp udp-dhcp.c

clean:
	rm udp-dhcp
