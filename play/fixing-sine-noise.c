#include "math.h"

//#define N (2 * 2 * 2 * 2 * 2 * 2 * 2 * 2)
#define N (1000)
float table[N];
void begin() {
  for (int i = 0; i < N; i++) {
    table[i] = sin(2 * M_PI * i / N);
  }
}

void process(double t, float* i, float* o) {
  {
    float phase = t * 1;
    phase -= (int)phase;
    float f = sin(M_PI * 2 * phase);
    o[2] = f * 0.99;

    float g = table[(int)(phase * (N - 1))];
    o[3] = g;
  }
  {
    double phase = t * 400;
    // using double fixes the noise!
    // double phase = t * 1000;
    phase -= (long)phase;
    float f = sin(M_PI * 2 * phase);
    o[0] = f;

    float g = table[(int)(phase * (N - 1))];
    o[1] = g;
  }
  // o[0] = o[1] = 0;
}
