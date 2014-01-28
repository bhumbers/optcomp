int compute (int a, int b)
{
  int result = (a/a);

  result *= (b/b);
  result += (b-b);
  result /= result;
  result -= result;
  return result;
}

//Basic additive identity test for optimizer
int add_zero(int a) 
{
	return a + 0;
}

//Basic multiplicative identity test for optimizer
int mul_one(int a) 
{
	return a * 1;
}

//Combination of a few identities
int alg_identity_combo(int a) 
{
	int b = (a * 1) + 0;
	return b;
}

