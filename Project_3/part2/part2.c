#include <stdio.h>
#include <stdlib.h>
 
int main(int argc, char *argv[])
{
  int i = 0;
  if (argc < 2)
    {
      printf("Not enough arguments\n");

    }
  else
    {
      i = atoi(argv[1]);
    }
  printf("Hello World %s\n", i);

  return 0;
}
