#include <lo/lo.h>

#include <iostream>

#include "RtAudio.h"
#include "compiler.h"

//////////////////////////////////////////////////////////////////////////////
// AUDIO THREAD //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int process(void *outputBuffer, void *inputBuffer, unsigned frameCount,
            double streamTime, RtAudioStreamStatus status, void *data) {
  //
  //
  SwappingCompiler &compiler(*static_cast<SwappingCompiler *>(data));
  float *input = static_cast<float *>(inputBuffer);
  float *output = static_cast<float *>(outputBuffer);
  static double t = 0;
  float in[8] = {0};
  float out[8] = {0};

  // detect code changes by looking at the pointer
  //
  auto play = compiler.function();

  if (play) {
    unsigned i = 0;
    unsigned o = 0;
    for (unsigned _ = 0; _ < frameCount; _ = 1 + _) {
      play(t, in, out);
      t += 1.0 / 44100.0;
      for (unsigned k = 0; k < OUTPUT_COUNT; k++) {
        output[o] = out[k];
        o = 1 + o;
      }
    }
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

  SwappingCompiler &compiler(*static_cast<SwappingCompiler *>(user_data));

  // XXX
  //
  // - compute the hash of the code?
  // - increment a "build number" and store with the compiler?
  // - check if the code is exactly the same as last time

  /*
  static std::string lastCode = "";
  if ((lastCode.size() == sourceCode.size()) && (lastCode == sourceCode)) {
    // skip the compile
    printf("ignoring repeated code\n");
    return 0;
  }
  lastCode = sourceCode;
  */

  printf("compiling...\n%s\n", sourceCode.c_str());
  if (!compiler.compile(sourceCode.c_str())) {
    //  printf("FAIL!\n");
  }
  fflush(stdout);

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
