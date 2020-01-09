#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

#define MAXLINE 1024    //***Modified to align with server, 10 for test the maximum length 

int writen(int socketfd, char* str, int num);
int readline(int socketfd, char* buffer, int max_line);

int main(int argc, char **argv){
	
	if (argc != 3) {
		errno = EPERM;
		printf("INPUT_ERROR: ERRNO: \t%s\n", strerror(errno));
		return -1;
	}
	char* server_name = argv[1];
	string _server_port = argv[2];
    //const char* server_name = "hera3.ece.tamu.edu"; // hera3.ece.tamu.edu for ece workstation; 192.168.0.108 for Macbook
    
	int server_port = -1;
	int c;
    int socketfd;
    int maxline = MAXLINE;
    char input[MAXLINE];
	char output[MAXLINE];
    string temp;
    struct sockaddr_in server_addr;
	
	while (1) {
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("SOCKET_OPEN: ERRNO: \t%s\n", strerror(errno));
		return -1;
    }
	printf("Open Socket succeed\n");
	
    // Input the server port to be connected
	server_port = stoi(_server_port);
	printf("Server port set is: %d\n", server_port);
	
	
    //printf("Input the server port to be connected: ");
    //scanf("%d", &server_port);	
	
    // Setup the server address
	
    memset(&server_addr, 0, sizeof(server_addr)); // Set all bits of server address to zero
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_name, &server_addr.sin_addr) < 0) {
        printf("ADDR_TRANS: ERRNO: \t%s\n", strerror(errno));
		return -1;
    }
	printf("Server name connected to is: %s\n", server_name);
    // Connect the socket to the server
    if ((connect(socketfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0) {
        printf("CONNECT: RRNO: \t%s\n", strerror(errno));
		return -1;
    }
	printf("Connect to socket\n");
	
		// Input string into the client
		printf("Insert a line into the client\n");
		fgets(input, sizeof(input), stdin);
		
		//*** Modified during server client test
		/*cin.ignore();
		getline(cin, temp); // Store the input string into input array
    
		for (int i = 0; i < temp.length(); i++){
			input[i] = temp[i];
		}*/
		//*** end
		printf("******************************\n");
		printf("The line read is: %s\n", input);
		printf("******************************\n");
	
		// Written operation
		if (writen(socketfd, input, strlen(input)) < 0) {
			printf("WRITE: ERRNO: \t%s\n", strerror(errno));
			return -1;
		}
		printf("Send message to the socket successfully\n");
		
		// Read messages from the socket
		if (read(socketfd , output, MAXLINE) < 0) { 
			printf("READ: ERRNO: \t%s\n", strerror(errno));
			return -1;
		}
		printf("Receive message from the socket successfully\n");
		printf("******************************\n");
		printf("The message is: %s\n", output);
		printf("******************************\n");
		// Reset char array for next turn
		memset(input, 0, sizeof(input));
		memset(output, 0, sizeof(output));
		close(socketfd);
		printf("Socket closed\n");
	}
	return 0;
}

int writen(int socketfd, char* str, int num) {
	int num_write = 0;
	int num_left = num;
	char* ptr = str;
	
	while (num_left > 0) {
		num_write = write(socketfd, ptr, 1); // Write message to socket with one char at one time
		if (num_write <= 0) {
			// if EINTR(interupt system call) is detected, do the same write operation again because the original one was blocked
			if (num_write < 0 && errno == EINTR) {
				printf("writen: ERRNO: \t%s\n", strerror(errno));
				num_write = 0;	// no char being written
			}
			// other error detected
			else {
				printf("writen: ERRNO: \t%s\n", strerror(errno));
				return -1;
			}
		}
		num_left = num_left - num_write;
		ptr = ptr + num_write;
	}
	return (num - num_left);
}

int readline(int socketfd, char* buffer, int max_line) {
	int num_read = 0;
	int num_left = max_line - 1; // last char needs to store EOF
	char* buff_ptr = buffer;
	
	while (num_left > 0) {
		num_read = read(socketfd, buff_ptr, 1); // read one char a time
		if (num_read < 0) { 
			// EINTR is detected, read same char again
			if (errno == EINTR) {
				num_read = 0; // no char read
			}
			// other error detected
			else {
				printf("readline: ERRNO: \t%s\n", strerror(errno));
				return -1;
			}
		}
		// EOF detected, break the read loop
		else if (num_read == 0) {
			break;
		}
		// newline detected, terminate the string by assign this char to null and break the read loop
		else if (*(buff_ptr) == '\n') {
			*(buff_ptr) = '\0';
			break;
		}
		num_left = num_left - num_read;
		buff_ptr = buff_ptr + num_read;
	}
	// buffer full, EOF for the last char
	if (num_left == 0) {
		*(buff_ptr) = '\0';
		num_left = 1; // for returning the correct nnumber
	}
	return (max_line - num_left); // return number of read char
}
















