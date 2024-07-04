int main()
{
  volatile long long n;
  for (unsigned long int index = 0; index < 1000000; ++index)
  {
    for (unsigned long int index2 = 0; index2 < 20000; ++index2)
    {
      ++n;
    }
  }
}
