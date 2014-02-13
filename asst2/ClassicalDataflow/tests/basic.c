int basic_return_val(int x)
{
  return x;
};

void basic_no_return_val(int x) 
{
  x = x + 1; //probably will be optimized out, but oh well
};

int basic_with_double_branch(int x, int y)
{ 
  int z = 42;
  if (x < 0)  
    if (y > 0)
      z = x + y;
    else
      z = x - y;
  else 
    z = x + x;
  return z;
}