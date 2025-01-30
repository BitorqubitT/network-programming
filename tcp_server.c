/*
** server.c -- a stream socket server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define PORT "8080"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

DWORD WINAPI handle_client(LPVOID client_socket)
{
    SOCKET sock = *(SOCKET *)client_socket;
    free(client_socket);  // Free allocated memory after copying

    char buffer[1024] = "Hello, world!";
    if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR) {
        fprintf(stderr, "send failed: %d\n", WSAGetLastError());
    }
    
    closesocket(sock);
    return 0;
}

int main(void)
{
    WSADATA wsaData;
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    //hints.ai_family = AF_UNSPEC;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        WSACleanup();
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == INVALID_SOCKET) {
            fprintf(stderr, "server: socket: %d\n", WSAGetLastError());
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes,
                sizeof(int)) == SOCKET_ERROR) {
            fprintf(stderr, "setsockopt: %d\n", WSAGetLastError());
            closesocket(sockfd);
            WSACleanup();
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, (int)p->ai_addrlen) == SOCKET_ERROR) {
            fprintf(stderr, "server: bind: %d\n", WSAGetLastError());
            closesocket(sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        WSACleanup();
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == SOCKET_ERROR) {
        fprintf(stderr, "listen: %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while (1) {
        sin_size = sizeof their_addr;
        SOCKET *client_socket = (SOCKET *)malloc(sizeof(SOCKET));  // Allocate memory
        if (client_socket == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            continue;
        }

        *client_socket = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (*client_socket == INVALID_SOCKET) {
            fprintf(stderr, "accept: %d\n", WSAGetLastError());
            free(client_socket);
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        HANDLE thread = CreateThread(NULL, 0, handle_client, client_socket, 0, NULL);
        if (thread == NULL) {
            fprintf(stderr, "CreateThread failed: %d\n", WSAGetLastError());
            closesocket(*client_socket);
            free(client_socket);
        } else {
            CloseHandle(thread);
        }
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}
