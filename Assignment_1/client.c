#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define MAXLINE 100

int writen(char* str, int socketfd);
int write_a_char (char* str, int socketfd, int n);
int readline(int socketfd, char* buffer, int max_line);
char read_a_char(int socketfd, int num, int max_line);

int main(){

    const char* server_name = "hera3.ece.tamu.edu";
    int server_port;
	int c;
    int socketfd;
    int maxline = MAXLINE;
	//char buffer[MAXLINE];
    char input[MAXLINE];
	char output[MAXLINE];
    struct sockaddr_in server_addr;

    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("ERRNO: \t%s\n", strerror(errno));
		return -1;
    }
	printf("Open Socket succeed\n");
    // Input the server port to be connected
    printf("Input the server port to be connected: ");
    scanf("%d", &server_port);	
 
    // Setup the server address
    memset(&server_addr, 0, sizeof(server_addr)); // Set all bits of server address to zero
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_name, &server_addr.sin_addr) < 0) {
        printf("ERRNO: \t%s\n", strerror(errno));
		return -1;
    }
	printf("Server address transform to network format.\n");
    // Connect the socket to the server
    if ((connect(socketfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0) {
        printf("ERRNO: \t%s\n", strerror(errno));
		return -1;
    }
	printf("Connect to socket\n");
    // Input string into the client
    printf("Insert a line into the client\n");
    scanf("%s", input); // Store the input string into input array
	printf("The line read is: %s\n", input);
	
    // Written operation
	if (writen(input, socketfd) < 0) {
		printf("ERRNO: \t%s\n", strerror(errno));
		return -1;
	}
	printf("Send message to the socket successfully\n");
	
	// Read messages from the socket
	if (readline(socketfd, output, maxline) < 0) {
		printf("ERRNO: \t%s\n", strerror(errno));
		return -1;
	}
	printf("Receive message from the socket successfully\n");
	printf("The message is: %s\n", output);
	
	return 0;
}

int writen(char* str, int socketfd) {
	int c;
	int num = strlen(str);
	int count = 0;
	// Write message to socket with one char at one time
	for (int i = 0; i < num; i++) {
		if ( c = write_a_char(str, socketfd, i) < 0) {
			printf("ERRNO: \t%s\n", strerror(errno));
			return -1;
		}
		count++;
	}
	return count;
}

int write_a_char (char* str, int socketfd, int n) {
	int c;
	if (c = write(socketfd, (str + n), 1) < 0) {
		// if EINTR(interupt system call) is detected, do the same write operation again because the original one was blocked
		if (errno == EINTR) {
			printf("ERRNO: \t%s\n", strerror(errno));
			write_a_char(str, socketfd, n);	
		}
		// other error return -1
		else {
			return -1;
		}
	}
	return 0;
}

int readline(int socketfd, char* buffer, int max_line) {
	char c;
	int buff_count = 0;
  //static char buff[MAXLINE];
  //static char* buff_ptr = buff;
	
	for (int i = 1; i < max_line; i++) { // i = 1 from the handout, but not sure for what?
		if(c = read_a_char(socketfd, buff_count, max_line) < 0) {
			printf("ERRNO: \t%s\n", errno);
			return -1;
		}
		// newline is detected
		else if (c == '\n') {
			*(buffer + buff_count) = 0;
			return buff_count; 
		}
		// terminate the string if EOF is detected 
		else if (c == 0) {
			printf("EOF\n");
			*(buffer + buff_count) = 0;
			return buff_count;
		}
		*(buffer + i) = c;
		buff_count++;
		printf("i = %d\n", i);
	}
	// null terminate the string when the buffer is full. Add null termination "\0" at the end of the buffer
	if (buff_count == max_line - 1) {
		buff_count++;
		*(buffer + buff_count) = 0;
		return (buff_count); 
	}
  //buffer = buff;
	return buff_count;
}

char read_a_char(int socketfd, int num, int max_line) {
	
	static char buff[MAXLINE];
	static char* cur_ptr;
	//static int buff_count;
	
	char input;
	int c;
	
	if(c = read(socketfd, buff, max_line) <= 0) {
		printf("A String read is: %s\n", buff);
		// EOF is detected
		if (c == 0) {
			printf("EOF\n");
			return 0;
		}
		// If EINTR is detected, do the same read operation again
		else if (errno == EINTR) {
			printf("ERRNO: \t%s\n", errno);
			read_a_char(socketfd, num, max_line);
		}
		// other error return -1
		else {
			printf("ERRNO: \t%s\n", errno);
			return -1;
		}
	}
	
	else {
		printf("B String read is: %s\n", buff);
		cur_ptr = buff + num;
		input = *cur_ptr;
		printf("word read is%c\n", input);
		return input;
	}
}
















