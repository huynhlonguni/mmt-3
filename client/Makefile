CXX ?= g++

.PHONY: main client

all: main

RAYLIB_LIBS = -lraylib -lopengl32 -lgdi32 -lwinmm
NO_CONSOLE = -mwindows
STATIC = -static -static-libgcc -static-libstdc++
ASIO_LIBS = -lws2_32 -lmswsock
INCLUDE_PATH = -I../shared/include/
LIBS_PATH = -L../shared/lib/

main:
	$(CXX) client.cpp main.cpp $(NO_CONSOLE) $(INCLUDE_PATH) $(LIBS_PATH) $(STATIC) $(RAYLIB_LIBS) $(ASIO_LIBS) -o client.exe
	client.exe

