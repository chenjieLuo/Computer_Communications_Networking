#Type "make all" to compile two files
all:server client

#Type "make server" to compile echo_server.cpp
server: server.cpp
	g++ -std=c++11 -o server server.cpp

#Type "make client" to compile client.cpp
client: client.cpp
	g++ -std=c++11 -o client client.cpp

#Type "make clean" to clean output files
clean:
	rm -rf *.o
