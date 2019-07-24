#pragma once

#include <mutex>

#include "libtcc.h"

constexpr const int OUTPUT_COUNT = 2;
template <int N>
struct Arr {
  float data[N];
};
using OutputType = Arr<OUTPUT_COUNT>;
// using OutputType = struct Stereo { float left, right; };
using PlayFunc = OutputType (*)(double);
using ProcessFunc = void (*)(double, float*, float*);
using InitFunc = void (*)(void);

struct TCC {
  TCCState* instance;
  ProcessFunc function;
  // PlayFunc function;
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
  bool compile(const std::string& code, bool tryLock = false);

  // call from the audio thread
  //
  ProcessFunc function();
  // PlayFunc function();
};
