#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 5984
#define BUFF_SIZE 4096

#pragma pack(1)
typedef struct st{
  int i;
  char *j;
} stuff;

#pragma pack(0)

int main(int argc, const char *argv[])
{
	int sock = 0;
	struct sockaddr_in serv_addr;
	char *hello = "Hello from client";
	//	char buffer[BUFF_SIZE] = {0};
	stuff test2;
	
	/* [C1: point 1]
	 * if condition will check whether socket is properly created, as if socket() returns
	 * a negative file descriptor, this means socket was failed to be initialized and return
	 * -1. 
	 * AF_INET and SOCK_STREAM are constants defined in the sys/socket.h 
	 * AF_INET = the address domain of the socket (unix domain for processes/ internet domain)
	 * SOCK_STREAM = type of socket (stream socket / datagram socket)
	 */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	/* [C2: point 1]
	 * memset() in this case is used to fill a block of memory (size of server address)
	 * with the value of '0' in form of initalization.
	 * sin_family and sin_port are initialized to its respective value (the address of the 
	 * domain and PORT number). the key thing here is the htons() function which keeps the 
	 * order with respect to network byte order
	 */
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	/* [C3: point 1]
	 * inet_pton reads "127.0.0.1" and see if this can be converted to a human readable IP
	 * address. If this fails, then it will return -1 along with the error message.
	 */
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	/* [C4: point 1]
	 * given C1 properly worked (and successfully created sock variable), connect() function
	 * is used to connect that socket to the server address. sizeof(serv_addr) is used to 
	 * ensure the proper sizing of what input is going into the function, and if this fails
	 * it will return -1 to denote the error.
	 */
	while (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("\nConnection Failed \n");
		sleep(1);
		//return -1;
	}


	/* [C5: point 1]
	 * This function is simply reading the newline from the input buffer as the form of
	 * acknowledgement by the user to continue this operation
	 */
	printf("Press any key to continue...\n");
	getchar();

	/* [C6: point 1]
	 * Given that C4 worked properly and now socket is connect()ed, we can now use send()
	 * function to send data to a socket sock. sendto() need to be used if the type of socket
	 * is different. strlen() needs to be used here in order to properly set the length of data
	 * and flag 0 is set.
	 */
	send(sock , hello , strlen(hello) , 0 );
	printf("Hello message sent\n");

	/* [C7: point 1]
	 * Similar to C6, we are now reading the data coming from the connected sock (server in 
	 * this case), and storing them into the buffer with size of 1024 bytes length.
	 */
	//	read( sock , buffer, 1024);
	recv(sock,&test2, sizeof(stuff), MSG_WAITALL);
	printf("Message from a server: %s\n", test2.j );
	return 0;
}
