#include "compiler.h"

#include <dlfcn.h>
#include <libtcc.h>
#include <math.h>

#include <mutex>
#include <regex>
#include <string>

#include "../HostInterface.h"

using std::string;

extern unsigned SAMPLE_RATE;

//////////////////////////////////////////////////////////////////////////////
// Tiny C Compiler ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void error_handler(void *str, const char *msg) {
  static_cast<string *>(str)->append(msg);
}

typedef void (*TCCErrorFunc)(void *opaque, const char *msg);

typedef TCCState *(*type_tcc_new)(void);
typedef void (*type_tcc_delete)(TCCState *);
typedef void (*type_tcc_set_error_func)(TCCState *, void *, TCCErrorFunc);
typedef void (*type_tcc_set_options)(TCCState *, const char *);
typedef int (*type_tcc_set_output_type)(TCCState *, int);
typedef void (*type_tcc_define_symbol)(TCCState *, const char *, const char *);
typedef int (*type_tcc_compile_string)(TCCState *, const char *);
typedef int (*type_tcc_add_symbol)(TCCState *, const char *, const void *);
typedef int (*type_tcc_add_library)(TCCState *, const char *);
typedef int (*type_tcc_relocate)(TCCState *, void *);
typedef void *(*type_tcc_get_symbol)(TCCState *, const char *);

struct TCC {
  type_tcc_new _new;
  type_tcc_delete _delete;
  type_tcc_set_error_func _set_error_func;
  type_tcc_set_options _set_options;
  type_tcc_set_output_type _set_output_type;
  type_tcc_define_symbol _define_symbol;
  type_tcc_compile_string _compile_string;
  type_tcc_add_symbol _add_symbol;
  type_tcc_add_library _add_library;
  type_tcc_relocate _relocate;
  type_tcc_get_symbol _get_symbol;

  TCCState *instance{nullptr};
  ProcessFunc function{nullptr};
  size_t size{0};

  ~TCC() { maybe_destroy(); }
  void maybe_destroy() {
    if (instance) {
      _delete(instance);

      // tcc_delete does not null the pointer
      instance = nullptr;
    }
  }

  bool compile(const string &code, string *err) {
    maybe_destroy();
    instance = _new();
    if (instance == nullptr) {
      err->append("Null Instance");
      return false;
    }

    _set_error_func(instance, (void *)err, error_handler);

    // treat warnings as errors
    //_set_options(instance, "-Wall -Werror -nostdinc -nostdlib");
    _set_options(instance, "-Wall -Werror");

    _set_output_type(instance, TCC_OUTPUT_MEMORY);

    // Define Pre-Processor Symbols
    //
    char buffer[10];
    sprintf(buffer, "%u", SAMPLE_RATE);
    _define_symbol(instance, "SAMPLE_RATE", buffer);

    // Do the compile step
    //
    if (0 != _compile_string(instance, code.c_str())) {
      // TODO: move all this to the client. the server should never fail to
      // compile now because the client does this check

      // Given error string like this:
      //   <string>:5: error: declaration expected
      //   $FILE ':' $LINE ':' $MESSAGE
      //
      std::regex re("^([^:]+):(\\d+):(.*)$");
      std::smatch m;
      if (std::regex_match(*err, m, re)) {
        // int line = stoi(m[2]);

        // XXX
        //
        // given the line number, extract the actual line

        err->clear();
        err->append("line:");
        err->append(std::to_string(stoi(m[2])));
        err->append(m[3].str());

      } else {
        err->append(" | failed to match error");
      }

      return false;
    }
    // add symbols
    _add_symbol(instance, "host_reset", (void *)host_reset);
    _add_symbol(instance, "host_float", (void *)host_float);
    _add_symbol(instance, "host_int", (void *)host_int);
    _add_symbol(instance, "host_char", (void *)host_char);

    // tcc_add_symbol(instance, "log", (void*)logf);
    // tcc_add_symbol(instance, "log", (void*)logf);
    //_add_library(instance, "System");
    int result = _add_library(instance, "m");
    printf("got %d\n", result);

    // XXX broken on linux
    // size = tcc_relocate(instance, nullptr);

    if (-1 == _relocate(instance, TCC_RELOCATE_AUTO)) {
      err->append("Relocate Failed");
      return false;
    }

    function = (ProcessFunc)_get_symbol(instance, "process");
    if (function == nullptr) {
      err->append("No 'process' Symbol");
      return false;
    }

    using InitFunc = void (*)(void);
    InitFunc init = (InitFunc)_get_symbol(instance, "begin");
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
  int size;

  SwappingCompilerImplementation() {
#ifdef USE_DYNAMIC
    void *handle = dlopen("libtcc.dylib", RTLD_NOW);

    if (handle == nullptr) {
      printf("DLOPEN ERROR:%s\n", dlerror());
      exit(1);
    }

    tcc[0]._new = (type_tcc_new)dlsym(handle, "tcc_new");
    tcc[0]._delete = (type_tcc_delete)dlsym(handle, "tcc_delete");
    tcc[0]._set_error_func =
        (type_tcc_set_error_func)dlsym(handle, "tcc_set_error_func");
    tcc[0]._set_options =
        (type_tcc_set_options)dlsym(handle, "tcc_set_options");
    tcc[0]._set_output_type =
        (type_tcc_set_output_type)dlsym(handle, "tcc_set_output_type");
    tcc[0]._define_symbol =
        (type_tcc_define_symbol)dlsym(handle, "tcc_define_symbol");
    tcc[0]._compile_string =
        (type_tcc_compile_string)dlsym(handle, "tcc_compile_string");
    tcc[0]._add_symbol = (type_tcc_add_symbol)dlsym(handle, "tcc_add_symbol");
    tcc[0]._add_library =
        (type_tcc_add_library)dlsym(handle, "tcc_add_library");
    tcc[0]._relocate = (type_tcc_relocate)dlsym(handle, "tcc_relocate");
    tcc[0]._get_symbol = (type_tcc_get_symbol)dlsym(handle, "tcc_get_symbol");
#else
    tcc[0]._new = tcc_new;
    tcc[0]._delete = tcc_delete;
    tcc[0]._set_error_func = tcc_set_error_func;
    tcc[0]._set_options = tcc_set_options;
    tcc[0]._set_output_type = tcc_set_output_type;
    tcc[0]._define_symbol = tcc_define_symbol;
    tcc[0]._compile_string = tcc_compile_string;
    tcc[0]._add_symbol = tcc_add_symbol;
    tcc[0]._add_library = tcc_add_library;
    tcc[0]._relocate = tcc_relocate;
    tcc[0]._get_symbol = tcc_get_symbol;
#endif

    tcc[1]._new = tcc_new;
    tcc[1]._delete = tcc_delete;
    tcc[1]._set_error_func = tcc_set_error_func;
    tcc[1]._set_options = tcc_set_options;
    tcc[1]._set_output_type = tcc_set_output_type;
    tcc[1]._define_symbol = tcc_define_symbol;
    tcc[1]._compile_string = tcc_compile_string;
    tcc[1]._add_symbol = tcc_add_symbol;
    tcc[1]._add_library = tcc_add_library;
    tcc[1]._relocate = tcc_relocate;
    tcc[1]._get_symbol = tcc_get_symbol;

    printf("%0X %0X\n", tcc[0]._new, tcc[1]._new);
  }

  bool compile(const string &code, string *err) {
#if _USE_TRY_LOCK
    if (tryLock && !lock.try_lock()) {
      string("No Lock");
      return false;
    }
#else
    lock.lock();  // blocking call
#endif

    bool compileSucceeded = false;

#ifdef USE_DYNAMIC
    printf("compiling with %s\n", ((1 - active) == 0) ? "dynamic" : "static");
#endif
    if (tcc[1 - active].compile(code, err)) {
      this->size = tcc[1 - active].size;
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

int SwappingCompiler::size() { return implementation->size; }

bool SwappingCompiler::operator()(const string &code, string *err) {
  return implementation->compile(code, err);
}

ProcessFunc SwappingCompiler::operator()(void) {
  return implementation->getFunctionMaybeSwap();
}
