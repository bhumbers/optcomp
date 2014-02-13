//Liveness testing for locally exposed vs. non-locally exposed usages
int loc_exp_usage(int a) 
{
  int x = a + 42;
  if (a < 0)
    return x;
  else
    return x + 1;
}