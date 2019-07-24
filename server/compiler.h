#pragma once

#include <string>

// signature each c sketch
//
using ProcessFunc = void (*)(double time, float* input, float* output);

// "pimpl" pattern hides implementation
//
class SwappingCompilerImplementation;

//
//
class SwappingCompiler {
  SwappingCompilerImplementation* implementation;

 public:
  SwappingCompiler();
  ~SwappingCompiler();

  // call from the server thread
  //
  std::string operator()(const std::string& code, bool tryLock = false);

  // call from the audio thread
  //
  ProcessFunc operator()();
};
