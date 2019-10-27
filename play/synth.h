float memory[N];
unsigned _index;

float phasor(float frequency) {
  float* phase = &memory[_index];
  _index += 1;
  phase[0] += frequency / 44100;
  if (phase[0] > 1)  //
    phase[0] -= 1;
  return phase[0];
}

float onepole(float x, float a) {
  float* history = &memory[_index];
  _index += 1;
  return history[0] = history[0] * a + (1 - a) * x;
}
