#include <stdio.h>

int main()
{
  unsigned val = 0x59b997fa;
  char s[10];
  sprintf(s, "%.8x", val);
  printf("%s\n", s);
}