#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Link the Winsock library
#pragma comment(lib, "ws2_32.lib")

static void client(int port) {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        fprintf(stderr, "WSAStartup failed\n");
        return;
    }

    SOCKET fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == INVALID_SOCKET) {
        perror("socket error:");
        WSACleanup();
        return;
    }

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons((short)port);

    // Convert string to binary address using inet_addr
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "inet_addr failed\n");
        closesocket(fd);
        WSACleanup();
        return;
    }

    // Connect to server
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        perror("connect error:");
        closesocket(fd);
        WSACleanup();
        return;
    }

    // Send a message to the server
    const char* msg = "the client says hello!";
    send(fd, msg, strlen(msg) + 1, 0);

    closesocket(fd);
    WSACleanup();
}

int main(int argc, char* argv[]) {
    if (argc > 1 && !strcmp(argv[1], "client")) {
        if (argc != 3) {
            fprintf(stderr, "not enough args!");
            return -1;
        }

        int port;
        sscanf(argv[2], "%d", &port);

        client(port);
    } else {
        fprintf(stderr, "Usage: %s client <port>\n", argv[0]);
    }

    return 0;
}
