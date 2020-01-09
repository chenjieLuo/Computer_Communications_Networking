#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/time.h> 
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
using namespace std;

#define MAX_DATA_SIZE 1024
#define MAX_BUFFER_SIZE 102400

//  DESIGNED A DATA STRUCTURE CALLED NODE TO SAVE EACH ENTITY IN LRU CACHE. 
struct Node{
    string key;
    string data;
    string expireat;
    string date;
    string last_modified;
    Node* next;
    Node(): key(""), data(""), expireat(""), date(""), last_modified(""), next(NULL) {}
    Node(string key, string data, string expireat, string date, string last_modified){
        this->key = key;
        this->data = data;
        this->next = NULL;
        this->expireat = expireat;
        this->date = date;
        this->last_modified = last_modified;
    }
};

//  LRU CACHE IS USED TO CACHE RECENT USED K ENTITIES. WHEN A REQUEST ARRIVES, WE FIRSTLY CHECK IF IT IS CONTAINED IN THE CACHE. IF NOT WE SEND REQUEST TO THE WEB SERVER
class LRUcache{
    public:
    unordered_map<string, Node*> map;
    int size;
    int capacity;
    Node* header;
    Node* tail;
    LRUcache(int capacity) {
        this->header = new Node();
        this->tail = header;
        this->size = 0;
        this->capacity = capacity;
        map.clear();
    }
    // MOVE THE NODE TO THE END OF END OF THE QUEUE TO MAINTAIN LRU CACHE
    void movetoTail(Node* prev){
        if (prev->next == tail)
            return;
        Node* temp = prev->next;
        prev->next = temp->next;
        map[temp->next->key] = prev;
        map[temp->key] = tail;
        tail->next = temp;
        tail = tail->next;
    }
    Node* get(string key) {
        if (map.find(key) == map.end())
            return NULL;
        movetoTail(map[key]);
        return map[key]->next;
    }
    
    void push(string key, string data, string _expireat, string _date, string _last_modified) {
        if (map.find(key) != map.end()){
            map[key]->next->data = data;
            map[key]->next->expireat = _expireat;
            map[key]->next->date = _date;
            map[key]->next->last_modified = _last_modified;
            movetoTail(map[key]);
        }
        else{
            Node *temp = new Node(key, data, _expireat, _date, _last_modified);
            map[key] = tail;
            tail->next = temp;
            tail = tail->next;
            size += 1;
            if (size > capacity){
                Node* temp2 = header->next;
                map.erase(temp2->key);
                header->next = temp2->next;
                if (header->next != NULL)
                    map[temp2->next->key] = header;
                size -= 1;
            }
        }
    }
    void print_status(){
        cout << "Currently there are " << map.size() << " entities in our cache..." << endl;
        int cnt = 0;
         for (auto &x: map){
             cout << ++cnt << ": " << x.first << endl;
             cout << "Expires at: " << map[x.first]->next->expireat << endl;
             cout << "Last accessed: " << map[x.first]->next->date << endl;
             cout << "Last modified: " << map[x.first]->next->last_modified << endl;
         }
    }
};

int findsubstr(string str, string a){
    for (int i = 7; i < str.length() - a.length(); i++){
        if (str.substr(i, a.length()) == a)
            return i;
    }
    return -1;
};

void print_status(std::string s){
    std::cout << s << "..." << std::endl;
    std::cout << ".................." << std::endl;
    std::cout << ".................." << std::endl;
}

double TimeDiffToNow (string t) {
    int t_length = t.length();
    double difference;
    char* time_in = new char[t_length + 1];
    time_t curr_time;
    time_t tm_in;
    struct tm* now;
    struct tm tm;
    
    // current time
    curr_time = time(NULL);
    now = gmtime(&curr_time);
    curr_time = mktime(now);
    // input time
    strcpy(time_in, t.c_str());
    strptime(time_in, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    tm_in = mktime(&tm);
    difference = difftime(curr_time, tm_in);
    
    char* cur;
    char* in;
    
    delete[] time_in;
    return difference;
    
}

void parsing_URL(char* URL, char* host_name, char* path_name) {
    char* temp_URL = (char*) malloc(MAX_DATA_SIZE * sizeof(char));
    char* temp_host;
    char* temp_path;
    char* temp_file;
    char* temp;
    int length_host;
    int length_path;
    int length_file;
    
    // PARSING THE REQUESTED URL
    
    memset(temp_URL, 0, MAX_DATA_SIZE * sizeof(char));
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
    free(temp_URL);
}

int main(int argc, char *argv[]){
    if (argc != 3){
        errno = EPERM;
        perror("Illegal Input! Please only input your ip address and port number. ");
        exit(EXIT_FAILURE);
    }

    string ID_addr = argv[1];
    string Port = argv[2];

    int web_socket;
    int server_socket;
    int client_socket;
    struct addrinfo currinfo, *serverinfo, *p;
    int current;
    int yes = 1;
    struct sockaddr_storage client_addr;

    fd_set master_set; 
    fd_set curr_set;
    socklen_t addrlen;
    int fdmax;
    char buffer[MAX_DATA_SIZE];
    char to_get_buffer[MAX_BUFFER_SIZE];
    char to_receive_buffer[MAX_BUFFER_SIZE + 1];
    char ipv4[30];
    LRUcache mycache(10);
    
    char path_name[50];
    char timestamp[30];

    FD_ZERO(&master_set);
    FD_ZERO(&curr_set);
    memset(&currinfo, 0, sizeof(currinfo));
    currinfo.ai_family = AF_INET;
    currinfo.ai_socktype = SOCK_STREAM;
    currinfo.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, argv[2], &currinfo, &serverinfo) != 0) {
        perror("Fail to get address info");
        exit(EXIT_FAILURE);
    }
    for(p = serverinfo; p != NULL; p = p->ai_next){
        if ((server_socket = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) < 0){
            perror("server: socket");
            continue;
        }
        
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))  < 0){
            perror("Fail to create socket");
            exit(EXIT_FAILURE);
        }

        if (::bind(server_socket, p->ai_addr, p->ai_addrlen) < 0) {
            close(server_socket);
            perror("Fail to bind");
            continue;
        }
        break;
    }

    if (!p){
        perror("Fail to bind");
        exit(EXIT_FAILURE);
    }
    print_status("Socket has been created");
    print_status("Socket has been binded");

    if (listen(server_socket, 10) < 0)   {
        perror("Fail to listen");
        exit(EXIT_FAILURE);
    }
    FD_SET(server_socket, &master_set);
    fdmax = server_socket;
    socklen_t addr_length;
    while (true){
        curr_set = master_set;
        if (select(fdmax + 1, &curr_set, NULL, NULL, NULL) < 0){
            perror("Fail to select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= fdmax; i++){
            if (FD_ISSET(i, &curr_set)){
                if (i == server_socket){
                    addr_length = sizeof(client_addr);
                    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_length);
                    if (client_socket < 0) 
                        perror("Fail to accept");
                    else {
                        FD_SET(client_socket, &master_set);
                        if (client_socket > fdmax)
                            fdmax = client_socket;
                    }
                }
                else{
                    size_t received_size = 0 ;
                    memset(buffer,0, MAX_DATA_SIZE);
                    received_size = recv(i, buffer, MAX_DATA_SIZE, 0);
                    for (auto &x: buffer)
                        cout << x;
                    cout << endl;

                    if (received_size <= 0) {
                        if (received_size == 0) {
                            print_status("No more data is received");
                        } else {
                            perror("Fail to received");
                        }
                        close(i);
                        FD_CLR(i, &master_set);
                        break;
                    }

                    cout << "The client is requesting url: " << buffer << endl;
                    string url_in_str(buffer);
                    mycache.print_status();
                    Node* res = mycache.get(url_in_str);
                    
                    bool url_expired = false;
                    //  IF THE URL IS IN THE CACHE, WE NEED TO CHECK IF IT EXPIRED
                    if (res != NULL) {
                        if (res->expireat == "") {
                            if (((res->date) == "") || ((res->last_modified) == "")) {
                                cout << "Two of the expires, date and last_modified are missing..." <<endl;
                                print_status("URL in cache is not fresh, need to refresh");
                                url_expired = true;
                            }
                            else if ((TimeDiffToNow(res->date) > 86400.00) && (TimeDiffToNow(res->last_modified) > 2592000.00)) {
                                cout << "The last access is over 24 hours or the last modification is over 1 month..." <<endl;
                                print_status("URL in cache is not fresh, need to refresh");
                                url_expired = true;
                            }
                        }
                        else if (TimeDiffToNow(res->expireat) > 0.00) {
                            cout << "URL in cache is expired..." <<endl;
                            print_status("URL in cache is not fresh, need to refresh");
                            url_expired = true;
                        }
                    }
                    if ((res == NULL) || (url_expired == true)){
						if (res == NULL) {
							print_status("Currently the target url is not cached");
						}
                        memset(ipv4, 0, sizeof(ipv4));
                        memset(path_name, 0, sizeof(path_name));
                        
                        parsing_URL(buffer, ipv4, path_name);
                        cout<<"The URL is: " << ipv4 << endl;
                        
                        memset(&currinfo, 0, sizeof(currinfo));
                        currinfo.ai_family = AF_INET;
                        currinfo.ai_socktype = SOCK_STREAM;

                        if (getaddrinfo(ipv4, "http", &currinfo, &serverinfo) != 0) {
                            perror("Fail to get address info");
                            exit(EXIT_FAILURE);
                        }

                        for(p = serverinfo; p != NULL; p = p->ai_next){
                            if ((web_socket = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) < 0){
                                perror("server: socket");
                                continue;
                            }
                            
                            if (connect(web_socket, p->ai_addr, p->ai_addrlen) < 0) {
                                close(web_socket);
                                perror("Fail to conenct");
                                continue;
                            }
                            break;
                        }
                        freeaddrinfo(serverinfo);
                        memset(to_get_buffer, 0, MAX_BUFFER_SIZE);
                        strcpy(to_get_buffer, "GET ");
						if (url_in_str.substr(0, 4) != "http")
							strcat(to_get_buffer, "http://");
                        strcat(to_get_buffer, buffer);
                        strcat(to_get_buffer, " HTTP/1.0\r\n\r\n");
                        
                        strcat(to_get_buffer, "Host: ");
                        strcat(to_get_buffer, ipv4);
                        
                        received_size = send(web_socket, to_get_buffer, sizeof(to_get_buffer), 0);
                        cout << "Request sent to " << buffer << endl;
                        cout << "GET message: " << to_get_buffer << endl;
                        memset(to_receive_buffer, 0, MAX_BUFFER_SIZE);
                        received_size = 0;
                        bool received = true;
                        char *read_ptr;
                        read_ptr = to_receive_buffer;
                        size_t sent_size;
                        while (received){
                            received_size = recv(web_socket, read_ptr, MAX_DATA_SIZE*sizeof(char), 0);
                            if (strstr(read_ptr, "404") != NULL){
                                print_status("404 Not Found");
                                sent_size = send(web_socket, "404 Not Found", 10, 0);
                                close(web_socket);
                                received = false;
                                break;
                            }
                            if (received_size <= 0){
                                print_status("Received completely from web server");
                                close(web_socket);
                                received = false;
                                break;
                            }
                            print_status("Receiving data from web server");
                            read_ptr += received_size;
                        }
                        sent_size = send(i, to_receive_buffer, sizeof(to_receive_buffer), 0);
                        if (sent_size <= 0){
                            perror("Fail to send");
                            exit(EXIT_FAILURE);
                        }
                        cout << "Transmission complete." << endl;
                        
                        // PARSING FOR EXPIRES, DATE, LAST_MODIFIED
                        char* ptr;
                        string _expireat, _date, _last_modified;
                        if ((ptr = strstr(to_receive_buffer, "expires:")) != NULL) {
                            memset(timestamp, 0, sizeof(timestamp));
                            memcpy(timestamp, ptr + 9, sizeof(timestamp));
                            _expireat = timestamp;
                            cout << "expires: " << _expireat << endl;
                        }
                        else if ((ptr = strstr(to_receive_buffer, "Expires:")) != NULL) {
                            memset(timestamp, 0, sizeof(timestamp));
                            memcpy(timestamp, ptr + 9, sizeof(timestamp));
                            _expireat = timestamp;
                            cout << "expires: " << _expireat << endl;
                        }
                        else {
                            _expireat = "";
                        }
                        
                        if ((ptr = strstr(to_receive_buffer, "date:")) != NULL) {
                            memset(timestamp, 0, sizeof(timestamp));
                            memcpy(timestamp, ptr + 6, sizeof(timestamp));
                            _date = timestamp;
                            cout << "date: " << _date << endl;
                        }
                        else if ((ptr = strstr(to_receive_buffer, "Date:")) != NULL) {
                            memset(timestamp, 0, sizeof(timestamp));
                            memcpy(timestamp, ptr + 6, sizeof(timestamp));
                            _date = timestamp;
                            cout << "date: " << _date << endl;
                        }
                        else {
                            _date = "";
                        }
                        
                        if ((ptr = strstr(to_receive_buffer, "Last-Modified:")) != NULL) {
                            memset(timestamp, 0, sizeof(timestamp));
                            memcpy(timestamp, ptr + 15, sizeof(timestamp));
                            _last_modified = timestamp;
                            cout << "_last_modified: " << _last_modified << endl;
                        }
                        else {
                            _last_modified = "";
                        }
                        // IF EXPIRES AND LAST-MODIFIED ARE MISSING, NOT CACHE THE URL (WE ARE UNSURE IF WE NEED TO CACHE IT IN THIS CASE)
                        // if ((_expireat == "") &&(_last_modified =="")) {
                        //     print_status("Expires and Last-Modified are missing, the URL will not be cached");
                        //     close(i);
                        //     FD_CLR(i, &master_set);
                        //     break;
                        // }
                        mycache.push(url_in_str, to_receive_buffer, _expireat, _date, _last_modified);
                        mycache.print_status();
                        memset(buffer, 0, MAX_DATA_SIZE);
                        memset(to_get_buffer, 0, MAX_BUFFER_SIZE);
                        close(i);
                        FD_CLR(i, &master_set);
                        break;
                    }
                    else{
                        print_status("Target url is found in our cache.");
                        strcpy(to_receive_buffer, res->data.c_str());
                        int sent = send(i, to_receive_buffer, sizeof(to_receive_buffer), 0);
                        if (sent <= 0){
                            perror("Fail to send");
                            exit(EXIT_FAILURE);
                        }
                        cout << "Transmission complete." << endl;
                        mycache.print_status();
                        memset(buffer, 0, MAX_DATA_SIZE);
                        memset(to_get_buffer, 0, MAX_BUFFER_SIZE);
                        close(i);
                        FD_CLR(i, &master_set);
                        break;
                    }
                }
            }
        }
    }
    return 0;
}