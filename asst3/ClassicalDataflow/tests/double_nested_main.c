////////////////////////////////////////////////////////////////////////////////
// 15-745 S14 Assignment 3
// Group: bhumbers, psuresh
////////////////////////////////////////////////////////////////////////////////

//Tests LICM for doubly nested loops

int main(void)
{
  int x = 5;
  int y = 1;
  int r = 10;
  for (int i = 0; i < 2*x; i++) {
    for (int j = 0; j < 12345*y; j++) {
      for (int k = 0; k < 5; k++) {
        int a = 5;
        r += a + y;
      }
    }
  }
  return r;
};