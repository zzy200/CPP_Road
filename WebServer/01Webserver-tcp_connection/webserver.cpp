#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>
#include <string.h>

int main() {
    //create web socket, AF_INET means IPV4, SOCK_STREAM means TCP connection
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Socket creation failed !" << std::endl;
        return 1;
    }

    //create address used to bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);     //h(host, small-endian or big-endian) to n(network, all big-endian) s(short)

    //bind the socket to the address, if success bind return 0, else return -1
    if(bind(server_fd, (struct sockaddr*)& address, sizeof(address)) < 0) {
        std::cerr << "Bind failed !" << std::endl;
        close(server_fd);
        return -1;
    }

    //listen, if success bind return 0, else return -1, set the socket to passive listen mode, 
    if(listen(server_fd, 20) < 0) {
        std::cerr << "Listen failed !" << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "Server listening on port:8080" << std::endl;

    //start infinite loop to handle connection requests
    while(true) {
        int client_socket_fd;   
        struct sockaddr_in client_addr;     //store client address
        socklen_t client_addr_len = sizeof(client_addr);
        client_socket_fd = accept(server_fd, (struct sockaddr*)& client_addr, &client_addr_len);
        if(client_socket_fd < 0) {
            std::cerr << "Accept failed !" << std::endl;
            continue;
        }

        //cout client ip address
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        std::cout << "New connections from: " << client_ip << std::endl;

        //simple response
        const char* response = "Hello from my web server!\n";
        send(client_socket_fd, response, strlen(response), 0);
        
        close(client_socket_fd); //close current connection
    }

    close(server_fd);
    return 0;
}