#pragma once

#include <mutex>

#include "libtcc.h"

constexpr const int OUTPUT_COUNT = 2;
// template <int N>
// struct Arr {
//  float data[N];
//};
// using OutputType = Arr<OUTPUT_COUNT>;
using OutputType = struct Stereo { float left, right; };
using PlayFunc = OutputType (*)(double);
using InitFunc = void (*)(void);

class TCC {
  TCCState *instance;
  PlayFunc play;
  char *p;
  void maybe_destroy();

 public:
  TCC();
  ~TCC();
  int compile(const char *code);
  OutputType operator()(double t);
};

class SwappingCompiler {
  TCC tcc[2];
  std::mutex lock;
  int active;
  bool shouldSwap;
  double t;

 public:
  SwappingCompiler();

  // call from the audio thread
  //
  bool checkForNewCode();

  // call from the audio thread
  //
  OutputType operator()();

  // call from the server thread
  //
  bool compileTry(const char *code);

  // call from the server thread
  //
  bool compile(const char *code);
};
