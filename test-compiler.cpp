
#include <unistd.h>

#include <iostream>
#include <thread>

#include "compiler.h"

const char* code = R"(
typedef struct {
  float left, right;
} Data;
Data play(double t) {
  //
  return (Data){100 * t, 101 * t};
}
)";

int main(int argc, char* argv[]) {
  SwappingCompiler compiler;

  bool done = false;

  std::thread audio([&]() {
    while (!done) {
      if (compiler.checkForNewCode()) {
        printf("\nswapped in new code\n");
      }

      OutputType q = compiler.operator()();
      printf("%f:%f ", q.left, q.right);

      usleep(5000);
    }
  });

  // for (int i = 0; i < 100; i++) {
  // if (compiler.compile(code)) {
  for (int i = 0; i < 100;) {
    if (compiler.compileTry(code)) {
      i++;
      printf("\nSUCCESS!\n");
      fflush(stdout);
    } else {
      printf("\nFAIL!\n");
      fflush(stdout);
    }
    usleep(10000);
  }

  done = true;
  audio.join();

  return 0;
}
