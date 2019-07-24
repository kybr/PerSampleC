.SUFFIXES:

CXX =
CXX += g++
CXX += -std=c++11

COMPILER =
COMPILER += -Wall
COMPILER += -g
COMPILER += -D__LINUX_ALSA__ 

LINKER =
LINKER += -ltcc
LINKER += -ldl
LINKER += -lpthread
LINKER += -llo
LINKER += -lasound

all: per-sample-c-server per-sample-c-client

%.o: %.cpp
	$(CXX) $^ -c $(COMPILER)

per-sample-c-server: per-sample-c-server.o compiler.o RtAudio.o
	$(CXX) $^ -o $@ $(LINKER)

per-sample-c-client: per-sample-c-client.cpp
	$(CXX) $^ -o $@ -llo

clean:
	rm -f per-sample-c-server per-sample-c-client test-compiler 
	rm -f RtAudio.o compiler.o per-sample-c.o per-sample-c-server.o

test-compiler: test-compler.cpp compiler.cpp
	$(CXX) $(COMPILER) $^ -o $@ $(LINKER)
