#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>

using namespace std;

#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5
#define ERROR_MSG_OPEN_FILE "Fail to open target file"


//  SELF DEFINED DATA STRUCTURE NAME TFTP_MSG, IT CONSISTS OF FOUR ATTRIBUTES
struct TFTP_MSG{
    int opcode;
    char *filename;
    int block_num;
    char *mode;
};


void print_status(std::string s){
    std::cout << s << "..." << std::endl;
    std::cout << ".................." << std::endl;
    std::cout << ".................." << std::endl;
}

void address_set(struct sockaddr_in &address, int &PORT){
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
}

void handle_sigchld(int sig) {
    int saved_errno = errno;
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
    errno = saved_errno;
}

//  CONVERT UNSIGNED SHORT INTEGER TO HOST BYTE ORDER
unsigned short int convert_network_byte(char *buf){
    unsigned short int i;
    memcpy(&i, buf, 2);
    i = ntohs(i);
    return i;
}

//  CONVERT TO NETWORK BYTE ORDER
void convert(char *buf, unsigned short int i){
    i = htons(i);
    memcpy(buf,&i,2);
}

//  READ FROM BUFFER AND CONVERT INTO TFTP_MSG STRUCTURE
struct TFTP_MSG* readFile(char *buffer){
    struct TFTP_MSG* out = (struct TFTP_MSG*)malloc(sizeof(struct TFTP_MSG));
    out->opcode = convert_network_byte(buffer);
    if (out->opcode == ACK){
        out->block_num = convert_network_byte(buffer + 2);
    }
    if (out->opcode == RRQ){
        int i = 0;
        char temp[512];
        while (buffer[i + 2] != '\0'){
            temp[i] = buffer[i + 2];
            i += 1;
        }
        temp[i] = '\0';
        out->filename = (char*)malloc(strlen(temp)*sizeof(char));
        strcpy(out->filename, temp);
        while(buffer[i + 2]=='\0')
            i++;
        int j = 0; 
        while (buffer[i + 2] != '\0'){
            temp[j] = buffer[i + 2];
            i += 1;
            j += 1;
        }
        temp[j] = '\0';
        out->mode = (char*)malloc(strlen(temp)*sizeof(char));
        strcpy(out->mode, temp);
    }
    return out;
}

//  WRAP THE MESSAGE FROM BUFFER INTO CHAR* TO SEND
char *wrap_msg(int opcode, int block_number, char *file_buffer, int length){
    char *out;
    if(opcode == DATA){
        out=(char *)malloc((length + 4)*sizeof(char));
        convert(out,opcode);
        convert(out + 2,block_number);
        memcpy(out + 4,file_buffer,length);
    }
    if(opcode == ERROR){
        out=(char *)malloc((length+5)*sizeof(char));
        convert(out,opcode);
        convert(out + 2,block_number);
        memcpy(out + 4,file_buffer,length);
        memset(out + 4 + length,'\0',1);
    }
    return out;
}

int readable_timeout(int fd, int sec){
    fd_set rset;
	struct timeval tv;
    
    FD_ZERO(&rset);
    FD_SET(fd, &rset);
	
    tv.tv_sec = sec;
    tv.tv_usec = 0;
    
    return (select(fd + 1, &rset, NULL, NULL, &tv));
}

int main(int argc, char **argv){
    if (argc != 3){
        errno = EPERM;
        perror("Illegal Input! Please only enter your IP addr and server port");
        exit(EXIT_FAILURE);
    }
    std::string IP_addr = argv[1];
    std::string port_str = argv[2];

    // fd_list saves all file descriptors
    fd_set fd_list;
    // curr_fd_list reads all current file descriptors
    fd_set curr_fd_list;
    FD_ZERO(&fd_list);
    FD_ZERO(&curr_fd_list);
    int max_fd;
    int socket_fd;  // server's socket
    int new_socketfd;

    char buffer[512];
    char s[INET_ADDRSTRLEN];
    int opt = 1;

    socklen_t client_addr_size;
    struct addrinfo addressinfo;
    struct addrinfo *server_addrinfo; 
    struct addrinfo *curr_addrinfo;
    struct sockaddr_storage client_addr;
    struct sockaddr addr;
    struct sockaddr_in *address;
    memset(&addressinfo, 0, sizeof(addressinfo));
    pid_t child;
    int bytes;

    addressinfo.ai_family = AF_INET;  
    addressinfo.ai_socktype = SOCK_DGRAM;   // SET THE PROTOCOL TO UDP
    if ((getaddrinfo(argv[1], argv[2], &addressinfo, &server_addrinfo)) != 0) {
        errno = EPERM;
        perror("Fail to set address...");
        exit(EXIT_FAILURE);
    }
    for (curr_addrinfo = server_addrinfo; curr_addrinfo != NULL; curr_addrinfo = curr_addrinfo->ai_next){
        // CREATE A SOCKET WITH A DESCRIPTER socket_fd WHICH SUPPORT IPv4
        if ((socket_fd = socket(curr_addrinfo->ai_family, curr_addrinfo->ai_socktype, curr_addrinfo->ai_protocol)) < 0){
            errno = ETIMEDOUT;
            continue;
        }
        print_status("Socket has been created...");
        setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (::bind(socket_fd, curr_addrinfo->ai_addr, curr_addrinfo->ai_addrlen) < 0){
            errno = EADDRINUSE;
            continue;
        }
        break;
    }

    if (curr_addrinfo == NULL){
        errno = EPERM;
        perror("Fail to create a socket and bind...");
        exit(EXIT_FAILURE);
    }

    print_status("Socket has been binded successfully!");
    print_status("Wait for connection");

    // int bytes;
    addr = *(curr_addrinfo->ai_addr);
    address = (struct sockaddr_in *) &addr;
    address->sin_port = htons(0);

    struct sigaction sa;
    sa.sa_handler = handle_sigchld; // wipe out all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    char nextchar = -1;

    while (true){
        client_addr_size = sizeof(client_addr);

        if (recvfrom(socket_fd, buffer, 511, 0, (struct sockaddr *)&client_addr, &client_addr_size) < 0){
            exit(EXIT_FAILURE);
        }
        struct sockaddr_in* new_client_addr = (struct sockaddr_in*) &client_addr;

        cout << "Received packet from IP: " << inet_ntop(client_addr.ss_family,&(((struct sockaddr_in*)address)->sin_addr),s, sizeof s) << endl;

        struct TFTP_MSG *to_send;
        to_send = readFile(buffer);
        if (to_send->opcode != RRQ){
            if (to_send->opcode == WRQ)
                print_status("Currently WRQ is not valid");
            else
            print_status("Invalid request");
            continue;
        }

        if (fork() == false){
            close(socket_fd);
            if ((socket_fd = socket(curr_addrinfo->ai_family, curr_addrinfo->ai_socktype, curr_addrinfo->ai_protocol)) < 0){
                exit(EXIT_FAILURE);
            }
            setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            if (::bind(socket_fd, &addr, sizeof(addr)) < 0){
                errno = EADDRINUSE;
                perror("Fail to bind...");
            }
            socklen_t new_len = sizeof(addr);
            if(getsockname(socket_fd, &addr, &new_len) == -1){
                perror("Error");
            }

            FILE *f;
            f = fopen(to_send->filename, "rb");

            if (f == NULL){
                print_status("Fail to open target file");
                char error_msg[512];
                strcpy(error_msg, "Fail to open target file");
                int error_length = strlen(error_msg);
                char *error_buffer = wrap_msg(ERROR, 1, error_msg, error_length);
                if ((sendto(socket_fd, error_buffer, error_length + 5, 0, (struct sockaddr*)&client_addr, client_addr_size)) < 0){
                    perror("Fail to send error message...");
                    continue;
                }
                exit(EXIT_FAILURE);
            }

            //  SEARCH FOR THE FILE SIZE AND RESET THE POINTER FILE *f TO THE BEGINNING OF FILE
            fseek(f, 0, SEEK_END);
            int file_size = ftell(f);
            fseek(f, 0, SEEK_SET);
            int num_packet = file_size / 512 + 1;

            cout << "The file size is: " << file_size << endl;
            //cout << "Number of packets needed: " << num_packet << endl;

            char file_buffer[512];
            int block_num = 0; 
            int block_cnt = 0;
            int last_ACK = 0;
            int file_length = 0;
            int special_letter = 0;
            char *ptr = file_buffer;
            int totalcnt = 0;
            short int c;
            int overhead = 0;
			int timeout, sel_timeout;
			int opc = -1;

            FD_SET(socket_fd, &fd_list);
            max_fd = socket_fd;

            while (last_ACK < num_packet){
                //  RUN THE ROLL OVER TO AVOID DEFAULT CLIENT TERMINATE
                if (block_num == 65536){
                    block_num = 0;
                }

                if (last_ACK == block_num){
                    for (int cnt = 0; cnt < 512; cnt++){
                        c = getc(f);
                        //  KEEP READING EVEN COME ACROSS EOF, THE FOLLOWING LOGIC WILL RESOVE THE LAST PACKET ISSUE
                        if (c == EOF){
                            if (ferror(f))
                                perror("Error");
                            break;
                        }
                        *ptr++ = c;
                        file_length += 1;
                    }
                    cout << "The file length is " << file_length << endl;
                    block_num += 1;
                    block_cnt += 1;
                    totalcnt += 1;
                    ptr = file_buffer;
                }
                //  THIS IF CONDITION PLAYS THE ROLE TO DETECT WHETHER THIS IS THE LAST PACKET
                if (block_cnt == num_packet)
                    file_length = file_size % 512;
				
                char* data_packet = wrap_msg(DATA, block_num, file_buffer, file_length);
				char* recv_packet = (char*) malloc(4 * sizeof(char));
                //cout << "Block number is " << block_num << endl;
				for (timeout = 0; timeout < 10; timeout++) { // if timeout over 10 times, the trasmission will be stopped and socket  will be closed
					if ((sendto(socket_fd, data_packet, file_length + 4, 0, (struct sockaddr *)&client_addr, client_addr_size)) < 0){
						perror("Fail to send...");
						exit(EXIT_FAILURE);
					}
					sel_timeout = readable_timeout(socket_fd, 1); // set time out to 1 second
					if (sel_timeout < 0) {
						perror("Select error: Receiving ACK...");
						exit(EXIT_FAILURE);
					}
					else if (sel_timeout == 0) {
						cout << "Timeout of receiving ACK: " << last_ACK << endl;
					}
					else {
						if (recvfrom(socket_fd, recv_packet, sizeof(recv_packet), 0, (struct sockaddr *)&client_addr, &client_addr_size) < 0){
							perror("Fail to receive ACK...");
							exit(EXIT_FAILURE);
						}
						opc = convert_network_byte(recv_packet);
						if (opc == 4) { // Check if the received message is ACK
							cout << "ACK received: " << last_ACK << endl;
							break;
						}
					}
				}
				if (timeout == 10) { // timeout equal to 10 times, close socket and child process
						cout << "Timeout happended over 10 times: close the socket." << endl;
						fclose(f);
						close(socket_fd);
						_exit(2); // close child process
				}
				file_length = 0;
				//free(data_packet);
				//memset(file_buffer, '\0', 512);
				/*ptr = file_buffer;
				curr_fd_list = fd_list;
				if (select(max_fd + 1, &curr_fd_list, NULL, NULL, NULL) < 0){
					perror("Fail to get update");
					exit(EXIT_FAILURE);
				}
				if(FD_ISSET(socket_fd, &curr_fd_list)){
					struct sockaddr_storage curr_addr;
					socklen_t curr_length = sizeof(curr_addr);
					
					if((bytes = recvfrom(socket_fd, buffer, 4, 0, (struct sockaddr *)&curr_addr, &curr_length)) == -1){
						perror("Fail to receive from socket");
						exit(EXIT_FAILURE);
					}
				}
				*/
                struct sockaddr_in* curr_client_addr = (struct sockaddr_in*)& client_addr;
                if(curr_client_addr->sin_addr.s_addr == new_client_addr->sin_addr.s_addr){
                    to_send = readFile(recv_packet);
                    last_ACK = to_send->block_num;
                }
				//cout << "block_cnt = " << block_cnt << ", num_packet = " << num_packet <<endl;
                if (block_cnt > num_packet)
                    break;
				free(data_packet);
				free(recv_packet);
            }
			cout << "RRQ service finished, closing the socket" << endl;
			close(socket_fd);
            fclose(f);
			_exit(2); // close child process
        }
    }
    close(socket_fd);
}