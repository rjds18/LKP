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
struct sockaddr_in serv_addr;
struct sockaddr_in serv_addr_orig;

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
  int cur_var;
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

size_t total;

char *serv_num;
char *cli_num;

bus_info bus_signal;
bus_info recv_signal;

unsigned long accessvar;

pthread_t fd_thread;

MSIList states_list[1024];

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

void* update_bus(ssize_t flag, bus_info sigs)
{
  buffer = malloc(sizeof(char)*page_size);

  if (order == 1 && sigs.BusRd == 1)
    {
      printf("Mch #1 needs to update the bus\n");
      
    }
  else if (order == 1 && sigs.BusRdX == 1)
    {
      printf("Mch #1 needs to update the bus\n");
    }
  else if (order == 1 && sigs.BusUpgr == 1)
    {
      printf("Mch #1 needs to check the bus\n");
    }
  else if (order == 2 && sigs.BusRd == 1)
    {
      printf("Mch #2 needs to update the bus\n");
    }
  else if (order == 2 && sigs.BusRdX == 1)
    {
      printf("Mch #2 needs to update the bus\n");
    }
  else if (order == 2 && sigs.BusUpgr == 1)
    {
      printf("Mch #2 needs to check the bus\n");
    }
  else if (accessvar != -1)
    {
      if (order == 1)
	{
	  recv(new_socket, &recv_signal, sizeof(recv_signal), MSG_WAITALL);
	}
      else if (order == 2)
	{
	  recv(client_fd, &recv_signal, sizeof(recv_signal), MSG_WAITALL);
	}
    }
  return 0;
}


void* fault_handler_thread(void *arg)
{
  static struct uffd_msg msg;   /* Data read from userfaultfd */
  long uffd;                    /* userfaultfd file descriptor */
  char *page = NULL;
  struct uffdio_copy uffdio_copy;

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
    nread = read(uffd, &msg, sizeof(msg));
    printf("Here before update\n");
    update_bus(nread, bus_signal);
    if (nread == 0) {
      printf("EOF on userfaultfd!\n");
      exit(EXIT_FAILURE);
    }
		  
    printf("\n[x] PAGEFAULT %ld for the page %ld and BusRd %d\n\n", nread, accessvar, bus_signal.BusRd);

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
  struct sockaddr_in cli_addr; 
  int opt = 1;
  int port = atoi(portnum);
  char *addr;
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
  memset(&serv_addr_orig, 0, sizeof(serv_addr));
  serv_addr_orig.sin_family = AF_INET;
  serv_addr_orig.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr_orig.sin_port = htons( port );
  
  if (bind(server_fd, (struct sockaddr *)&serv_addr_orig, sizeof(serv_addr_orig)) < 0) {
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
	  total = data.pg_size;
	  unsigned long len;  /* Length of region handled by userfaultfd */
	  printf("\nInvoking userfaultfd initialization\n");
 
	  page_size = sysconf(_SC_PAGE_SIZE);
	  len = data.pg_size * page_size;
	  data.length = len;
	  addr = mmap(NULL, len, PROT_READ | PROT_WRITE,
			   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	  data.addr = addr;
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

	  printf("\nSending to the mch #2 mmap addr = %p pg_cnt = %ld\n", addr, data.pg_size);

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
	  int initval = 0;
	  while (initval < data.pg_size)
	    {
	      states_list[initval].M = false;
	      states_list[initval].S = false;
	      states_list[initval].I = true;
	      initval++;
	    }
	  do  {	    
	    sleep(0.5);
	    printf("\nPlease enter the command < Read = r | Write = w | View = v | Quit = q > : ");
	    while ((getchar()) != '\n');
	    scanf("%c", &commands);
	    if(commands == 'r')
	      {
		/*if pagefault occurs here, then I generate BusRd signal*/
		/*BusRd signal will then try to recv message from the shared memory*/
		printf("For which page? (0-%ld, or -1 for all): ", data.pg_size-1);
		scanf("%ld", &accessvar);
		bus_signal.cur_var = accessvar;
		if (accessvar < data.pg_size || accessvar == -1)
		  {
		    int l = 0x0;
		    int j = 0x0;
		    int k = 0x0;
		    char c;
		    printf("Reading page %ld %d %d\n", accessvar, states_list[accessvar].S, states_list[accessvar].M);
		    if(states_list[accessvar].S == true || states_list[accessvar].M == true)
		      {
			printf("Check local copy\n");
			l += (accessvar*page_size);
			c = addr[l];
			printf("Reading the string: %c\n", c);			    
		      }
		    else if (states_list[accessvar].I == true)
		      {
			printf("Invalid state\n");
			bus_signal.BusRd = 1;
			printf("Trying to read invalid page, generate BusRd signal = %d\n", bus_signal.BusRd);
			j += (accessvar*page_size);
			c = addr[j];
			printf("Reading the string: %c\n", c);
			/*reads data from the client
			  if data doesn't exist, then just return empty
			  and keep it shared
			*/			
		      }
		    else if (accessvar == -1)
		      {
			printf("Reading all pages\n");
			int count = 0;
			while (count < data.pg_size)
			  {
			    states_list[count].M = false;
			    states_list[count].S = true;
			    states_list[count].I = false;
			    count++;
			  }
			while (k < data.length) {
			  c = addr[k];
			  printf("Reading the string: %c\n", c);
			  k += page_size;
			}
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
		if (accessvar < data.pg_size || accessvar == -1)
		  {
		    int l = 0x0;
		    int j = 0x0;
		    if (accessvar == -1)
		      {
			while (l < data.length)
			  {
			    memset(addr+l, 0, sizeof(page_size));
			    l += page_size;
			  }
			while( j < data.length)
			  {
			    memcpy(addr+j, message, strlen(message));
			    j += page_size;
			  }
		      }
		    else
		      {
			printf("Here with %ld\n", accessvar);
			printf("Read address %p: \n", addr + l);
			l += (accessvar*page_size);
			memset(addr+l, 0, sizeof(page_size));
			memcpy(addr+l, message, strlen(message));
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
		    printf("Page #%d M:%d S:%d I:%d\n", start, states_list[start].M, states_list[start].S, states_list[start].I);
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
	  while ((getchar()) != '\n');
	}
    }

  return 0;
}

void *client_socket(void *portnum)
{

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
      int initval = 0;
      while (initval < recv_data.pg_size)
	{
	  states_list[initval].M = false;
	  states_list[initval].S = false;
	  states_list[initval].I = true;
	  initval++;
	}
      
      do  {
	//	
	sleep(0.5);
	printf("Please enter the command < Read = r | Write = w | View = v | Quit = q > : ");
	scanf("%c", &commands);

	//	if(strcmp(commands,"r") == 0)
	
	if(commands == 'r')
	  {
	    printf("For which page? (0-%ld, or -1 for all): ", recv_data.pg_size-1);
	    scanf("%ld", &accessvar);
	    if (accessvar < recv_data.pg_size || accessvar == -1)
	      {
		//mem_read(access);
		printf("Here\n");
		int l = 0x0;
		char c;
		printf("Reading page %ld\n", accessvar);
		if (accessvar == -1)
		  {
		    printf("Reading all pages\n\n\n");
		    int count = 0;
		    while (count < recv_data.pg_size)
		      {
			states_list[count].S = true;
			states_list[count].I = false;			    
			count++;
		      }
		    
		    while (l < recv_data.length) {
		      c = recv_addr[l];
		      //printf("Read address %p: \n", addr + l);
		      printf("Reading the string: %c\n", c);
		      l += page_size;
		    }
		  }
		else
		  {
		    states_list[accessvar].S = true;
		    states_list[accessvar].I = false;			    
		    l += (accessvar*page_size);
		    c = recv_addr[l];
		    //printf("Read address %p: \n", addr + l);
		    printf("Reading the string: %c\n", c);
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
		    /*
		    while (l < recv_data.length)
		      {
			memset(recv_addr+l, 0, sizeof(page_size));
			l += page_size;
		      }
		    */
		    while( j < recv_data.length)
		      {
			memcpy(recv_addr+j, message, strlen(message));
			j += page_size;
		      }
		  }
		else
		  {
		    printf("Here with %ld\n", accessvar);
		    printf("Read address %p: \n", recv_addr + l);
		    l += (accessvar*page_size);
		    memset(recv_addr+l, 0, sizeof(page_size));
		    memcpy(recv_addr+l, message, strlen(message));
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
		printf("Page #%d M:%d S:%d I:%d\n", start, states_list[start].M, states_list[start].S, states_list[start].I);
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

