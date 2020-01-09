#include <stdio.h>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <iostream>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/time.h> 
#include <vector>
#include <queue>
#include <netdb.h>
#include <unordered_map>

// PAYLOAD LENGTH
#define MAXLINE 512
// HEADER TYPE
#define JOIN 2
#define SEND 4
#define FWD 3
// ATTRIBUTE TYPE
#define USERNAME 2
#define MESSAGE 4
#define REASON 1
#define CLIENT_COUNT 3

// JOIN AND LEAVE MSG
#define LEAVE_MSG " leaves"
#define JOIN_MSG " joins"
#define REASON_TOO_MANY_USER "Too many users. Maximum number of clients has been reached..."
#define REASON_EXISTED_USER_NAME "The username you chose has been used by others..."

using namespace std;

struct SBCP_ATTRIBUTE{
    unsigned int type : 16;
    unsigned int length : 16;
    char payload[512]; // maximum size of MESSAGE equals to 512 bytes
};

struct SBCP_MSG{
    unsigned int vrsn : 9;
    unsigned int type : 7;
    unsigned int length : 16;
    struct SBCP_ATTRIBUTE attribute[2];
};

struct SBCP_CLIENT_INFO{
    char username[16];
    int fd;
};

//PRINT OUT SUCCESS MESSAGE FOR CREATING SOCKETS
void socket_created(){
    std::cout << "Socket has been created..." << std::endl;
    std::cout << ".................." << std::endl;
    std::cout << ".................." << std::endl;
}

//USER INPUT TO GET PORT NUMBER
void PORT_obtained(int &PORT, std::string &INPUT){
    PORT = stoi(INPUT);
    std::cout << "User input Port number is: " << PORT << std::endl;
    std::cout << "Port number has been set..." << std::endl;
    std::cout << ".................." << std::endl;
    std::cout << ".................." << std::endl;
}

void print_status(std::string s){
    std::cout << s << "..." << std::endl;
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

void address_set(struct sockaddr_in &address, int &PORT){
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
}

// SET max_clients AND CHECK IF THE INPUT IS REASONABLE
void max_clients_obtained(int &max_clients, string &max_clients_str){
    max_clients = stoi(max_clients_str);
    if (max_clients > 20 or max_clients < 0){
        errno = EPERM;
        perror("Illegal Input! The maximum clients number can only be positive integer and cannot be greater than 20");
        exit(EXIT_FAILURE);
    }
    std::cout << "Maximum clients allowance is: " << max_clients << std::endl;
    std::cout << ".................." << std::endl;
    std::cout << ".................." << std::endl;
}

bool isempty(char*buffer){
    if (buffer[0] == '\0')
        return true;
    else
        return false;
}

// CHECK INSIDE clients TO SEE IF THE USERNAME HAS BEEN USED
bool user_exist(char user_name[], vector<struct SBCP_CLIENT_INFO> &clients, int &num_clients){
    for(int i = 0; i < num_clients; i++){
        if(strcmp(user_name,clients[i].username) == 0){
            return false;
        }
    }
    return true;
}

// CHECK IF CERTAIN USER HAS JOINED BEFORE
bool is_joined(int client_fd, vector<struct SBCP_CLIENT_INFO> &clients, int &num_clients){
    struct SBCP_MSG join_msg;
    struct SBCP_ATTRIBUTE join_msg_attribute;
    char username[16];
    read(client_fd,(struct SBCP_MSG *) &join_msg,sizeof(join_msg));
    join_msg_attribute = join_msg.attribute[0];
    strcpy(username, join_msg_attribute.payload);
    
    if (user_exist(username, clients, num_clients) == false){
        cout << "Username already exists!" << endl;
        return true;
    }
    strcpy(clients[num_clients].username, username);
    clients[num_clients].fd = client_fd;
    num_clients += 1;
        
    return false;
}

// IF A NEW USER JOINED THE CHAT ROOM, AN ACK MESSAGE WILL BE SENT TO HIM TO SHOW NUMBER OF CLIENTS RIGHT NOW AND THEIR NAMES
void send_ACK(int &num_clients, vector<struct SBCP_CLIENT_INFO> &clients, int &index){
    struct SBCP_MSG new_msg;
    new_msg.vrsn = 3;
    new_msg.type = 7;
    new_msg.attribute[0].type = 3;
    
    char cnt_in_array[10];
    sprintf(cnt_in_array, "%d", num_clients);
    strcpy(new_msg.attribute[0].payload, cnt_in_array);
    new_msg.attribute[1].type = 4;
    strcpy(new_msg.attribute[1].payload, "Current users are: \n");
    for (int i = 0; i < num_clients; i++){
        strncat(new_msg.attribute[1].payload, clients[i].username, sizeof(clients[i].username));
        strncat(new_msg.attribute[1].payload, ", \n", sizeof(", \n"));
    }
    if (write(clients[index].fd, (void *)&new_msg, sizeof(new_msg)) < 0){
        perror("Fialed to ACK...");
    }
    return;
}

//  IF A NEW USER FAILED TO JOIN THE CHAT ROOM, A NAK MESSAGE WILL BE SENT TO HIM TO TELL HIM FALURE TO JOIN AND REASON
void send_NAK(int &fd, int reasons){
    struct SBCP_MSG new_msg;
    new_msg.vrsn = 3;
    new_msg.type = 5;
    new_msg.attribute[0].type = 1;
    if (reasons == 0)
        strcpy(new_msg.attribute[0].payload, REASON_TOO_MANY_USER);
    else if (reasons == 1)
        strcpy(new_msg.attribute[0].payload, REASON_EXISTED_USER_NAME);
    if (write(fd, (void *)&new_msg, sizeof(new_msg)) < 0){
        perror("Fialed to NAK...");
    }
    return;
}


int main(int argc, char **argv){
    if (argc != 4){
        errno = EPERM;
        perror("Illegal Input! Please only enter your IP addr, server port and max clients in order");
        exit(EXIT_FAILURE);
    }
    std::string IP_addr = argv[1];
    std::string port_str = argv[2];
    std::string max_clients_str = argv[3];
    int socket_fd;
    int new_socket;
    int PORT;
    int max_clients;
    std::string str_read;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    fd_set fd_list;
    int current_fd;
    int max_fd;
    char *buffer;
    char *to_send;
    std::queue<char*> to_send_queue;
    int val_read;
    int writenout;
    char welcome_message[45] = "Welcome! You have connected to the server! ";
    int num_clients;
    fd_set temp_fd_list;
    struct SBCP_MSG received_msg;
    struct SBCP_MSG forward_msg;
    struct SBCP_MSG broadcast_join_msg;
    struct SBCP_MSG broadcast_leave_msg;
    struct SBCP_ATTRIBUTE client_attribute;
    FD_ZERO(&fd_list);
    FD_ZERO(&temp_fd_list);
    int no = 0;

    // CREATE A SOCKET WITH A DESCRIPTER socket_fd WHICH BOTH SUPPORT IPv6 and IPv4
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        errno = ETIMEDOUT;
        perror("Failed to create socket...");
        exit(EXIT_FAILURE);
    }

    socket_created();

    PORT_obtained(PORT, port_str);
    max_clients_obtained(max_clients, max_clients_str);
    int client_fd[max_clients];

    for (int i = 0; i < max_clients; i++)
        client_fd[i] = 0;
    address_set(address, PORT);
    memset(&(address.sin_zero), '\0', 8);

    //BIND THE SOCKET TO THE IP ADDRESS AND PORT
    if (::bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        errno = EADDRINUSE;
        perror("Failed to bind...");
        exit(EXIT_FAILURE);
    }
    print_status("Socket has been binded successfully!");
    std::cout << "Wait for connection..." << std::endl;

    // RESERVE MEMORY SPACE FOR STROING JOINED CLIENTS INFO INCLUDING FILE DESCRIPTER AND USERNAME
    struct SBCP_CLIENT_INFO new_client;
    struct sockaddr_in new_addr;
    vector<struct SBCP_CLIENT_INFO> clients(max_clients, new_client);
    vector<struct sockaddr_in> clients_addr(max_clients, new_addr);

    //SET server_fd TO PASSIVE SOCKET AND COULD ACCEPT CONNECTION, SET MAXIMUM CONNECTION AT A TIME TO 10
    if (listen(socket_fd, 10) < 0){
        errno = ETIMEDOUT;
        perror("Failed to listen...");
        exit(EXIT_FAILURE);
    }
    
    FD_SET(socket_fd, &fd_list);
    max_fd = socket_fd;  
    while (true){
        temp_fd_list = fd_list;
        // USE select() TO CHECK ALL THE SOCKETS IF NEW MESSAGES ARRIVE. THE TIMEOUT IS SET TO INFINITE UNTIL ANY OF SOCKET HAS AN UPDATE
        if (select(max_fd + 1, &temp_fd_list, NULL, NULL, NULL) < 0){
            perror("Failed to select...");
            exit(EXIT_FAILURE);
        }
        // CHECK ALL THE FILE DESCRIPTER TO SEE IF NEW MESSAGES ARRIVE
        for (int i = 0; i <= max_fd; i++){
            if (FD_ISSET(i, &temp_fd_list)){
                if (i == socket_fd){
                    socklen_t client_addr_size = sizeof(clients_addr[num_clients]);
                    new_socket = accept(socket_fd, (struct sockaddr *)&clients_addr[num_clients], &client_addr_size);
                    if (new_socket < 0){
                        perror("Failed to accept...");
                        exit(EXIT_FAILURE);
                    }
                    if (num_clients < max_clients){
                        if (is_joined(new_socket, clients, num_clients) == false){
                            FD_SET(new_socket, &fd_list);

                            max_fd = max(max_fd, new_socket);
                            int index = 0;
                            for (; index < num_clients; index++){
                                if (clients[index].fd == new_socket)
                                    break;
                            }
                            std::cout << "User " << clients[index].username << " users joined the chat room..." << std::endl;
                            std::cout << "Currently there are "<< num_clients << " in our chat room..." << std::endl;
                            //  WHEN A NEW CLIENT JOIN THE CHAT, SEND AN ACK TO HIM TO INDICATE HE HAS JOIN SUCCESSFULLY
                            send_ACK(num_clients, clients, index);
                            broadcast_join_msg.vrsn = 3;
                            broadcast_join_msg.type = 8;
                            broadcast_join_msg.attribute[0].type = 2;

                            strcpy(broadcast_join_msg.attribute[0].payload, clients[index].username);

                            for (int j = 0; j <= max_fd; j++){
                                if (FD_ISSET(j, &fd_list)){
                                    if (j != socket_fd and j != new_socket){
                                        //  BROADCAST TO EVERY OTHER CLIENT THAT A NEW CLIENT HAS JOINED IN
                                        if (write(j, (void *)&broadcast_join_msg, sizeof(broadcast_join_msg)) < 0){
                                            perror("Failed to broadcast...");
                                            exit(EXIT_FAILURE);
                                        }
                                    }
                                }
                            }
                            cout << "All other clients have known a new client joined..." << endl;
                        }
                        else{
                            send_NAK(new_socket, 1);
                        }
                    }
                    else{
                        // IF THE NUMBER OF CAPACITY HAS BEEN REACHED NO MORE USERS CAN CONNECT
                        send_NAK(new_socket, 0);
                        std::cout << "Capacity of users has been reached. New client is abandoned..."<< std::endl;
                    }
                }
                else{
                    val_read = read(i, (struct SBCP_MSG *) &received_msg, sizeof(received_msg));
                    if (val_read < 0){
                        perror("Failed to read message...");
                    }
                    if (val_read == 0){
                        int k = 0;
                        for (; k < num_clients; k++){
                            if (clients[k].fd == i)
                                break;
                        }
                        broadcast_leave_msg.type = 6;
                        broadcast_leave_msg.attribute[0].type = 2;
                        broadcast_leave_msg.vrsn = 3;
                        broadcast_leave_msg.length = 520;
                        broadcast_leave_msg.attribute[0].length = 516;
                        strcpy(broadcast_leave_msg.attribute[0].payload, clients[k].username);
                        
                        cout << "User " << clients[k].username << " has left chat room..." << endl;

                        // BROADCAST clients[k].username HAS LEFT THE CHAT ROOM
                        for (int j = 0; j <= max_fd; j++){
                            if (FD_ISSET(j, &fd_list)){
                                if (j != socket_fd){
                                    if (write(j, (void*)&broadcast_leave_msg, sizeof(broadcast_leave_msg)) < 0)
                                        perror("Failed to broadcast...");
                                }
                            }
                        }
                    }
                    if (val_read <= 0){
                        close(i);
                        FD_CLR(i, &fd_list);
                        for (int a = i; a < num_clients; a++)
                            clients[a] = clients[a + 1];
                        num_clients -= 1;
                    }
                    else{
                        // IT IS A SEND MESSAGE AND JUST FORWARD TO OTHERS
                        forward_msg.vrsn = received_msg.vrsn;
                        forward_msg.type = 3;
                        forward_msg.attribute[0].length = received_msg.attribute[0].length;
                        forward_msg.attribute[0].type = 4;

                        // CHECK THE INFO OF SEND BY USING FILE DESCRIPTER
                        int k = 0;
                        for (; k < num_clients; k++){
                            if (clients[k].fd == i)
                                break;
                        }

                        if (received_msg.type == 9){
                            strcpy(forward_msg.attribute[0].payload, clients[k].username);
                            strncat(forward_msg.attribute[0].payload, " is IDLE.", sizeof(" is IDLE."));
                            cout << forward_msg.attribute[0].payload << endl;
                        }
                        else{
                            strcpy(forward_msg.attribute[0].payload, clients[k].username);
                            strncat(forward_msg.attribute[0].payload, " says: ", sizeof(" says: "));
                            strncat(forward_msg.attribute[0].payload, received_msg.attribute[0].payload, sizeof(received_msg.attribute[0].payload));
                            cout << forward_msg.attribute[0].payload << endl;
                        }

                        for (int j = 0; j <= max_fd; j++){
                            if (FD_ISSET(j, &fd_list)){
                                if (j != socket_fd and j != i){
                                    // FORWARD THE MESSAGE TO EVERY OTHER JOINED CLIENTS
                                    if (write(j, (void*) &forward_msg, val_read) < 0)
                                        perror("Failed to write...");
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}