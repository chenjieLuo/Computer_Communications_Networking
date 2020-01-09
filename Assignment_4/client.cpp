#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <iostream>

#define MAX_BUFFER_SIZE 1024 * 1024

using namespace std;

void parsing_URL(char* URL, char* host_name, char* path_name, char* file_name) {
	char* temp_URL = (char*) malloc(150 * sizeof(char));
	char* temp_host;
	char* temp_path;
	char* temp_file;
	char* temp;
	int length_host;
	int length_path;
	int length_file;
	
	// Parsing the requested URL
	
	memset(temp_URL, 0, 150 * sizeof(char));
	memcpy(temp_URL, URL, strlen(URL));
	if (strstr(temp_URL, "https://") != NULL) {
		temp_host = temp_URL + 8 * sizeof(char); // point to the next char after "https:\\"
	}
	else if (strstr(temp_URL, "http://") != NULL) {
		temp_host = temp_URL + 7 * sizeof(char); // point to the next char after "http:\\"
	}
	else {
		temp_host = temp_URL; // point to the head of URL if no "http:\\" included
	}
	// find the second "/" and str before it would be path name
	temp_path = strtok(temp_host, "/");
	temp_path = strtok(NULL, "/") - 1 * sizeof(char);
	memcpy(temp_URL, URL, strlen(URL));
	length_host = strlen(temp_host) - strlen(temp_path);
	length_path = strlen(temp_path) + 1;
	memcpy(host_name, temp_host, length_host);
	memcpy(path_name, temp_path, length_path);
	temp = strtok(temp_path, "/");
	// find the file name
	while (temp != NULL) {
		temp_file = temp;
		temp = strtok(NULL, "/");
	}
	temp_file = temp_file;
	memcpy(temp_URL, URL, strlen(URL));
	length_file = strlen(temp_file);
	memcpy(file_name, temp_file, length_file);
	
	free(temp_URL);
	
}



int main(int argc, char **argv){
	
	if (argc != 4) {
		errno = EPERM;
		printf("INPUT_ERROR: ERRNO: \t%s\n", strerror(errno));
		return -1;
	}
	char* server_name = argv[1];
	char* _server_port = argv[2];
	char* _URL_name = argv[3];
   
    
	int server_port = -1;
	int c;
    int socketfd = -1;
	char host_name[50];
    char path_name[100];
	char file_name[50];
	char GET_msg[150];

    struct sockaddr_in server_addr;
	
	char buffer[MAX_BUFFER_SIZE + 1];
	int length_recv = 0;
	char response_code[3];
	char* file_head;
	int file_length;
	char* head_response;
	
	// Parsing the requested URL
	printf("Parsing URL\n");
	parsing_URL(_URL_name, host_name, path_name, file_name);
	
	if (file_name[strlen(file_name) - 1] == '/') {
		file_name[strlen(file_name) - 1] = '\0';
	}
	
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("SOCKET_OPEN: ERRNO: \t%s\n", strerror(errno));
		return -1;
    }
	printf("Open Socket succeed\n");
	
    // Input the server port to be connected
	server_port = atoi(_server_port);
	printf("Server port set is: %d\n", server_port);
	
    // Setup the server address
	
    memset(&server_addr, 0, sizeof(server_addr)); 
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_name, &server_addr.sin_addr) < 0) {
        printf("ADDR_TRANS: ERRNO: \t%s\n", strerror(errno));
		close(socketfd);
		return -1;
    }
	
    // Connect the socket to the server
    if ((connect(socketfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0) {
        printf("CONNECT: RRNO: \t%s\n", strerror(errno));
		close(socketfd);
		return -1;
    }
	printf("Connect to %s successfully\n", server_name);
	
	// Create GET message
	sprintf(GET_msg, "GET %s HTTP/1.0\r\nHOST: %s\r\n\r\n", path_name, host_name);
	
	// Send GET request to proxy server
	if ((write(socketfd, _URL_name, strlen(_URL_name))) < 0) {
        printf("Send GET message: ERRNO: \t%s\n", strerror(errno));
		close(socketfd);
        return -1;
    }
	//printf("******GET message was sent******\n%s", GET_msg);
	printf("Target file is %s\n", path_name);
	printf("Sending request to proxy server: %s\n", server_name);
	
	// Received requested file from socket
	memset(buffer, 0, (MAX_BUFFER_SIZE * sizeof(char) + 1));
	
	// Received file and save in the buffer
	length_recv = recv(socketfd, buffer, MAX_BUFFER_SIZE * sizeof(char), 0);
	
	if (length_recv < 0) {
		printf("Recv: ERRNO: \t%s\n", strerror(errno));
		close(socketfd);
		return -1;
	}
	// If no data received, close socket and return
	else if (length_recv == 0) {
		printf("No data received from server\n");
		close(socketfd);
		return 0;
	}
	// Show response code
	if ((head_response = strstr(buffer, "HTTP/1.0")) != NULL){
		memcpy(response_code, head_response + 9, 3 * sizeof(char));
	}
	else if ((head_response = strstr(buffer, "HTTP/1.1")) != NULL){
		memcpy(response_code, head_response + 9, 3 * sizeof(char));
	}
	printf("Response Code: %s\n", response_code);
	
	printf("File received from server successfully\n");
	// Create file
	FILE* fd = fopen(file_name, "w");
	
	file_head = buffer;
	file_length = strlen(buffer);

	// Write the buffer to the file
	fwrite(file_head, sizeof(char), file_length, fd);
	printf("File saved as: %s\n", file_name);

	fclose(fd);
	close(socketfd);
	printf("Socket closed\n");
	
	return 0;
}

