#include "math.h"

#define N (1000)

float memory[N];
unsigned _index = 0;

float mtof(float m) {  //
  return 8.175799 * pow(2.0, m / 12.0);
}

float mix(float a, float b, float t) {  //
  return (1 - t) * a + t * b;
}

float phasor(float frequency) {
  float phase = memory[_index];
  phase += frequency / SAMPLE_RATE;
  while (phase > 1)  //
    phase -= 1;
  memory[_index] = phase;
  _index++;
  return phase;
}

float onepole(float x0, float a1) {
  float y1 = memory[_index];
  y1 = (1 - a1) * x0 + a1 * y1;
  memory[_index] = y1;
  _index++;
  return y1;
}

float ms20(float x, float f, float q) {
  float* z = &memory[_index];
  _index += 2;
  float t = exp(-2 * M_PI * f / 44100);
  float s = tanh(q * z[1]);
  z[0] = mix(x - s, z[0], t);
  z[1] = mix(z[0] + s, z[1], t);
  return z[1];
}

float quasi(float frequency, float filter) {
  float* z = &memory[_index];
  _index += 2;
  float w = frequency / 44100;
  float b = 13 * pow(0.5 - w, 4.0) * filter;
  float phase = 2 * phasor(frequency) - 1;
  z[0] = (z[0] + sin(M_PI * (phase + b * z[0]))) / 2;
  float o = 2.5 * z[0] - 1.5 * z[1];
  z[1] = z[0];
  return (o + (0.376 - w * 0.752)) * (1.0 - 2.0 * w);
}

// take a phasor/ramp on (0, 1) and divide it into integer pieces
// ...but what if it's not linear? ooooooo.
float subdiv(float p, float d) {
  p *= d;
  return p - (int)p;
}

void process(double t, float* i, float* o) {
  _index = 0;  // reset the memory pointer

  if (0) {
    static double k = 0;
    float r = pow(2, 1.3 * (2 * phasor(1.0 / 17) - 1));
    k += 0.00005 * r;
    t = k;
  } else {
    t *= 1.2;
    // t *= 4.2;
  }

  int T = t;
  float e = t - T;
  e = 1 - e;
  // e = e * e;
  e = onepole(e, 0.97);
  float f = onepole(((float[]){34, 37, 44, 53, 36, 57, 33, 48})[T % 8], 0.996);
  float v = quasi(mtof(f), 1.0);
  v = e * ms20(v, mtof(134 * mix(0.1, 0.7, subdiv(e, 2))), 1.5);
  o[0] = o[1] = v * 0.707;
}
