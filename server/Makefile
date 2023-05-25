CXX ?= g++

.PHONY: main server

all: main

SCREEN_CAPTURE_LIBS = -lscreen_capture -lgdi32 -ld3d11 -ldxgi -ldwmapi
ASIO_LIBS = -lws2_32 -lmswsock
STATIC = -static -static-libgcc -static-libstdc++
INCLUDE_PATH = -I../shared/include/
LIBS_PATH = -L../shared/lib/

main:
	$(CXX) main.cpp $(INCLUDE_PATH) $(LIBS_PATH) $(STATIC) $(SCREEN_CAPTURE_LIBS) $(ASIO_LIBS) -o server.exe
	server.exe