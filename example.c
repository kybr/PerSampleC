
typedef struct {
  float left, right;
} Output;

float phasor(unsigned which, float frequency) {
  static struct { float phase; } state[10];
  float phase = state[which].phase;
  phase += frequency / 44100;
  if (phase > 1) phase -= 1;
  state[which].phase = phase;
  return phase;
}

Output play(double t) {
  float e = phasor(0, 2);
  e = 1 - e;
  e *= e * e;

  float f = phasor(3, 2.05);
  f = 1 - f;
  f *= f * f;

  float q = phasor(1, 100) * 2 - 1;
  float p = phasor(2, 100) * 2 - 1;
  q *= e;
  p *= f;
  return (Output){q, p};
}
