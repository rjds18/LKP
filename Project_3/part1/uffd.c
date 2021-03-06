/* userfaultfd_demo.c

   Licensed under the GNU General Public License version 2 or later.
*/
#define _GNU_SOURCE
#include <sys/types.h>
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
#include <poll.h>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE);	\
	} while (0)

static int page_size;

static void *
fault_handler_thread(void *arg)
{
	static struct uffd_msg msg;   /* Data read from userfaultfd */
	static int fault_cnt = 0;     /* Number of faults so far handled */
	long uffd;                    /* userfaultfd file descriptor */
	static char *page = NULL;
	struct uffdio_copy uffdio_copy;
	ssize_t nread;

	uffd = (long) arg;

	/* [H1: point 1]
	 In this if function, it first checks whether the "page" pointer is NULL (meaning empty). 
	 If it is indeed empty then we use mmap function which creates a new mapping in the 
	 virtual address space of the calling process.
	 
	 NULL will be provided for the starting address as this allows kernel to choose 
	 natural address 
	 
	 page_size variable (instantiated above) will be provided for its mapping length | 
	 (PROT_* and MAP_* ) are just flags to provide the memory protection info

	 -1 is for its file descriptor as this will be changed later on |
	 and 0 for the offset since we are in the initialization phase for the particular flag 
	 MAP_ANONYMOUS which is required for some implementations. It is important to clarify what 
	 ANONYMOUS means in this case. This doesn't mean that the file is mapped anonymously, but 
	 more like the file ITSELF is anonymous (hence there isn't any files that are specified)
	 
	 nested if command is for when things go wrong and we need to error while providing 
	 the info that "mmap" is the culprit. 
	 */
	if (page == NULL) {
		page = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
			    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (page == MAP_FAILED)
			errExit("mmap");
	}

	/* [H2: point 1]
	   for (;;) is an equivalent way to type while(true). The reason why you would wish to use 
	   this is to avoid a type conversion problem.
	 */
	for (;;) {

		/* See what poll() tells us about the userfaultfd */

		struct pollfd pollfd;
		int nready;

		/* [H3: point 1]
		   1) after creating the struct of a class pollfd called pollfd, we need to 
		   initialize its constructor. In this case, we initialize its member variable of 
		   fd to the userfaultfd fd
		   
		   2) POLLIN in this case is a special case which basically sets the event that 
		   "there is data to read"
		   
		   3) nready I'm guessing stands for "node ready" variable. In this context, we use 
		   poll() function with provided file descriptor struct "pollfd" to find out when 
		   this stuct is ready to perform I/O operation. Hence, we pass in the reference to 
		   the struct point pollfd (which contains the member variables fd / events set 
		   previously) and pass in additional values such as the number of items (which is 
		   1 in this context) and set timeout option to -1 as we don't want poll to block 
		   in this case.
		   
		   4) if poll returns -1 (to denote that we failed), then we need to end this program
		 */
		printf("here\n");
		memset(page, '\0' , page_size);
		pollfd.fd = uffd;
		pollfd.events = POLLIN;
		nready = poll(&pollfd, 1, -1);

		if (nready == -1)
			errExit("poll");


		/*
		printf("\nfault_handler_thread():\n");
		printf("    poll() returns: nready = %d; "
                       "POLLIN = %d; POLLERR = %d\n", nready,
                       (pollfd.revents & POLLIN),
                       (pollfd.revents & POLLERR));
		*/
		//printf("here2\n");
		printf("\nfault_handler_thread():\n");
		printf("    poll() returns: nready = %d; "
                       "POLLIN = %d; POLLERR = %d\n", nready,
                       (pollfd.revents & POLLIN) != 0,
                       (pollfd.revents & POLLERR) != 0);

		/* [H4: point 1]
		   given that the previous events have occurred without any problems, we are ready 
		   to check whether pagefaultfd is ready to be used. this function is simply taking 
		   in the file descriptor (in our case uffd) with message pointer which should only 
		   be available if userfaultfd worked properly. If unable to read, throw an error 
		   and exit.
		 */
		nread = read(uffd, &msg, sizeof(msg));
		//		printf("here3\n");
		if (nread == 0) {
			printf("EOF on userfaultfd!\n");
			exit(EXIT_FAILURE);
		}

		if (nread == -1)
			errExit("read");

		/* [H5: point 1]
		   for the purpose of this project, we need to check whether page fault happen. 
		   Therefore, it page fault DOES NOT happen, then we are going to throw an exit.
		 */
		if (msg.event != UFFD_EVENT_PAGEFAULT) {
			fprintf(stderr, "Unexpected event on userfaultfd\n");
			exit(EXIT_FAILURE);
		}

		/* [H6: point 1]
		   %llx stands for long long hexadecimal and this is simply verifying the page fault 
		   functionality. here we can access flags and address using the struct 
		   instantitated in the userfaultfd.h 
		 */
		printf("    UFFD_EVENT_PAGEFAULT event: ");
		printf("flags = %llx; ", msg.arg.pagefault.flags);
		printf("address = %llx\n", msg.arg.pagefault.address);

		/* [H7: point 1]
		   after we have finished creating a user fault mapping, we will fill in a region 
		   of memory with using memset. Modulo operator used in this case for the byte is 
		   to change content into different numbers?
		 */
		// check.
		
		//memset(page, 'A' + fault_cnt % 20, page_size);
		fault_cnt++;

		/* [H8: point 1]
		   uffdio_copy is a struct also defined in the userfaultfd.h. this is used in order 
		   to copy a certain memory regions into the designated userfault registered range. 
		   after that, it wakes up the blocked thread
		 */
		uffdio_copy.src = (unsigned long) page;
		uffdio_copy.dst = (unsigned long) msg.arg.pagefault.address &
			~(page_size - 1);
		uffdio_copy.len = page_size;
		uffdio_copy.mode = 0;
		uffdio_copy.copy = 0;

		/* [H9: point 1]
		   ioctl is used in order to create a fd which will be utilized from the user space 
		   perspective to handle page faults. Therefore for this if statement, if this is 
		   not possible, then we will simply throw an error and exit out of the program.
		 */
		if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy) == -1)
			errExit("ioctl-UFFDIO_COPY");

		/* [H10: point 1]
		   verifying whether the copy function properly worked.
		 */
		printf("        (uffdio_copy.copy returned %lld)\n",
                       uffdio_copy.copy);
	}
}

int
main(int argc, char *argv[])
{
	long uffd;          /* userfaultfd file descriptor */
	char *addr;         /* Start of region handled by userfaultfd */
	unsigned long len;  /* Length of region handled by userfaultfd */
	pthread_t thr;      /* ID of thread that handles page faults */
	struct uffdio_api uffdio_api;
	struct uffdio_register uffdio_register;
	int s;
	int l;

	/* [M1: point 1]
	   this is checking whether the user is properly providing the input while doing ./uffd 
	   command (this is reinforced by argc not matching ONE arguments besides ./uffd). 
	   Therefore, if you do something like ./uffd 1 3 4 this will be thrown.
	 */
	if (argc != 2) {
		fprintf(stderr, "Usage: %s num-pages\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* [M2: point 1]
	   sysconf is used to get configuration information during the runtime (of a page size in 
	   bytes)
	   using this information, we can further use strtoul to convert our input from the command 
	   line and multiply with page_size while converting it to the unsigned long integer. NULL 
	   denotes the end pointer of the arguments and 0 is simply denoting the base
	 */
	page_size = sysconf(_SC_PAGE_SIZE);
	len = strtoul(argv[1], NULL, 0) * page_size;

	/* [M3: point 1]
	   we will be calling system call from the userspace using syscall() function with the 
	   identifier of __NR_userfaultfd (__NR_SYSCALL_BASE+388); this essnetially is creating 
	   userfaultfd object. Also provide few helpful flags (O_CLOEXEC - close on execution | 
	   file is opened in nonblocking mode when it is possible) to give an additional information.
	 */
	uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
	if (uffd == -1)
		errExit("userfaultfd");

	/* [M4: point 1]
	   this basically "enables" the operation of the userfaultfd. supplying information for this
	   struct gives inthe information of being able to use userfaultfd features (this is used by
	   using the features member variable)
	 */
	uffdio_api.api = UFFD_API;
	uffdio_api.features = 0;
	if (ioctl(uffd, UFFDIO_API, &uffdio_api) == -1)
		errExit("ioctl-UFFDIO_API");

	/* [M5: point 1]
	   this is creating a space that will be used by "fault handler thread" (which will be used 
	   further down during the pthread_create). Similar arguments as previously explained above.
	 */
	addr = mmap(NULL, len, PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (addr == MAP_FAILED)
		errExit("mmap");


	/* [M6: point 1]
	   this is where we register a memory address range with the userfaultfd object. important 
	   thing to notice here is regarding the .mode which is used to track page faults on 
	   missing pages (described on ioctl)
	 */
	uffdio_register.range.start = (unsigned long) addr;
	uffdio_register.range.len = len;
	uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;

	if (ioctl(uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
		errExit("ioctl-UFFDIO_REGISTER");

	/* [M7: point 1]
	   we are finally calling the void function fault_handler_thread described above while 
	   creating a thread which will process that function. If s returns something other than 0, 
	   that means error has occurred.
	 */
	s = pthread_create(&thr, NULL, fault_handler_thread, (void *) uffd);
	if (s != 0) {
		errno = s;
		errExit("pthread_create");
	}

	/*
	 * [U1]: point 5
	 setting l = 0x0 ensures that we will start from the address 0. From here on we will be 
	 jumping through 1024 bytes (since that's how far each data is located) until we reach to 
	 the condition where the current address is larger than the length. This is further shown on
	 the output as each loop iteration increments the address by 400 (hexadecimal) which is 
	 equivalent to 1024 (decimal)
	 
	 */
	
	printf("-----------------------------------------------------\n");
	l = 0x0;
	while (l < len) {
		char* c = &addr[l];
		printf("The value of l %d\n", l);
		printf("#1. Read address %p in main(): ", addr + l);
		printf("%s\n", c);
		l += 1024;
	}
	
	/*
	 * [U2]: point 5
	 although this code looks exactly same as the U1 (which it actually is), interesting thing 
	 occurs when this is run as shown on the output. Because when U1 initially ran, the memory 
	 is empty, we call to the fault handler thread. In this case, since U1 already attempted to 
	 read through empty memory which subseqently called page fault handler to solve this problem
	 , U2 when it repeats the code, it will be able to successfully read through the address. 

	 */
	
	printf("-----------------------------------------------------\n");
	l = 0x0;
	while (l < len) {
		char c = addr[l];
		printf("#2. Read address %p in main(): ", addr + l);
		printf("%c\n", c);
		l += 2048;
	}

	/*
	 * [U3]: point 5
	 remember how i mentioned that on the U2, because U1 already dealt with empty memory? For 
	 this code, we can see that there is an additional function used stated "madvise" which is a
	 system call used to provide kernel with an information about the address range.  On this 
	 particular code, we are using it with the flag MADV_DONTNEED which to summarize free 
	 resources attached with it.

	 However, key thing we need to observe here is that the char c = addr[l]; now changes to B 
	 from A, which is the direct result of freeing up the space using madvise (and expecting we 
	 won't be using this memory address again).
	*/
	/*
	printf("-----------------------------------------------------\n");
	if (madvise(addr, len, MADV_DONTNEED)) {
		errExit("fail to madvise");
	}

	*/
	
	l = 0x0;
	while (l < len) {
		char c = addr[l];
		printf("#3. Read address %p in main(): ", addr + l);
		printf("%c\n", c);
		l += 1024;
	}
	//*/
	/*
	 * [U4]: point 5
	 this is pretty much repeat of the U2 but confirming again that fault handler thread won't 
	 be called in this case due to the same reason as previously explained.
	 */
	/*
	printf("-----------------------------------------------------\n");
	l = 0x0;
	while (l < len) {
		char c = addr[l];
		printf("#4. Read address %p in main(): ", addr + l);
		printf("%c\n", c);
		l += 1024;
	}
	*/
	exit(EXIT_SUCCESS);
}
