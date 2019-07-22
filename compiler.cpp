#include "compiler.h"

#include "libtcc.h"

TCC::TCC() : instance(nullptr), play(nullptr), p(nullptr) {}

void TCC::maybe_destroy() {
  if (instance) {
    tcc_delete(instance);

    // tcc_delete does not null the pointer
    instance = nullptr;
  }
}

TCC::~TCC() { maybe_destroy(); }

int TCC::compile(const char *code) {
  maybe_destroy();
  instance = tcc_new();
  if (instance == nullptr) return 1;

  tcc_set_output_type(instance, TCC_OUTPUT_MEMORY);
  tcc_set_options(instance, "-Werror");
  // tcc_set_options(instance, "-Werror -g");

  if (0 != tcc_compile_string(instance, code)) return 2;

  if (p) delete p;
  int size = tcc_relocate(instance, nullptr);
  p = new char[size];
  if (-1 == tcc_relocate(instance, p)) return 3;

  play = (PlayFunc)tcc_get_symbol(instance, "play");
  if (play == nullptr) return 4;

  InitFunc init = (InitFunc)tcc_get_symbol(instance, "init");
  if (init) init();

  return 0;
}

OutputType TCC::operator()(double t) {
  if (play == nullptr) return {0};
  return play(t);
}

SwappingCompiler::SwappingCompiler() : active(0), shouldSwap(false), t(0) {}

bool SwappingCompiler::checkForNewCode() {
  bool hasSwappedCode = false;
  if (lock.try_lock()) {
    if (shouldSwap) {
      active = 1 - active;
      hasSwappedCode = true;
    }
    shouldSwap = false;
    lock.unlock();
  }
  return hasSwappedCode;
}

OutputType SwappingCompiler::operator()() {
  t += 1.0 / 44100;
  return tcc[active](t);
}

bool SwappingCompiler::compileTry(const char *code) {
  bool compileSucceeded = false;
  if (lock.try_lock()) {
    if (tcc[1 - active].compile(code)) {
      // compile failed
    } else {
      shouldSwap = true;
      compileSucceeded = true;
    }
    lock.unlock();
  } else {
    // XXX set error to "could not get the lock"
  }
  return compileSucceeded;
}

bool SwappingCompiler::compile(const char *code) {
  lock.lock();
  bool compileSucceeded = false;
  if (tcc[1 - active].compile(code)) {
    // compile failed
  } else {
    shouldSwap = true;
    compileSucceeded = true;
  }
  lock.unlock();
  return compileSucceeded;
}
