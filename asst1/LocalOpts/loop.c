int g;
int g_incr (int c)
{
  g += c;
  return g;
}
int loop (int a, int b, int c)
{
  int i;
  int ret = 0;
  for (i = a; i < b; i++) {
   g_incr (c);
  }
  return ret + g;
}
int Fact( int a)
{
if(a == 1)
     return 1;
else
       {
       return (a * (Fact(a-1)));
       }
}

int var_ar(int a, ...) {
  return 42 + a;
}

void call()
{
Fact(5);
}
