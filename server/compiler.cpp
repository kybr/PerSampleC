#include "compiler.h"

#include <math.h>

#include <mutex>

#include "libtcc.h"

extern unsigned CHANNELS_OUT;
extern unsigned CHANNELS_IN;
extern unsigned SAMPLE_RATE;
extern unsigned FRAME_COUNT;

//////////////////////////////////////////////////////////////////////////////
// Tiny C Compiler ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

std::string header = R"(
float log(float);
float pow(float, float);
)";

struct TCC {
  TCCState* instance{nullptr};
  ProcessFunc function{nullptr};
  size_t size{0};

  ~TCC() { maybe_destroy(); }
  void maybe_destroy() {
    if (instance) {
      tcc_delete(instance);

      // tcc_delete does not null the pointer
      instance = nullptr;
    }
  }

  bool compile(const std::string& code) {
    maybe_destroy();
    instance = tcc_new();
    if (instance == nullptr) {
      // XXX failed
      return false;
    }

    tcc_set_output_type(instance, TCC_OUTPUT_MEMORY);
    tcc_set_options(instance, "-Werror");
    // tcc_set_options(instance, "-Werror -g");

    char buffer[10];
    sprintf(buffer, "%u", SAMPLE_RATE);
    tcc_define_symbol(instance, "SAMPLE_RATE", buffer);

    if (0 != tcc_compile_string(instance, (header + code).c_str())) {
      // XXX failed to compile; check error
      return false;
    }

    tcc_add_symbol(instance, "log", (void*)logf);
    tcc_add_symbol(instance, "pow", (void*)powf);

    size = tcc_relocate(instance, nullptr);
    if (-1 == tcc_relocate(instance, TCC_RELOCATE_AUTO)) {
      return false;
    }

    //  if (p) delete p;
    //  int size = tcc_relocate(instance, nullptr);
    //  p = new char[size];
    //  if (-1 == tcc_relocate(instance, p)) return 3;

    function = (ProcessFunc)tcc_get_symbol(instance, "process");
    if (function == nullptr) {
      // XXX no "process" symbol
      return false;
    }

    using InitFunc = void (*)(void);
    InitFunc init = (InitFunc)tcc_get_symbol(instance, "init");
    if (init) init();

    return true;
  }
};

//////////////////////////////////////////////////////////////////////////////
// Private Implementation ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

struct SwappingCompilerImplementation {
  TCC tcc[2];
  std::mutex lock;
  int active{0};
  bool shouldSwap{false};

  bool compile(const std::string& code, bool tryLock) {
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

  ProcessFunc getFunctionMaybeSwap() {
    if (lock.try_lock()) {
      if (shouldSwap) active = 1 - active;
      shouldSwap = false;
      lock.unlock();
    }
    return tcc[active].function;
  }
};

//////////////////////////////////////////////////////////////////////////////
// Public Interface //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

SwappingCompiler::SwappingCompiler()
    : implementation(new SwappingCompilerImplementation) {}
SwappingCompiler::~SwappingCompiler() { delete implementation; }

bool SwappingCompiler::operator()(const std::string& code, bool tryLock) {
  return implementation->compile(code, tryLock);
}

ProcessFunc SwappingCompiler::operator()(void) {
  return implementation->getFunctionMaybeSwap();
}
