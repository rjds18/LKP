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

//variables
char* serv_num; //server port from the cmd line
char* cli_num;  //client port from the cmd line
int mch_num;    //machine number (1 | 2);
char* addr;     //mmap address setting globally to easily share
char commands;  //this will be used for menu() - only used in the client
char *recv_message; //received message will be stored here;

int pf_chk;
pthread_t pf_thread; //pagefault thread for both mch 1 and mch 2
long init_uffd; //uffd which will be used for the pagefault thread

int server_fd, new_socket;  //server file descriptor / new-socket
int client_fd;

static int page_size; //page size which will be used for pagefault
struct uffdio_api uffdio_api;
struct uffdio_register uffdio_register;

typedef struct {
  bool M;
  bool S;
  bool I;
} MSIList;

typedef struct {
  char *addr;
  long page_num;
  ssize_t page_count;
  int read_signal[MAX_SIZE];
  int pagefault_signal[MAX_SIZE];
  int BusRdx[MAX_SIZE];
  int invalidate_signal[MAX_SIZE];
  int BusUpgr[MAX_SIZE];
  int flush[MAX_SIZE];
  long access_page;
} proc_info;

MSIList list_MSI[MAX_SIZE];

void *server_socket();
void *client_socket();
void *menu();
void *fault_handler_thread();
proc_info temp_data; //this is temp data which mch 2 will use to initialize its own addr for mmap



int main(int argc, char *argv[])
{
  pthread_t serv_thread, cli_thread;
  int serv_chk, cli_chk;
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
  pthread_join( cli_thread, NULL); pthread_join( serv_thread, NULL); 
}
//FAULT
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
  //memset(page, '\0', page_size);
  for (;;) {

    struct pollfd pollfd;
    int nready;
    pollfd.fd = uffd;
    pollfd.events = POLLIN;

    nready = poll(&pollfd, 1, -1);
    if (nready == -1)
      errExit("poll");
    if(temp_data.pagefault_signal[temp_data.page_num] == 1)
      {//printf("test: %d\n", temp_data.pagefault_signal[temp_data.page_num]);
	memset(page, '\0', page_size);  }
    else
      { printf("[x] PAGEFAULT\n\n");   }
    nread = read(uffd, &msg, sizeof(msg));
    if (nread == 0) {
      printf("EOF on userfaultfd!\n");
      exit(EXIT_FAILURE);           }
    if (nread == -1)
      errExit("read");
    if (msg.event != UFFD_EVENT_PAGEFAULT) {
      fprintf(stderr, "Unexpected event on userfaultfd\n");
      exit(EXIT_FAILURE);          }
    uffdio_copy.src = (unsigned long) page;
    uffdio_copy.dst = (unsigned long) msg.arg.pagefault.address &
      ~(temp_data.page_count - 1);
    uffdio_copy.len = page_size;
    uffdio_copy.mode = 0;
    uffdio_copy.copy = 0;
    if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy) == -1)
      errExit("ioctl-UFFDIO_COPY");
  }
}
//MENU
void *menu()
{
  char* reading = malloc(sizeof(char)*page_size);
  char* message = malloc(sizeof(char)*page_size);           //message allocation
  recv_message = malloc(sizeof(char) *page_size);
  long base_addr = 0x0; //base addr used to see which page is read
  int index = -2;            //index number of MSI list
  do {
    if (mch_num == 1) { while((getchar()) != '\n'); }
    printf("Please enter the command < Read = r | Write = w | View = v | Quit = q > : ");
    scanf("%c", &commands);
    if (commands == 'r')
      {
	printf("For which page? (0-%ld, or -1 for all): ", temp_data.page_count-1);
	scanf("%ld", &temp_data.page_num);
	index = temp_data.page_num;
	if (index == -1)
	  {
	    printf("Reading all pages\n");
	    int count;
	    for(count = 0; count < temp_data.page_count; count++)
	      {
		printf("Page %d\n", count);
		temp_data.page_num = count;
		temp_data.access_page = base_addr + (count * page_size);
		//printf("Current M:%d S:%d I:%d\n", list_MSI[count].M, list_MSI[count].S, list_MSI[count].I);
		if (list_MSI[count].I == true) // need to see whether this is invalid)
		  {
		    memset(addr + temp_data.access_page, '\0', page_size);
		    temp_data.read_signal[count] = 1 ;       //request to read from the other machine
		    temp_data.pagefault_signal[count] = 1;   //also tell that other machine that page fault occured at this page
		    temp_data.invalidate_signal[count] = 0;
		    if(temp_data.read_signal[count] == 1)
		      {
			//printf("Sending read signal + transitioning to the shared state\n");
			list_MSI[temp_data.page_num].I = false; list_MSI[temp_data.page_num].S = true; list_MSI[temp_data.page_num].M = false;
		      }	
		    send(client_fd, &temp_data, sizeof(proc_info), 0);
		    //recv_message = '\0';
		    read(client_fd, recv_message, sizeof(char)*page_size);
		    temp_data.read_signal[count] = 0;
		    printf("Reading the page %s\n", recv_message);
		  }
		else if(list_MSI[count].S == true || list_MSI[count].M == true)
		  {
		    reading = &addr[temp_data.access_page];
		    printf("Reading the page %s\n", reading);
		  }
	      }
	  }
	else
	  {
	    temp_data.access_page = base_addr + (temp_data.page_num * page_size);
	    //printf("Current MSI: %d %d %d\n", list_MSI[index].M, list_MSI[index].S, list_MSI[index].I);
	    if (list_MSI[index].I == true) // need to see whether this is invalid)
	      {
		memset(addr + temp_data.access_page, '\0', page_size);
		temp_data.read_signal[index] = 1 ;       //request to read from the other machine
		temp_data.pagefault_signal[index] = 1;   //also tell that other machine that page fault occured at this page
		temp_data.invalidate_signal[index] = 0;
		if(temp_data.read_signal[index] == 1)
		  {
		    //printf("Sending read signal + transitioning to the shared state\n");
		    list_MSI[temp_data.page_num].I = false; list_MSI[temp_data.page_num].S = true; list_MSI[temp_data.page_num].M = false;
		  }	
		 send(client_fd, &temp_data, sizeof(proc_info), 0);
		 //recv_message = '\0';
		 read(client_fd, recv_message, sizeof(char)*page_size);
		 temp_data.read_signal[index] = 0;
		 printf("Reading the page %s\n", recv_message);
	      }
	    else if(list_MSI[index].S == true || list_MSI[index].M == true)
	      {
		reading = &addr[temp_data.access_page];
		printf("Reading the page %s\n", reading);
	      }
	  }      
      }
    else if (commands == 'w')
      {
	printf("For which page? (0-%ld, or -1 for all): ", temp_data.page_count-1);
	scanf("%ld", &temp_data.page_num);
	index = temp_data.page_num;
	if (index == -1)
	  {
	    while((getchar()) != '\n');
	    printf("Enter your message: ");
	    scanf("%[^\n]s", message);
	    printf("Writing to all pages\n");
	    int count;
	    for (count = 0; count < temp_data.page_count; count++)
	      {
		temp_data.page_num = count;
		if(list_MSI[count].I == true)
		  {	temp_data.pagefault_signal[count] = 1;
		    temp_data.BusRdx[count] = 1;            //Turning on data exist signal
		    temp_data.invalidate_signal[count] = 1; //Invalid this page for the other application.
		    temp_data.read_signal[count] = 0;       //Turning off read signal
		  }
		else if (list_MSI[count].S == true)
		  {	temp_data.BusUpgr[count] = 1;
		    temp_data.invalidate_signal[count] = 1; //Invalid this page for the other application.
		    temp_data.read_signal[count] = 0;       //Turning off read signal
		  }
		//printf("checking my state to modified\n");
		list_MSI[count].M = true; 	list_MSI[count].I = false;  list_MSI[count].S = false;//changing MSI
		temp_data.access_page = base_addr + (temp_data.page_num * page_size);
		//while((getchar()) != '\n');
		memcpy(addr+temp_data.access_page, message, strlen(message));
		//printf("Writing %s\n", &addr[temp_data.access_page]);
		send(client_fd, &temp_data, sizeof(proc_info), 0);
	      }
	  }
	else
	  {
	    if(list_MSI[index].I == true)
	      {	temp_data.pagefault_signal[index] = 1;
		temp_data.BusRdx[index] = 1;            //Turning on data exist signal
		temp_data.invalidate_signal[index] = 1; //Invalid this page for the other application.
		temp_data.read_signal[index] = 0;       //Turning off read signal
	      }
	    else if (list_MSI[index].S == true)
	      {	temp_data.BusUpgr[index] = 1;
		temp_data.invalidate_signal[index] = 1; //Invalid this page for the other application.
		temp_data.read_signal[index] = 0;       //Turning off read signal
	      }
	    printf("checking my state to modified\n");
	    list_MSI[index].M = true; 	list_MSI[index].I = false;  list_MSI[index].S = false;//changing MSI
	    temp_data.access_page = base_addr + (temp_data.page_num * page_size);
	    while((getchar()) != '\n');
	    printf("Enter your message: ");
	    scanf("%[^\n]s", message);
	    //memset(addr+temp_data.access_page, '\0', 0);
	    memcpy(addr+temp_data.access_page, message, strlen(message));
	    printf("Writing %s\n", &addr[temp_data.access_page]);
	    send(client_fd, &temp_data, sizeof(proc_info), 0);	  	    
	  }
      }
    else if (commands == 'v')
      {
	printf("For which page? (0-%ld, or -1 for all): ",temp_data.page_count-1);
	scanf("%ld", &temp_data.page_num);
	index = temp_data.page_num;
	if (index == -1)
	  {
	    printf("Viewing all pages %ld\n", temp_data.page_count);
	    int start = 0;
	    while (start < temp_data.page_count)
	      {
		printf("Page #%d M:%d S:%d I:%d\n", start, list_MSI[start].M, list_MSI[start].S, list_MSI[start].I);
		start++;	  
	      }
	  }
	
      }  if (mch_num == 2) { while((getchar()) != '\n'); }
  } while (commands != 'q');
  if (commands == 'q')
    {  exit(1);    }

  return 0;
}

//SERVER
void *server_socket(void *portnum)
{
    proc_info recv_data;        //data which will be received from the client
    char* flush = malloc(sizeof(char)*page_size);
    struct sockaddr_in address; //specifying address to connect a socket
    int port = atoi(portnum);
    int opt = 1;
    int addrlen = sizeof(address);
  
    printf("Server Socket with port = %d\n", port);  
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
      perror("socket failed");
      exit(EXIT_FAILURE);
    } // create socket for server_fd
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
		   &opt, sizeof(opt))) {
      perror("setsockopt");
      exit(EXIT_FAILURE);
    } // set socket option for server_fd
  
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons( port );
  
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {    perror("bind failed");
      exit(1);  } // binding socket described in server_fd to the addr
  
  if (listen(server_fd, 3) < 0) {    perror("listen");
    exit(1);  } // marks the socket server_fd as a passive socket which will listen for client
  
  printf("Trying to connect ... \n");
  
  if( (new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) )
    {      printf("Connection accepted\n");        }
  if (mch_num == 2)
    {
      // first time listening to the machine 1 for the info regarding page_size + mmap
      printf("waiting for the initial data from the mch #1\n");
      read(new_socket, &recv_data, sizeof(proc_info));
      temp_data = recv_data;
      /* after receiving the data, this is where magic will begin with pagefault */
      page_size = sysconf(_SC_PAGE_SIZE);
      unsigned long len = recv_data.page_count * page_size;
      addr = mmap(recv_data.addr, len, PROT_READ | PROT_WRITE,
		  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      printf("mmap generated address %p\n", addr);
      int i = 0;
      printf("Here 2\n");
      while (i < temp_data.page_count)
	{ list_MSI[i].M = false; 
	  list_MSI[i].S = false;
	  list_MSI[i].I = true;
	  i++;     }
      init_uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
      if (init_uffd == -1)
	errExit("userfaultfd");
      uffdio_api.api = UFFD_API;
      uffdio_api.features = 0;
      if (ioctl(init_uffd, UFFDIO_API, &uffdio_api) == -1)
	errExit("ioctl-UFFDIO_API");
      uffdio_register.range.start = (unsigned long) addr;
      uffdio_register.range.len = len;
      uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
      if (ioctl(init_uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
	errExit("ioctl-UFFDIO_REGISTER");
    }  
  for (;;)
    {
      int check;
      
      if ((check = read(new_socket, &recv_data , sizeof(proc_info)) <= 0))
	{ printf("Client disconnected, exiting...\n");	  exit(1); }
      temp_data = recv_data;	
      //      printf("\n\nReceived data as following: \n Address %p \n Page Num: %ld \n Page Count %ld \n Read Signal %d \n Pagefault Signal %d \n Invalidate Signal %d \n BusRdx Signal %d \n", addr, recv_data.page_num, recv_data.page_count, recv_data.read_signal[recv_data.page_num], recv_data.pagefault_signal[recv_data.page_num], recv_data.invalidate_signal[recv_data.page_num], recv_data.BusRdx[recv_data.page_num]);
      //printf("Current MSI: %d %d %d\n", list_MSI[recv_data.page_num].M, list_MSI[recv_data.page_num].S, list_MSI[recv_data.page_num].I);
      
      if(list_MSI[recv_data.page_num].M == true && (/*recv_data.BusRdx[recv_data.page_num] == 1 ||*/ recv_data.read_signal[recv_data.page_num] == 1) )
	{
	      flush = &addr[temp_data.access_page];
	      send(new_socket, flush, strlen(flush), 0);
	      list_MSI[recv_data.page_num].M = false; 	  list_MSI[recv_data.page_num].S = true; 	  list_MSI[recv_data.page_num].I = false;
	   
	}     
      else if(list_MSI[recv_data.page_num].S == true && (recv_data.BusUpgr[recv_data.page_num] == 1 || recv_data.BusRdx[recv_data.page_num] == 1) )
	{
	  //	  printf("Here\n");
	  list_MSI[recv_data.page_num].M = false; 	  list_MSI[recv_data.page_num].S = false; 	  list_MSI[recv_data.page_num].I = true;
	}
      else if(list_MSI[recv_data.page_num].I == true && recv_data.read_signal[recv_data.page_num] == 1)
	{
	  //getting here means that there is no data since current state for this page is invalid.
	  list_MSI[recv_data.page_num].M = false; 	  list_MSI[recv_data.page_num].S = true; 	  list_MSI[recv_data.page_num].I = false;
	  //printf("Sending no data\n");
	  char* no_data = " ";
	  send(new_socket, no_data, strlen(no_data), 0);
	}
      else if( (list_MSI[recv_data.page_num].M == true || list_MSI[recv_data.page_num].S == true) && (recv_data.read_signal[recv_data.page_num] == 0 || recv_data.invalidate_signal[recv_data.page_num] == 1) )
	{
	  list_MSI[recv_data.page_num].M = false; 	  list_MSI[recv_data.page_num].S = false; 	  list_MSI[recv_data.page_num].I = true;
	}
    }
  /*-----------------need to add decoder to deal with received data-----------*/
  return 0;
}

void *client_socket(void *portnum)
{
  struct sockaddr_in serv_addr;
  proc_info send_data; //this will primarily be used from client to send data to the other serv
  int port = atoi(portnum);
  /*------------------setting up sockets----------------------*/
  if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {    perror("socket failed");
    exit(EXIT_FAILURE);  }
  printf("\nClient Socket with port = %d\n", port);
  serv_addr.sin_family = AF_INET;
  if(inet_aton("127.0.0.1", &serv_addr.sin_addr) <= 0)
    {      printf("\nInvalid Address | Address not supported \n");
      exit(1);    }
  serv_addr.sin_port = htons(port);
  if ((connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {      //checks first for the connection, if it reaches this state, that means mch = 1;
      //otherwise, it will be mch_num = 2
      mch_num = 1;
      while ((connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) {
          printf("\nConnection Failed \n");
          sleep(1); } }
  else
    {      mch_num = 2;   }

  if(mch_num == 1) //this is the first time initialization of mmap for #1 and #2. 
    {
      printf("Enter the page count: ");
      scanf("%lu", &send_data.page_count);
      temp_data.page_count = send_data.page_count; //keeping temp updated as well
      page_size = sysconf(_SC_PAGE_SIZE);
      unsigned long len = send_data.page_count * page_size;
      addr = mmap(NULL, len, PROT_READ | PROT_WRITE,
		  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      send_data.addr = addr;
      if (send_data.addr == MAP_FAILED)
	{ printf("Failed\n"); errExit("mmap"); }
      printf("mmap generated address %p \n", addr);
      int i = 0;
      printf("Here 1\n");
      while (i < temp_data.page_count)
	{ list_MSI[i].M = false; 
	  list_MSI[i].S = false;
	  list_MSI[i].I = true;
	  i++;     }
      send(client_fd, &send_data, sizeof(proc_info), 0);
      init_uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
      if (init_uffd == -1)
	errExit("userfaultfd");
      uffdio_api.api = UFFD_API;
      uffdio_api.features = 0;
      if (ioctl(init_uffd, UFFDIO_API, &uffdio_api) == -1)
	errExit("ioctl-UFFDIO_API");
      uffdio_register.range.start = (unsigned long) addr;
      uffdio_register.range.len = len;
      uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
      if (ioctl(init_uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
	errExit("ioctl-UFFDIO_REGISTER");
    }
  else
    {  do { /*waiting*/ printf("waiting\n"); sleep(1); }while(addr == NULL); }  
  /*--------------------setting up menu----------------------*/
  pf_chk = pthread_create(&pf_thread, NULL, fault_handler_thread, (void *) init_uffd);
  if(pf_chk != 0)
    {      errno = pf_chk; errExit("pthread_create");     }
  menu();  
  return 0;
}

