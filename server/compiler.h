#pragma once

#include <mutex>

#include "libtcc.h"

// signature each c sketch
//
using ProcessFunc = void (*)(double time, float* input, float* output);

struct TCC {
  TCCState* instance;
  ProcessFunc function;
  size_t size;
  void maybe_destroy();
  TCC();
  ~TCC();
  bool compile(const std::string& code);
};

class SwappingCompiler {
  TCC tcc[2];
  std::mutex lock;
  int active;
  bool shouldSwap;

 public:
  SwappingCompiler();

  // call from the server thread
  //
  bool operator()(const std::string& code, bool tryLock = false);

  // call from the audio thread
  //
  ProcessFunc operator()();
};
