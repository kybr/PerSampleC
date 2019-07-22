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

per-sample-c: per-sample-c.cpp compiler.cpp RtAudio.cpp
	$(CXX) $(COMPILER) $^ -o $@ $(LINKER)

per-sample-c-submit: per-sample-c-submit.cpp
	$(CXX) $^ -o $@ -llo

clean:
	rm per-sample-c per-sample-c-submit test-compiler 
	rm *.o

test-compiler: test-compler.cpp compiler.cpp
	$(CXX) $(COMPILER) $^ -o $@ $(LINKER)
