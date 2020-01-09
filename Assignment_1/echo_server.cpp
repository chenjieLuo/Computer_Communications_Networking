#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <string.h> // string in mac; sting.h in Linux
#include <iostream>
#include <sys/types.h>
#include <arpa/inet.h>

//#define PORT 4500

//using namespace std;

//PRINT OUT SUCCESS MESSAGE FOR CREATING SOCKETS
void socket_created(){
    std::cout << "Socket has been created..." << std::endl;
    std::cout << ".................." << std::endl;
    std::cout << ".................." << std::endl;
}

//USER INPUT TO GET PORT NUMBER
void get_PORT(int &PORT, std::string &INPUT){
    PORT = stoi(INPUT);
    std::cout << "User input Port number is: " << PORT << std::endl;
    PORT = stoi(INPUT);
    std::cout << "Port number has been set..." << std::endl;
    std::cout << ".................." << std::endl;
    std::cout << ".................." << std::endl;
}

//WRITEN FUNCTION WRITE len BYTES TO THE socket_fd.
//IF FAILED, IT SHOULD RETURN -1. OTHERWISE, len SHOULD BE RETURNED.
int writen(int &socket_fd, char* buffer, int len){
    int currptr = 0;
    int has_written = 0;
    while (currptr < len){
        has_written = write(socket_fd, buffer, len - currptr);
        if (has_written <= 0)
            return -1;
        buffer += has_written;
        currptr += has_written;
    }
    return currptr;
}

int main(int argc, char **argv){
    if (argc != 2){
        errno = EPERM;
        perror("Illegal Input! Please only enter you Port number");
        exit(EXIT_FAILURE);
    }
    int PORT = -1;
    std::string str = argv[1];
    int server_fd;
    int new_socket;
    int val_read;
    struct sockaddr_in address;
    int writenout = 0;
    int addrlen = sizeof(address);
    char buffer[1024] = {0}; // buffer[10] for test the maximum length
    //::string str_to_send;
    pid_t child;
    
    //CREATE A SOCKET WITH SOCKET DESCRIPTOR socket_fd. IF socket_fd < 0, IT FAILED TO CREATE A SOCKET
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        errno = ETIMEDOUT;
        perror("Failed to create socket...");
        exit(EXIT_FAILURE);
    }
    socket_created();
    get_PORT(PORT, str);
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    //BIND THE SOCKET TO THE IP ADDRESS AND PORT
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        errno = EADDRINUSE;
        perror("Failed to bind...");
        exit(EXIT_FAILURE);
    }
    std::cout << "Socket has been binded successfully! " << std::endl;
    std::cout << "Wait for connection..." << std::endl;
    
    //SERVER WILL KEEP ACCEPTING
    while (true){
        //SET server_fd TO PASSIVE SOCKET AND COULD ACCEPT CONNECTION, SET MAXIMUM CONNECTION AT A TIME TO 5
        if (listen(server_fd, 5) < 0)
        {
            errno = ETIMEDOUT;
            perror("Failed to listen...");
            exit(EXIT_FAILURE);
        }
        //WHEN NEW CLIENT CONNECTS, A NEW SOCKET new_socket IS CREATED FOR COMMUNICATION
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0){
            errno = ETIMEDOUT;
            perror("Failed to accept new client...");
            exit(EXIT_FAILURE);
        }
        std::cout << "Connection established..." << std::endl;
        std::cout << ".................." << std::endl;
        std::cout << ".................." << std::endl;
        
        //WHEN NEW CLIENTS CONNECT, CREATE CHILD PROCESS TO HANDLE EACH CLIENT
        if ((child = fork()) == 0){
            val_read = read(new_socket, buffer, 1024); // buffer[10] for test the maximum length
            std::cout << buffer << std::endl;
            writenout = writen(new_socket, buffer, strlen(buffer) + 1);
            //IF writenout == -1 IS TRUE, IT MEANS SERVER FAILED TO WRITE BACK TO SOCKET
            if (writenout < 0){
                errno = ETIMEDOUT;
                perror("Failed to write back to socket...");
                exit(EXIT_FAILURE);
            }
            std::cout << "Message has been echoed back" << std::endl;
        }
    }
    return 0;
}
