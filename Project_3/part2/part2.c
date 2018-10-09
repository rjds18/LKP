#define _GNU_SOURCE
#include <stdio.h>
#include <linux/userfaultfd.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>                                                                            
#define PAIR_PORT1 8080 //server port number
#define PAIR_PORT2 2000 //client port number
#define MAX_SIZE 4096

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE);   \
        } while (0)

char hello[50] = "Hello from Server";

void server_socket(int process_type, int mmap_size)
{
  char* addr;
  char c[5] = {'A', 'B', 'C', 'D', 'E'};

  struct sockaddr_in serv_addr, cli_addr;
  
  int i = 0;
  int opt = 1;
  int server_fd;
  int new_socket;
  int port;
  char string[MAX_SIZE] = {0};
  if (process_type == 0)
    {
      port = PAIR_PORT1;
    }
  else
    {
      port = PAIR_PORT2;
    }
  int addrlen = sizeof(serv_addr);

  printf("Server Socket with port = %d\n", port);  
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
		 &opt, sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons( port );
  
  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("bind failed");
    exit(1);
  }
  if (listen(server_fd, 5) < 0) {
    perror("listen");
    exit(1);
  }
  int clilen = sizeof(cli_addr);
  int addrlen2 = sizeof(serv_addr);
  if ((new_socket = accept(server_fd, (struct sockaddr *)&serv_addr,
			   (socklen_t*)&addrlen2) < 0)) {
    perror("accept");
    exit(EXIT_FAILURE);
  }

  int j = send(new_socket , "1" , 1024 , 0 );
  printf("String sending: %s %d %d\n", hello, j, new_socket);

}


void client_socket( int process_type, int mmap_size)
{
  struct sockaddr_in serv_addr;
  int port;
  int client_fd;
  char buffer[MAX_SIZE] = {0};
  if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }
  if (process_type == 0)
    {
      port = PAIR_PORT2;
    }
  else
    {
      port = PAIR_PORT1;
    }

  int addrlen = sizeof(serv_addr);
  printf("\nClient Socket with port = %d\n", port);
  memset(&serv_addr, '0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
	
  if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    exit(EXIT_FAILURE);
  }
  printf("Passed inet\n");
  if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    exit(EXIT_FAILURE);
  }
  printf("Press any key to continue...\n");
  getchar();

  recv(client_fd, buffer, sizeof(buffer), 0);
  printf("Message from a server: %s\n", buffer);
}

void select_function(char* socket_type, int mmapnum, int ptype)
{
  if(strcmp(socket_type,"s") == 0)
    {
      server_socket( ptype, mmapnum);
    }
  else if(strcmp(socket_type, "c") == 0)
    {
      client_socket( ptype, mmapnum);
    }
  else
    {
      printf("Not Applicable\n");
    } 
}


int main(int argc, char *argv[])
{
  char* server_type;
  char* client_type;
  int mmapsize;
  int proc_type;
  int i = 0;
  unsigned char *data = malloc(sizeof(void *));
  
  if (argc < 4)
    {
      printf("Not enough arguments\n");
    }
  else
    {
      server_type = argv[1];
      mmapsize = atoi(argv[2]);
      client_type = argv[3];
      proc_type = atoi(argv[4]);

      int port_num;
      printf("Enter the port number: ");
      scanf("%d", &port_num);
      printf("\n");
      printf("Port number entered: %d\n", port_num);
      
      if (proc_type == 0)	
	{
	  select_function(server_type, mmapsize, proc_type);  
	}
      else if (proc_type == 1)
	{
	  select_function(client_type, mmapsize, proc_type);
	}
    }


  

  return 0;
}
