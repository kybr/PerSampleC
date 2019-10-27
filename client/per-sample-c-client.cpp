#include <lo/lo.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "libtcc.h"
using ProcessFunc = void (*)(double time, float *input, float *output);
using std::string;
std::string slurp(const std::string &fileName);
std::string slurp();

unsigned SAMPLE_RATE = 44100;

int main(int argc, char *argv[]) {
  // use command line arguments to...
  // set destination address and port (defaults 127.0.0.1:9010)
  // disable compile checking or other processes

  std::string sourceCode;
  if (argc > 1)
    sourceCode = slurp(argv[1]);
  else
    sourceCode = slurp();

  // try to compile (time this!)
  // if it fails, report error and exit
  // run a few times in case it crashes (time this?!)
  //

  TCCState *instance = tcc_new();
  tcc_set_options(instance, "-Wall -Werror -nostdinc -nostdlib");
  tcc_set_output_type(instance, TCC_OUTPUT_MEMORY);

  char buffer[10];
  sprintf(buffer, "%u", SAMPLE_RATE);
  tcc_define_symbol(instance, "SAMPLE_RATE", buffer);

  if (0 != tcc_compile_string(instance, sourceCode.c_str())) {
    exit(1);
  }

  // XXX for now, we leave this here, but eventually, we should
  // get this linker information from a modeline
  //
  tcc_add_library(instance, "m");

  int size = tcc_relocate(instance, nullptr);

  if (-1 == tcc_relocate(instance, TCC_RELOCATE_AUTO)) {
    printf("Relocate failed!");
    exit(1);
  }

  ProcessFunc function = (ProcessFunc)tcc_get_symbol(instance, "process");
  if (function == nullptr) {
    printf("No process callback found.");
    exit(1);
  }

  using InitFunc = void (*)(void);
  InitFunc init = (InitFunc)tcc_get_symbol(instance, "begin");
  if (init) init();

  ////////
  // better that this crash now than it crash the server
  float i[8] = {0};
  float o[8] = {0};
  for (int n = 0; n < 44100; n++)  //
    function((double)n / SAMPLE_RATE, i, o);

  // run pre-processor to remove comments and include files
  //
  // libtcc lets us run the preprocessor when we pass TCC_OUTPUT_PREPROCESS to
  // tcc_set_output_type(), but this spits the code out the standard output. we
  // must intercept this output to get it into a string. also, we might want to
  // do this step before anything else?
  //
  // https://stackoverflow.com/questions/5419356/redirect-stdout-stderr-to-a-string
  //

  // maybe...
  //   minify (instead of clang-format)
  //   extract "modeline" commands

  lo_blob b = lo_blob_new(sourceCode.size(), sourceCode.data());
  lo_address t = lo_address_new(nullptr, "9010");
  if (!lo_send(t, "/code", "b", b)) {
    printf("failed to send packet.");
    fflush(stdout);
    exit(1);
  }

  return 0;
}

std::string slurp(const std::string &fileName) {
  using namespace std;
  ifstream ifs(fileName.c_str(), ios::in | ios::binary | ios::ate);

  ifstream::pos_type fileSize = ifs.tellg();
  ifs.seekg(0, ios::beg);

  vector<char> bytes(fileSize);
  ifs.read(bytes.data(), fileSize);

  return string(bytes.data(), fileSize);
}

std::string slurp() {
  using namespace std;
  vector<char> cin_str;
  // 64k buffer seems sufficient
  streamsize buffer_sz = 65536;
  vector<char> buffer(buffer_sz);
  cin_str.reserve(buffer_sz);

  auto rdbuf = cin.rdbuf();
  while (auto cnt_char = rdbuf->sgetn(buffer.data(), buffer_sz))
    cin_str.insert(cin_str.end(), buffer.data(), buffer.data() + cnt_char);
  return string(cin_str.data(), cin_str.size());
}
