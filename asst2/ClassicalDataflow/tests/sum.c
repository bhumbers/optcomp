
//Simple example from Asst2 w/ expected liveness output provided in writeup
int sum (int a, int b)
{
  int i;
  int res = 1;

  for (i = a; i < b; i++)
    res *= i;

  return res;
}
