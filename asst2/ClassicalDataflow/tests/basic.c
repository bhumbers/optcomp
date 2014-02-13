int basic_return_val(int x)
{
  return x;
};

void basic_no_return_val(int x) 
{
  x = x + 1; //probably will be optimized out, but oh well
};