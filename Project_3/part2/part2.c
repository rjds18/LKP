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
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define MAX_SIZE 4096

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE);   \
        } while (0)

int serv_num;
int cli_num;
int mem_size;

int test;

void *server_socket();
void *client_socket();

int main(int argc, char *argv[])
{
  
  //pthread_t serv_thread;
  pthread_t cli_thread;
  /*
  int server_t;
  int client_t;
  *//*
   printf("Enter the port number for the server: ");
  scanf("%d", &serv_num);
  printf("\n");

  printf("Enter the port number for the client: ");
  scanf("%d", &cli_num);
  printf("\n");
    */
  //  client_socket();
  
  pthread_create( &cli_thread, NULL, client_socket, NULL);
  pthread_join(cli_thread, NULL);
  /*  printf("Testing: Server=0 | Client=1: ");
  scanf("%d", &test);
  printf("\n");
  
 
   
  printf("Enter the memory size: ");
  scanf("%d", &mem_size);
  printf("Server Port %d | Client Port %d | Mem size entered: %d \n", serv_num, cli_num, mem_size);
  *//*
  if (test == 0)
    {
      printf("here");
      
      server_t = pthread_create( &serv_thread, NULL, server_socket, NULL);
      if (server_t < 0)
	{
	  printf("Error\n");
	}
    }
  else if (test == 1)
    {
      client_t =
      if (client_t < 0)
	{
	  printf("Error\n");
	}
      pthread_join( cli_thread, NULL);
    }
    */
  return 0;
}

void *server_socket()
{
  char* addr;
  struct sockaddr_in serv_addr, cli_addr;
  
  int opt = 1;
  int server_fd, new_socket, client_size;
  int port = serv_num;
  
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
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons( port );
  
  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("bind failed");
    exit(1);
  }
  if (listen(server_fd, 3) < 0) {
    perror("listen");
    exit(1);
  }
  printf("Trying to connect ... \n");
  
  client_size = sizeof(struct sockaddr_in);
  
  if( (new_socket = accept(server_fd, (struct sockaddr *)&cli_addr, (socklen_t*)&client_size)) )
    {
      printf("Connection accepted\n");
      printf("\nServer Socket with mmap_size of %d and port = %d\n", mem_size, port);
      addr = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
		  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
      if (addr == MAP_FAILED)
	{
	  errExit("mmap");
	}
      
      write(new_socket, &addr, sizeof(addr));
      printf("Address returned by mmap() = %p\n", addr);
      
    }
  close(server_fd);
  return 0;
}


void *client_socket()
{
  struct sockaddr_in serv_addr;
  int port = cli_num;
  int client_fd = 0;
  char* buffer;
  char* addr;
  
  if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }
  printf("\nClient Socket with port = %d\n", port);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  if(inet_aton("127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
      printf("\nInvalid Address | Address not supported \n");
      exit(1);
    }  
  printf("Passed inet\n");
  while ((connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) {
    printf("\nConnection Failed \n");
    sleep(1);
  }
  int i =  read(client_fd, &buffer, sizeof(buffer));
  printf("Reading %d %p \n", i , buffer );
  //write(client_fd, &number, sizeof(number));
  addr = mmap(buffer, mem_size, PROT_READ | PROT_WRITE,                                       
	      MAP_SHARED | MAP_ANONYMOUS, -1, 0);                                            
  if (addr == MAP_FAILED)                                                                      
    {                                                                                          
      errExit("mmap");
    }      
  printf("Address returned by mmap() = %p Page size = %d \n", addr, mem_size);

  return 0;
}

