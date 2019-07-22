#include <lo/lo.h>

#include <iostream>

#include "RtAudio.h"
#include "compiler.h"

//////////////////////////////////////////////////////////////////////////////
// AUDIO THREAD //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int process(void *outputBuffer, void *inputBuffer, unsigned frameCount,
            double streamTime, RtAudioStreamStatus status, void *data) {
  auto compiler = static_cast<SwappingCompiler *>(data);

  if (compiler->checkForNewCode()) {
    printf("swapped in new code\n");
  }

  unsigned n = 0;
  float *o = static_cast<float *>(outputBuffer);
  for (unsigned i = 0; i < frameCount; i = 1 + i) {
    OutputType q = compiler->operator()();
    o[n++] = q.left;
    o[n++] = q.right;
    // for (unsigned k = 0; k < OUTPUT_COUNT; k++)  //
    //  o[n++] = q.data[k];
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////////
// COMPILER THREAD ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int handle_code(const char *path, const char *types, lo_arg **argv, int argc,
                void *data, void *user_data) {
  unsigned char *blobdata = (unsigned char *)lo_blob_dataptr((lo_blob)argv[0]);
  int blobsize = lo_blob_datasize((lo_blob)argv[0]);

  std::string sourceCode(blobdata, blobdata + blobsize);

  auto compiler = static_cast<SwappingCompiler *>(user_data);

  printf("compiling...\n%s\n", sourceCode.c_str());
  // if (compiler->compileTry(sourceCode.c_str())) {
  if (compiler->compile(sourceCode.c_str())) {
    printf("SUCCESS!\n");
    fflush(stdout);
  } else {
    printf("FAIL!\n");
    fflush(stdout);
  }

  return 0;
}

void handle_error(int num, const char *msg, const char *path) {
  printf("liblo server error %d in path %s: %s\n", num, path, msg);
  fflush(stdout);
}

//////////////////////////////////////////////////////////////////////////////
// MAIN THREAD ///////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
  SwappingCompiler compiler;

  // SERVER/COMPILER THREAD
  //
  lo_server_thread s = lo_server_thread_new("9010", handle_error);
  lo_server_thread_add_method(s, "/code", "b", handle_code, &compiler);
  lo_server_thread_start(s);

  // AUDIO THREAD
  //
  RtAudio dac;

  if (dac.getDeviceCount() < 1) {
    printf("No audio devices found!\n");
    exit(0);
  }

  unsigned frameCount = 512;
  unsigned sampleRate = 44100;
  RtAudio::StreamParameters oParams;
  oParams.deviceId = dac.getDefaultOutputDevice();
  oParams.nChannels = OUTPUT_COUNT;

  try {
    dac.openStream(&oParams, nullptr, RTAUDIO_FLOAT32, sampleRate, &frameCount,
                   &process, &compiler);
    dac.startStream();
  } catch (RtAudioError &e) {
    printf("ERROR: %s\n", e.getMessage().c_str());
    fflush(stdout);
  }

  ////////////////////////////////////////////////////////////////////////////
  // WAIT FOR QUIT
  ////////////////////////////////////////////////////////////////////////////

  printf("Hit enter to quit...\n");
  fflush(stdout);
  getchar();

  try {
    dac.stopStream();
    dac.closeStream();
  } catch (RtAudioError &error) {
    error.printMessage();
  }

  lo_server_thread_free(s);
  return 0;
}
