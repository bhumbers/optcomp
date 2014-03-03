int basic_loop(int x)
{
  int r = 10;
  for (int i = 0; i < 42*x; i++) {
    r += x;
  }
  return r;
};
