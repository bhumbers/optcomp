int super_basic_for_non_ssa(int x) 
{
  int y = 42;
  y = x + 1;
  x = 12345;
  return x + y;
};