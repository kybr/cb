_:
	c++ -std=c++14 -I uvw/src -l uv -o udp      udp.cpp
	c++ -std=c++14 -I uvw/src -l uv -o udp-echo udp-echo.cpp
	c++ -std=c++14 -I uvw/src -l uv -o udp-send udp-send.cpp

clean:
	rm udp-echo udp-send udp
