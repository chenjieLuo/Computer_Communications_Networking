This project is divided into two parts: 
Chenjie Luo(UIN: 324007289) is responsible for the server side while Yuwen Chen(UIN: 227009499) is responsible for elient side. 

This project consists of four main files: 
1. echo_server.cpp
2. echo_server
3. client.cpp
4. client

Since at the beginning we used different languages for client and server, we update client.c to client.cpp to align with server side. We uploaded original client.c file as well. 

The functionality of this project is to send words from client to server and echo it back. To realize this, we write our own realine function as well as writen function. Readline function will keep reading until '\n' is reached. Similarly, writen function will keep writing to socket until all chars in the buffer has been written in to socket. If readline() or writen() failed to do so, they will return -1 as flag. 
