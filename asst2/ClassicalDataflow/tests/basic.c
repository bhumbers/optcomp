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

int foo(int x, int y) 
{
  int b = x + y;
  int c = 45 + b;
  if (x > 5)
    return 5 + x;
  else
    return foo(y-1,1);
}

int bar(int x, int y)
{
  if (x > 2)
    return x + y;
  else 
    return bar(x - 1, y);
}

int basic_function_call(int x, int y) 
{
  int a = x * y;
  if (a < 42)
    return foo(a, 13) + 5;
  else
    a = x - 1;
  return foo(a, x);
}
