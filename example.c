
typedef struct {
  float left, right;
} Data;

Data play(double t) {
  t *= 10;
  int T = t;
  float q = t - T;
  float p = t - T;
  //
  return (Data){q, p};
}
