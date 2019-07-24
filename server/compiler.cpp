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

std::string errorMessage;

void tcc_error_handler(void* tcc, const char* msg) {
  errorMessage = msg;
  // TODO:
  // Given error string like this:
  //   <string>:5: error: declaration expected
  //
  // - Remove file name prefix which is "<string>"
  // - Correct line number which is off by the header size
}

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
      errorMessage = "Null Instance";
      return false;
    }

    tcc_set_error_func(instance, nullptr, tcc_error_handler);
    tcc_set_options(instance, "-Werror");  // treat warnings as errors
    tcc_set_output_type(instance, TCC_OUTPUT_MEMORY);

    // Define Pre-Processor Symbols
    //
    char buffer[10];
    sprintf(buffer, "%u", SAMPLE_RATE);
    tcc_define_symbol(instance, "SAMPLE_RATE", buffer);

    // Do the compile step
    //
    if (0 != tcc_compile_string(instance, (header + code).c_str())) {
      // XXX failed to compile; check error
      return false;
    }

    tcc_add_symbol(instance, "log", (void*)logf);
    tcc_add_symbol(instance, "pow", (void*)powf);

    size = tcc_relocate(instance, nullptr);
    if (-1 == tcc_relocate(instance, TCC_RELOCATE_AUTO)) {
      errorMessage = "Relocate Failed";
      return false;
    }

    //  if (p) delete p;
    //  int size = tcc_relocate(instance, nullptr);
    //  p = new char[size];
    //  if (-1 == tcc_relocate(instance, p)) return 3;

    function = (ProcessFunc)tcc_get_symbol(instance, "process");
    if (function == nullptr) {
      errorMessage = "No 'process' Symbol";
      // XXX no "process" symbol
      return false;
    }

    using InitFunc = void (*)(void);
    InitFunc init = (InitFunc)tcc_get_symbol(instance, "init");
    if (init) init();

    errorMessage = "";

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

  std::string compile(const std::string& code, bool tryLock) {
    if (tryLock && !lock.try_lock())
      return std::string("No Lock");
    else
      lock.lock();  // blocking call

    std::string message;

    if (tcc[1 - active].compile(code)) {
      shouldSwap = true;
    } else {
      message = errorMessage;
    }

    lock.unlock();
    return message;
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

std::string SwappingCompiler::operator()(const std::string& code,
                                         bool tryLock) {
  return implementation->compile(code, tryLock);
}

ProcessFunc SwappingCompiler::operator()(void) {
  return implementation->getFunctionMaybeSwap();
}
