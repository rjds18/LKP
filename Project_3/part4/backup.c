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
#include <stdbool.h>


#define MAX_SIZE 4096

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE);   \
        } while (0)

void *server_socket();
void *client_socket();
void *fault_handler_thread();
void *menu();
void *mem_read();
void *mem_write();
void *update_bus();

struct uffdio_api uffdio_api;
struct uffdio_register uffdio_register;

typedef struct
{
  bool M;
  bool S;
  bool I;
} MSIList;


typedef struct 
{
  char *addr;
  unsigned long uffd;
  unsigned long pg_size;
  int message;
  unsigned long length;
} stuff;

typedef struct
{
  int BusRd;
  int BusRdX;
  int BusUpgr;
   
} bus_info;

int order;
char *buffer;
char commands;

stuff data;
stuff recv_data;
stuff mch;
size_t page_size;

int client_fd, server_fd, new_socket;
int client_size;
int fd_chk;


char *serv_num;
char *cli_num;

unsigned long accessvar;

pthread_t fd_thread;

int main(int argc, char *argv[])
{
  pthread_t serv_thread;
  pthread_t cli_thread;
  int serv_chk, cli_chk ;
  
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
  sleep(1);
  return 0;
}

ssize_t nread;

void* update_bus(ssize_t flag, unsigned long ax, bus_info sigs)
{
  if (order == 1 && commands == 'w')
    {
      printf("Mch #1 needs to update the bus\n");
    }
  else if (order == 1 && commands == 'r')
    {
      printf("Mch #1 needs to update the bus\n");
    }
  printf("flag: %ld page #: %ld\n", flag, ax);
  //printf("%d %d %d\n", sigs.BusRd, sigs.BusRdx, sigs.BusUpgr);
  //printf("After pagefault, i need send nread to the second machine to update userfaultfd\n");
  if (order == 2 && sigs.BusRdX == 1)
    {
      printf("Need to invalide my value\n");
    }
  return 0;
}


void* fault_handler_thread(void *arg)
{
        static struct uffd_msg msg;   /* Data read from userfaultfd */
        long uffd;                    /* userfaultfd file descriptor */
	char *page = NULL;
        struct uffdio_copy uffdio_copy;
	bus_info sig;

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
	  /*	  
	  while (commands == '\n')
	    {
	      printf("looping\n");
	    }
	  */
	  printf("before PAGEFAULT %ld\n", nread);
	  nread = read(uffd, &msg, sizeof(msg));

	  if (nread == 0) {
	    printf("EOF on userfaultfd!\n");
	    exit(EXIT_FAILURE);
	  }
	  
	  if ((order == 1 && commands == 'w') || (order == 2 && commands == 'w'))
	    {
	      sig.BusRdX = 1;
	      printf("Invalidate other processes cache line\n");
	      update_bus(nread, accessvar, sig);
	    }
	  else if ((order == 2 && commands == 'r') || (order == 1 && commands == 'r'))
	    {
	      sig.BusRd = 1;
	      printf("Invalidate other processes cache line\n");
	      update_bus(nread, accessvar, sig);
	    }

		  
	  printf("\n[x] PAGEFAULT %ld %ld\n", nread, accessvar);

	  //	  printf("nread %ld\n", nread);
	  if (nread == -1)
	    errExit("read");
		
	  if (msg.event != UFFD_EVENT_PAGEFAULT) {
	    fprintf(stderr, "Unexpected event on userfaultfd\n");
	    exit(EXIT_FAILURE);
	  }
	  uffdio_copy.src = (unsigned long) page;
	  uffdio_copy.dst = (unsigned long) msg.arg.pagefault.address &
	    ~(page_size - 1);
	  
	  uffdio_copy.len = page_size;
	  uffdio_copy.mode = 0;
	  uffdio_copy.copy = 0;
	  
	  if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy) == -1)
	    errExit("ioctl-UFFDIO_COPY");
	}

}

void *server_socket(void *portnum)
{
  struct sockaddr_in serv_addr, cli_addr; 
  int opt = 1;
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
			   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	  if (data.addr == MAP_FAILED)
	    {
	      printf("Failed\n");
	      errExit("mmap");
	    }
	  printf("mmap generated address %p \n", data.addr);
	  /*MSI Array*/
	  
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
	    
	  fd_chk = pthread_create(&fd_thread, NULL, fault_handler_thread, (void *) data.uffd);
	  if (fd_chk != 0) {
	    errno = fd_chk;
	    errExit("pthread_create");
	  }
	  /*------------menu-----------*/

	  char *recv_message = malloc(sizeof(char)*page_size);
	  char *message = malloc(sizeof(char)*page_size);
	  //	  enum MSI list[data.pg_size];
	  MSIList list[data.pg_size];

	  do  {
	    
	    sleep(0.5);
	    printf("\nPlease enter the command < Read = r | Write = w | View = v | Quit = q > : ");
	    while ((getchar()) != '\n');
	    scanf("%c", &commands);
	    //	    if(strcmp(commands,"r") == 0)
	    if(commands == 'r')
	      {
		/*if pagefault occurs here, then I generate BusRd signal*/
		/*BusRd signal will then try to recv message from the shared memory*/
		
		printf("For which page? (0-%ld, or -1 for all): ", data.pg_size-1);
		scanf("%ld", &accessvar);
		
		if (accessvar < data.pg_size || accessvar == -1)
		  {
		    list[accessvar].S = 1;
		    
		    int l = 0x0;
		    int j = 0x0;
		    int k = 0x0;
		    char* c;
		    printf("Reading page %ld\n", accessvar);
		    if (accessvar == -1)
		      {
			if (order == 1)
			  {
			    while (k < data.length)
			      {
				memset(data.addr+k, 0, sizeof(page_size));
				k += page_size;
			      }
			    while( j < data.length)
			      {
				memcpy(data.addr+j, recv_message, strlen(recv_message));
				j += page_size;
			      }
			  }	
			printf("Reading all pages\n");
			while (l < data.length) {
			  c = &data.addr[l];

			  printf("Reading the string: %s\n", c);
			  l += page_size;
			}

		      }
		    else
		      {
			l += (accessvar*page_size);
			if (order == 1)
			  {
			    memset(data.addr+l, 0, sizeof(page_size));
			    printf("hi\n");

			    memcpy(data.addr+l, recv_message, strlen(recv_message));
			  }
			c = &data.addr[l];
			printf("Reading the string: %s\n", c);			


		      }
		  }
		else
		  {
		    printf("Can't read that page\n");
		  }
	      }

	    if(commands == 'w')
	      {
		printf("For which page? (0-%ld, or -1 for all): ", data.pg_size-1);
		scanf("%ld", &accessvar);
		while ((getchar()) != '\n');
		printf("Enter your message: ");
		scanf("%[^\n]s", message);
		if (order == 1)
		  {
		    write(new_socket, message, sizeof(char)*page_size);
		  }					
		
		if (accessvar < data.pg_size || accessvar == -1)
		  {

		    //mem_write(access, message, data);
		    int l = 0x0;
		    int j = 0x0;
		    if (accessvar == -1)
		      {
			while (l < data.length)
			  {
			    memset(data.addr+l, 0, sizeof(page_size));
			    l += page_size;
			  }
			while( j < data.length)
			  {
			    memcpy(data.addr+j, message, strlen(message));
			    j += page_size;
			  }
		      }
		    else
		      {
			printf("Here with %ld\n", accessvar);
			printf("Read address %p: \n", data.addr + l);
			l += (accessvar*page_size);
			memset(data.addr+l, 0, sizeof(page_size));
			memcpy(data.addr+l, message, strlen(message));

		      }
		  }
		else
		  {
		    printf("Can't write to that page\n");
		  }
	      }

	    if(commands == 'v')
	      {
		int start = 0;
		while (start < data.pg_size)
		  {
		   
		    start++;
		  }
	      }
	  } while(commands != 'q');
	  
	  free(message);
	  if(commands == 'q')
	    {
	      exit(1);
	    }
	  /*-----------menu ends--------*/
	}
    }

  return 0;
}

void *client_socket(void *portnum)
{
  struct sockaddr_in serv_addr;
  int port;
  port = atoi(portnum);
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

      /*MSI Array*/
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
		       MAP_SHARED | MAP_ANONYMOUS, -1, 0);
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

      fd_chk = pthread_create(&fd_thread, NULL, fault_handler_thread, (void *) recv_data.uffd);
      if (fd_chk != 0) {
	errno = fd_chk;
	errExit("pthread_create");
      }

      /*------------menu-----------*/
      //      char *commands = malloc(sizeof(char)*page_size);

      char *recv_message = malloc(sizeof(char)*page_size);
      char *message = malloc(sizeof(char)*page_size);

      do  {
	//	
	sleep(0.5);
	printf("Please enter the command < Read = r | Write = w | View = v | Quit = q > : ");
	scanf("%c", &commands);    
	//	if(strcmp(commands,"r") == 0)
	if(commands == 'r')
	  {
	    /*
	    if (first == 0)
	      {
		read(client_fd, recv_message, sizeof(char) * page_size);
		printf("Received data %s\n", recv_message);
		first++;
	      }
	   */
	    printf("For which page? (0-%ld, or -1 for all): ", recv_data.pg_size-1);
	    scanf("%ld", &accessvar);
	    if (accessvar < recv_data.pg_size || accessvar == -1)
	      {
		//mem_read(access);
		printf("Here\n");
		int l = 0x0;
		int j = 0x0;
		int k = 0x0;
		char* c;
		printf("Reading page %ld\n", accessvar);
		if (accessvar == -1)
		  {
		    if (order == 2)
		      {
			printf("Received data %s\n", recv_message);
			while (k < recv_data.length)
			  {
			    memset(recv_data.addr+k, 0, sizeof(page_size));
			    k += page_size;
			  }
			while( j < recv_data.length)
			  {
			    memcpy(recv_data.addr+j, recv_message, strlen(recv_message));
			    j += page_size;
			  }
		      }
		    printf("Reading all pages\n");
		    while (l < recv_data.length) {
		      c = &recv_data.addr[l];
		      //printf("Read address %p: \n", data.addr + l);
		      printf("Reading the string: %s\n", c);
		      l += page_size;
		    }

		  }
		else
		  {
		    l += (accessvar*page_size);
		    if ( order == 2)
		      {
			memset(recv_data.addr+l, 0, sizeof(page_size));
			if (recv_message != NULL)
			  {
			    memcpy(recv_data.addr+l, recv_message, strlen(recv_message));
			  }
		      }		   
		    c = &recv_data.addr[l];
		    //printf("Read address %p: \n", data.addr + l);
		    printf("Reading the string: %s\n", c);

		  }
	      }  
	    else
	      {
		printf("Can't read that page\n");
	      }
	  }
	else if (commands == 'w')
	  {
	    printf("For which page? (0-%ld, or -1 for all): ", recv_data.pg_size-1);
	    scanf("%ld", &accessvar);
	    if (accessvar < recv_data.pg_size || accessvar == -1)
	      {
		while ((getchar()) != '\n');
		printf("Enter your message: ");
		scanf("%[^\n]s", message);
		//mem_write(access, message, recv_data);
		int l = 0x0;
		int j = 0x0;
		if (accessvar == -1)
		  {
		    while (l < recv_data.length)
		      {
			memset(recv_data.addr+l, 0, sizeof(page_size));
			l += page_size;
		      }
		    while( j < recv_data.length)
		      {
			memcpy(recv_data.addr+j, message, strlen(message));
			j += page_size;
		      }
		    if (order == 2)
		      {
			write(new_socket, message, sizeof(char)*page_size);
		      }
		  }
		else
		  {
		    printf("Here with %ld\n", accessvar);
		    /*
		      msiList[access].I = false;
		      msiList[access].M = true;
		    */
		    printf("Read address %p: \n", recv_data.addr + l);
		    l += (accessvar*page_size);
		    memset(recv_data.addr+l, 0, sizeof(page_size));
		    memcpy(recv_data.addr+l, message, strlen(message));
		    if (order == 2)
		      {
			write(new_socket, message, sizeof(char)*page_size);
		      }
		  }
	      }
	    
	    else
	      {
		printf("Can't write to that page\n");
	      }
	  }
	else if(commands == 'v')
	  {
	    int start = 0;
	    while (start < recv_data.pg_size)
	      {
		start++;
	      }
	  }
	while ((getchar()) != '\n');	
      } while(commands != 'q');

      free(message);
      if(commands == 'q')
	{
	  exit(1);
	}
      /*-----------menu ends--------*/

    }
  
 
  return 0;
}

