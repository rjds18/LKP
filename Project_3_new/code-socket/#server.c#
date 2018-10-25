#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#define PORT 5984
#define BUFF_SIZE 4096

#pragma pack(1)
typedef struct {
  int i;
  char *j = "hello";
} stuff;
#pragma pack(0)

int main(int argc, const char *argv[])
{
	int server_fd, new_socket;
	// server file descriptor / new socket
	
	struct sockaddr_in address;
	// this struct is used to specify a local / remote endpoint address
	// to which to connect a socket
	
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[BUFF_SIZE] = {0};
	//	char *hello = "Hello from server";
	stuff *test = malloc(sizeof(stuff));

	/* [S1: point 1]
	 * Although the file desciptor of 0 is valid, in this case if we get 0, we must
	 * return error since this means that std input is not using the file descriptor zero
	 * which could be unintentionally done by a programmer and lead to serious problems.
	 */
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	/* [S2: point 1]
	 * In this case we are just configuring options for a socket we created in the S1. 
	 * By using SOL_SOCKET variable, we are letting this item be searched in the created
	 * socket itself. REUSEADDR | REUSEPORT may seem redundant, this is not true. REUSEADDR
	 * allows the server to bind() with address which is in TIME_WAIT state (final stage of TCP
	 * state) while REUSEPORT allows multiple addresses to bind() with the same address given
	 * all of these addresses use the SO_REUSEPORT option.
	 */
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
		       &opt, sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	/* [S3: point 1]
	 * This is similar to the client side where we are adding AF_INET (the address of the 
	 * domain) and PORT number, but the new option set here is the sin_addr.s_addr which is
	 * set to INADDR_ANY. When INADDR_ANY is specified, the socket will be bound to all
         * local interfaces. We want to do this as we don't want server to be only binding to
	 * the localhost interface {hence, if we wish to do this, it will be inet_addr("localhost")
	 * rather than INADDR_ANY}. Also this instructs the server to automatically fill the IP
	 * of the server which a process is running on
	 * 
	 */
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

	/* [S4: point 1]
	 * as the name suggests, bind() will bind the socket described in server_fd to the
	 * address (which is a data structure containing the information regarding your IP add +
	 * port number).
	 */
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	/* [S5: point 1]
	 * listen() function marks the socket server_fd as a passive socket (to receive incoming
	 * connection requests). 3 is inserted there in order to create the boundary in case of
	 * massive connection requests. If a request comes when the queue is full, then the client
	 * who is requesting the connection will receive the error message of ECONNREFUSED or 
	 * other mitigation options.
	 */
	if (listen(server_fd, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	/* [S6: point 1]
	 * accept() function follows the order of listen() function by extracting the waiting queue
	 * and this creates a new "connected" socket which will generate the NEW file descriptor
	 * that will be stored to a variable (in this case new_socket). The reason why this 
	 * variable is needed is because socket is just like a file. Each client connection operate
	 * on its own respective socket. 
	 */
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
				 (socklen_t*)&addrlen)) < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}

	/* [S7: point 1]
	 * This function is simply reading the newline from the input buffer as the form of 
	 * acknowledgement by the user to continue this operation.
	 */
	printf("Press any key to continue...\n");
	getchar();

	/* [S8: point 1]
	 * we will be using the new_socket (which is created by the accept() function given that
	 * the client is legit), to receive any data and store them into the buffer. Then use
	 * printf to call that received data to output.
	 */
	read( new_socket , buffer, 1024);
	printf("Message from a client: %s\n",buffer );

	/* [S9: point 1]
	 * we will now be sending data TO the client (which its file descriptor is described in the
	 * new_socket variable) with the message "hello" with its respective size. 
	 */
	test->j = "hello";
	test->i = 2;
	send(new_socket , test, sizeof(stuff) , 0 );
	//	sendto(new_socket, &pkt,sizeof(struct stuff),0
	printf("Hello message sent\n");
	return 0;
}
