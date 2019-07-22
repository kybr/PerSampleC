#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#include "libtcc.h"

using PlayFunc = float (*)(double);
using InitFunc = void (*)(void);

struct TCC {
  TCCState* instance{nullptr};
  PlayFunc function{nullptr};

  void maybe_destroy() {
    if (instance) {
      tcc_delete(instance);

      // tcc_delete does not null the pointer
      instance = nullptr;
    }
  }

  ~TCC() { maybe_destroy(); }

  int compile(const char* code) {
    maybe_destroy();
    instance = tcc_new();
    if (instance == nullptr) return 1;

    tcc_set_output_type(instance, TCC_OUTPUT_MEMORY);

    if (0 != tcc_compile_string(instance, code)) return 2;
    if (-1 == tcc_relocate(instance, TCC_RELOCATE_AUTO)) return 3;

    function = (PlayFunc)tcc_get_symbol(instance, "play");
    if (function == nullptr) return 4;

    InitFunc init = (InitFunc)tcc_get_symbol(instance, "init");
    if (init) init();

    return 0;
  }

  float operator()(double t) {
    if (function)
      return function(t);
    else
      return 0;
  }
};

struct SwappingCompiler {
  TCC tcc[2];
  std::mutex lock;
  int active = 0;
  bool shouldSwap = false;

  // call from the audio thread
  //
  bool checkForNewCode() {
    if (lock.try_lock()) {
      if (shouldSwap) active = 1 - active;
      shouldSwap = false;
      lock.unlock();
      return true;
    }
    return false;
  }

  // call from the audio thread
  //
  float operator()(double t) { return tcc[active](0); }

  // call from the server thread
  //
  bool compileTry(const char* code) {
    bool returnValue = false;
    if (lock.try_lock()) {
      if (tcc[1 - active].compile(code)) {
        // compile failed
      } else {
        shouldSwap = true;
        returnValue = true;
      }
      lock.unlock();
    } else {
      // someone else has the lock.. try again
    }
    return returnValue;
  }

  // call from the server thread
  //
  bool compile(const char* code) {
    lock.lock();
    bool returnValue = false;
    if (tcc[1 - active].compile(code)) {
      // compile failed
    } else {
      shouldSwap = true;
      returnValue = true;
    }
    lock.unlock();
    return returnValue;
  }
};

int main(int argc, char* argv[]) {
  SwappingCompiler compiler;
  bool done = false;

  std::thread audio([&]() {
    while (!done) {
      if (compiler.checkForNewCode()) {
        // do stuff
      }

      for (int i = 0; i < 1024; i++) compiler(0);
    }
  });

  const int N = 100000;

  std::chrono::high_resolution_clock::time_point then{
      std::chrono::high_resolution_clock::now()};

  int missedLock = 0;

  int n = 0;
  while (n < N) {
    if (compiler.compileTry("float play(double t) { return 100 * t; }")) {
      // if the compile succeeded
      n++;
    } else {
      // if the compile FAILED
      missedLock++;
    }
  }

  printf("missed: %d\n", missedLock);
  printf("trials: %d\n", N);
  printf("ellapsed: %lf\n",
         std::chrono::duration<double>(
             std::chrono::high_resolution_clock::now() - then)
             .count());

  done = true;
  audio.join();

  return 0;
}
