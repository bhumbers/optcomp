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

//Basic additive identity test for optimizer
int add_zero(int a) 
{
	return a + 0;
}

