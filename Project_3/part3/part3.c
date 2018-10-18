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

void *server_socket();
void *client_socket();
void *fault_handler_thread();
void *menu();
void *mem_read();
void *mem_write();

struct uffdio_api uffdio_api;
struct uffdio_register uffdio_register;


typedef struct 
{
  char *addr;
  unsigned long pg_size;
  unsigned long uffd;
  unsigned long length;

} stuff;



int order;
char *buffer;

stuff data;
stuff recv_data;
stuff mch;
size_t page_size;


char *commands;


int main(int argc, char *argv[])
{
  pthread_t serv_thread;
  pthread_t cli_thread;
  pthread_t fd_thread;
  
  char *serv_num;
  char *cli_num;

  
  int serv_chk, cli_chk, fd_chk;
  
  serv_num = argv[1];
  cli_num = argv[2];  
  
  serv_chk = pthread_create( &serv_thread, NULL, server_socket, (void *)serv_num);
   if (serv_chk != 0) {
    errno = serv_chk;
    errExit("pthread_create");
  }
  
  cli_chk = pthread_create( &cli_thread, NULL, client_socket, (void *)cli_num);
  if (cli_chk != 0) {
    errno = cli_chk;
    errExit("pthread_create");
    }

  pthread_join( cli_thread, NULL);
  pthread_join( serv_thread, NULL);

  if (order == 1)
    {
      mch = data;
    }
  else if (order == 2)
    {
      mch = recv_data;
    }
  
  fd_chk = pthread_create(&fd_thread, NULL, fault_handler_thread, (void *) mch.uffd);
  if (fd_chk != 0) {
    errno = fd_chk;
    errExit("pthread_create");
  }
  sleep(1);
  menu();
  
  //pthread_join( fd_thread, NULL);  
  
  //printf("Server Port %s | Client Port %s | Machine # %d \n\n", serv_num, cli_num, order);
  return 0;
}



void* menu()
{
  char *commands = malloc(sizeof(char)*page_size);
  unsigned long access;
  char *message = malloc(sizeof(char)*page_size);
  do  {
    sleep(0.5);
    printf("\nPlease enter the command < Read = r | Write = w | Quit = q > : ");
    scanf("%s", commands);

    if(strcmp(commands,"r") == 0)
      {
	printf("For which page? (0-%ld, or -1 for all): ", mch.pg_size-1);
	scanf("%ld", &access);
	if (access < mch.pg_size || access == -1)
	  {
	    mem_read(access);
	  }
	else
	  {
	    printf("Can't read that page\n");
	  }
      }
    else if(strcmp(commands, "w") == 0)
      {
	printf("For which page? (0-%ld, or -1 for all): ", mch.pg_size-1);
	scanf("%ld", &access);
	if (access < mch.pg_size || access == -1)
	  {
	    while ((getchar()) != '\n');
	    printf("Enter your message: ");
	    scanf("%[^\n]s", message);
	    mem_write(access, message);
	  }
	else
	  {
	    printf("Can't write to that page\n");
	  }
      }
  } while(strcmp(commands, "q") != 0);

  free(commands);
  
  if(strcmp(commands, "q") == 0)
    {
      exit(1);
    }
  return 0;
}

void* mem_read(unsigned long num)
{
  int l = 0x0;
  char* c;
  printf("Reading page %ld\n", num);
  if (num == -1)
    {
      printf("Reading all pages\n");
      while (l < mch.length) {
	c = &mch.addr[l];
	printf("Read address %p: \n", mch.addr + l);
	printf("%s\n", c);
	l += page_size;
      }
    }
  else
    {
      l += (num*page_size);
      c = &mch.addr[l];
      printf("Read address %p: \n", mch.addr + l);
      printf("%s\n", c);
    }
  return 0;
}

void* mem_write(unsigned long num, char *msg)
{
  //  size_t str_len = strlen(msg);
  int l = 0x0;

  if (num == -1)
    {
      while( l < mch.length)
	{
	  memcpy(mch.addr+l, msg, strlen(msg));
	  l += page_size;
	}
    }
  else
    {
      l += (num*page_size);
      memcpy(mch.addr+l, msg, strlen(msg));
    }
  //printf("Write message %s in the page %ld\n", msg, num);
  //printf("length is: %ld\n", str_len);
  
  return 0;
}


void* fault_handler_thread(void *arg)
{
        static struct uffd_msg msg;   /* Data read from userfaultfd */
        long uffd;                    /* userfaultfd file descriptor */
	char *page = NULL;
        struct uffdio_copy uffdio_copy;
        ssize_t nread;

	uffd = (long) arg;
	if (page == NULL) {
                page = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                if (page == MAP_FAILED)
                        errExit("mmap");
        }
	for (;;) {
	  struct pollfd pollfd;
	  int nready;

	  pollfd.fd = uffd;
	  pollfd.events = POLLIN;
	  nready = poll(&pollfd, 1, -1);
	  if (nready == -1)
	    errExit("poll");
	  
	  printf("\n[x] PAGEFAULT\n");
	  /*
	  printf("    poll() returns: nready = %d; "
		 "POLLIN = %d; POLLERR = %d\n", nready,
		 (pollfd.revents & POLLIN) != 0,
		 (pollfd.revents & POLLERR) != 0);
	  */
	  nread = read(uffd, &msg, sizeof(msg));
	  if (nread == 0) {
	    printf("EOF on userfaultfd!\n");
	    exit(EXIT_FAILURE);
	  }
	  
	  if (nread == -1)
	    errExit("read");
		
	  if (msg.event != UFFD_EVENT_PAGEFAULT) {
	    fprintf(stderr, "Unexpected event on userfaultfd\n");
	    exit(EXIT_FAILURE);
	  }
	  /*
	  printf("    UFFD_EVENT_PAGEFAULT event: ");
	  printf("flags = %llx; ", msg.arg.pagefault.flags);
	  printf("address = %llx\n", msg.arg.pagefault.address);
	  */
	  uffdio_copy.src = (unsigned long) page;
	  uffdio_copy.dst = (unsigned long) msg.arg.pagefault.address &
	    ~(page_size - 1);
	  
	  uffdio_copy.len = page_size;
	  uffdio_copy.mode = 0;
	  uffdio_copy.copy = 0;
	  
	  if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy) == -1)
	    errExit("ioctl-UFFDIO_COPY");
	  /*	  
	  printf("        (uffdio_copy.copy returned %lld)\n",
		 uffdio_copy.copy);
	  */
	}

}

void *server_socket(void *portnum)
{
  struct sockaddr_in serv_addr, cli_addr; 
  int opt = 1;
  int server_fd, new_socket, client_size;
  int port = atoi(portnum);

  
  printf("Server Socket with port = %d\n", port );  
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
    
  }
  if (listen(server_fd, 3) < 0) {
    perror("listen");

  }
  client_size = sizeof(struct sockaddr_in);
  if( (new_socket = accept(server_fd, (struct sockaddr *)&cli_addr, (socklen_t*)&client_size)) )
    {
      printf("\nMachine # is %d\n", order);
      if (order == 1)
	{
	  sleep(1);
	  printf("Enter the page count: ");
	  scanf("%lu", &data.pg_size);
	  unsigned long len;  /* Length of region handled by userfaultfd */
	  printf("\nInvoking userfaultfd initialization\n");
 
	  page_size = sysconf(_SC_PAGE_SIZE);
	  len = data.pg_size * page_size;
	  data.length = len;
	  
	  data.addr = mmap(NULL, len, PROT_READ | PROT_WRITE,
			   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	  if (data.addr == MAP_FAILED)
	    {
	      printf("Failed\n");
	      errExit("mmap");
	    }

	  write(new_socket, &data, sizeof(stuff));
	  data.uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);  
	  if (data.uffd == -1)
	    errExit("userfaultfd");

	  printf("\nSending to the mch #2 mmap addr = %p pg_cnt = %ld\n", data.addr, data.pg_size);

	  uffdio_api.api = UFFD_API;
	  uffdio_api.features = 0;
	  if (ioctl(data.uffd, UFFDIO_API, &uffdio_api) == -1)
	    errExit("ioctl-UFFDIO_API");
	  
	  uffdio_register.range.start = (unsigned long) data.addr;
	  uffdio_register.range.len = len;
	  uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
	  if (ioctl(data.uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
	    {
	      errExit("ioctl-UFFDIO_REGISTER");
	    }
	  printf("Finished Initialization\n");

	}

    }

  return 0;
}

void *client_socket(void *portnum)
{
  struct sockaddr_in serv_addr;
  int port;
  port = atoi(portnum);
  int client_fd;
  char *recv_addr;
  
  if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }
  printf("\nClient Socket with port = %d\n", port);
  serv_addr.sin_family = AF_INET;
  if(inet_aton("127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
      printf("\nInvalid Address | Address not supported \n");
      exit(1);
    }
  serv_addr.sin_port = htons(port);
  if ((connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
      order = 1;
      while ((connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
	{
	  printf("\nConnection Failed \n");
	  sleep(1);
	} 
    }
  else
    {
      order = 2;
    }

  printf("\nConnection Established with %d\n", port);
  
  if (order == 2)
    {

      recv(client_fd, &recv_data, sizeof(recv_data), MSG_WAITALL);
      unsigned long len;  /* Length of region handled by userfaultfd */
      printf("\nInvoking userfaultfd initialization\n");

      printf("\nReceiving from the mch #1 mmap addr = %p pg_cnt = %ld\n", recv_data.addr, recv_data.pg_size);

      
      page_size = sysconf(_SC_PAGE_SIZE);
      len = recv_data.pg_size * page_size;

      recv_data.length = len;
      
      recv_data.uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
      if (recv_data.uffd == -1)
	errExit("userfaultfd");
      
      recv_addr = mmap(recv_data.addr, len, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (recv_addr == MAP_FAILED)
	{
	  printf("Failed\n");
	  errExit("mmap");
	}
      
	  
      uffdio_api.api = UFFD_API;
      uffdio_api.features = 0;
      if (ioctl(recv_data.uffd, UFFDIO_API, &uffdio_api) == -1)
	errExit("ioctl-UFFDIO_API");
	  
      uffdio_register.range.start = (unsigned long) recv_addr;
      uffdio_register.range.len = len;
	  
      uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
      
      if (ioctl(recv_data.uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
	{
	  errExit("ioctl-UFFDIO_REGISTER");
	}
      printf("Finished Initialization\n");
      //      menu();
    }
 
  return 0;
}

