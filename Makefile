.SUFFIXES:

CXX =
CXX += g++
CXX += -std=c++11

LINKER =
LINKER += -ltcc
LINKER += -ldl
LINKER += -lpthread

per-sample-c: per-sample-c.cpp
	$(CXX) $< -o $@ $(LINKER)
