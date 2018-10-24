#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>


#define PORT 5984
#define BUFF_SIZE 4096


void server_socket()
{
  int server_fd, new_socket;
  struct sockaddr_in address;

  int opt = 1;
  int addrlen = sizeof(address);
  char buffer[BUFF_SIZE] = {0};
  char *hello = "Hello from server";
  
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
		  &opt, sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons( PORT );

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  for (;;) {
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
			     (socklen_t*)&addrlen)) < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }

    printf("Press any key to continue...\n");
    getchar();
    
    read( new_socket , buffer, 1024);
    printf("Message from a client: %s\n",buffer );
    
    send(new_socket , hello , strlen(hello) , 0 );
    printf("Hello message sent\n");
  }
}

void client_socket()
{
  int sock = 0;
  struct sockaddr_in serv_addr;
  char *hello = "Hello from client";
  char buffer[BUFF_SIZE] = {0};
  
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    exit(1);
  }
  
  memset(&serv_addr, '0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  
  if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    exit(1);
  }
  
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    exit(1);
  }
  
  printf("Press any key to continue...\n");
  getchar();
  
  send(sock , hello , strlen(hello) , 0 );
  printf("Hello message sent\n");
  
  read( sock , buffer, 1024);
  printf("Message from a server: %s\n",buffer );
  
   
}


int main()
{
  printf("Hello World\n");
  int c;

  printf("Enter a value :");
  c = getchar();

  if(c == 0)
    {
      server_socket();
    }
  else if(c ==1)
    {
      client_socket();
    }
}

