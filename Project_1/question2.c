#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
  int opt;
  
  char *strval;
  strval = (char*)malloc(sizeof(char)*100);
  int keyval = 0;
  
  while((opt = getopt(argc, argv, "s:k:" )) !=-1)
    {
      switch(opt)
	{
	case 's':
	  strval = optarg;
	  break;
	case 'k':
	  keyval = atoi(optarg);
	  break;
	  
	}

    }
  
  printf("Hello, invoking q2 sys call\n");
  printf("string ");
  printf("%s",strval);
  printf("\n");
  printf("encrypt = %d\n", keyval);

  long int sys_status = syscall(335, strval, keyval);
  //long int sys_status = sys_s2_encrypt("hello", 5);
  if (sys_status == 0)
    {
      printf("syscall has returned 0, use dmesg to check the info\n");
    }
  else
    {
      //int buffer[1024];
      //      char * eMessage = perror(errno);
      printf("syscall failed; EINVAL = %s\n", strerror(errno));
    }

  return 0;
}
