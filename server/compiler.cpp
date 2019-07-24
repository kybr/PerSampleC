#include "compiler.h"

#include "libtcc.h"

TCC::TCC() : instance(nullptr), function(nullptr) {}

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

  // tcc_define_symbol(instance, "SAMPLE_RATE", "44100");
  // tcc_define_symbol(instance, "FRAME_COUNT", "512");
  // tcc_define_symbol(instance, "CHANNELS_IN", "2");
  // tcc_define_symbol(instance, "CHANNELS_OUT", "2");

  if (0 != tcc_compile_string(instance, code.c_str())) {
    // XXX failed to compile; check error
    return false;
  }

  size = tcc_relocate(instance, nullptr);
  if (-1 == tcc_relocate(instance, TCC_RELOCATE_AUTO)) {
    return false;
  }

  //  if (p) delete p;
  //  int size = tcc_relocate(instance, nullptr);
  //  p = new char[size];
  //  if (-1 == tcc_relocate(instance, p)) return 3;

  // function = (PlayFunc)tcc_get_symbol(instance, "play");
  function = (ProcessFunc)tcc_get_symbol(instance, "process");
  if (function == nullptr) {
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

ProcessFunc SwappingCompiler::function(void) {
  // PlayFunc SwappingCompiler::function(void) {
  if (lock.try_lock()) {
    if (shouldSwap) active = 1 - active;
    shouldSwap = false;
    lock.unlock();
  }
  return tcc[active].function;
}