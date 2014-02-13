//Liveness testing for locally exposed vs. non-locally exposed usages
int loc_exp_usage(int a) 
{
  int x = 5 + a;
  int y = 0;

  for (int i = 0; i < a; i++) {
    y += x;
  }
    
  return y;
}