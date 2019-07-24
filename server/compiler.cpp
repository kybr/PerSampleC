#include "compiler.h"

#include "libtcc.h"

TCC::TCC() : instance(nullptr), play(nullptr) {}

void TCC::maybe_destroy() {
  if (instance) {
    tcc_delete(instance);

    // tcc_delete does not null the pointer
    instance = nullptr;
  }
}

TCC::~TCC() { maybe_destroy(); }

bool TCC::compile(const std::string &code) {
  maybe_destroy();
  instance = tcc_new();
  if (instance == nullptr) {
    // XXX failed
    return false;
  }

  tcc_set_output_type(instance, TCC_OUTPUT_MEMORY);
  tcc_set_options(instance, "-Werror");
  // tcc_set_options(instance, "-Werror -g");

  if (0 != tcc_compile_string(instance, code.c_str())) {
    // XXX failed to compile; check error
    return false;
  }

  size = tcc_relocate(instance, nullptr);
  if (-1 == tcc_relocate(instance, TCC_RELOCATE_AUTO)) {
    // XXX failed to relocate
    return false;
  }

  //  if (p) delete p;
  //  int size = tcc_relocate(instance, nullptr);
  //  p = new char[size];
  //  if (-1 == tcc_relocate(instance, p)) return 3;

  play = (PlayFunc)tcc_get_symbol(instance, "play");
  if (play == nullptr) {
    // XXX no "play" symbol
    return false;
  }

  InitFunc init = (InitFunc)tcc_get_symbol(instance, "init");
  if (init) init();

  return true;
}

SwappingCompiler::SwappingCompiler() : active(0), shouldSwap(false) {}

bool SwappingCompiler::compile(const std::string &code, bool tryLock) {
  if (tryLock && !lock.try_lock())
    return false;
  else
    lock.lock();  // blocking call

  bool compileSucceeded = false;

  if (tcc[1 - active].compile(code)) {
    shouldSwap = true;
    compileSucceeded = true;
  }

  lock.unlock();
  return compileSucceeded;
}

PlayFunc SwappingCompiler::function(void) {
  if (lock.try_lock()) {
    if (shouldSwap) active = 1 - active;
    shouldSwap = false;
    lock.unlock();
  }
  return tcc[active].play;
}
