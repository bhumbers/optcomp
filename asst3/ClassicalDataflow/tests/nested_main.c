////////////////////////////////////////////////////////////////////////////////
// 15-745 S14 Assignment 3
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

//Tests LICM for nested loops

int main(void)
{
  int x = 5;
  int y = 1;
  int r = 10;
  for (int i = 0; i < 2*x; i++) {
    for (int j = 0; j < 12345*y; j++) {
      r += y;
    }
  }
  return r;
};