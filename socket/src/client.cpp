#include <error.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <iostream>
#include <tuple>
#include "util.hpp"

std::string MESSAGE = "Hello Server! 👋";

void waitHello(int socket)
{
    char buffer[1024] = {0};

    LOG_INFO("Waiting for server hello...");

    int bytes_received = recv(socket, buffer, 1024, 0);
    if (bytes_received < 0)
    {
        EXIT_WITH_LOG_CRITICAL("Error in receiving data from server");
    }
    std::cout << "Received message from server: " << buffer << std::endl;
}

void sendHello(int socket)
{
    if (send(socket, MESSAGE.c_str(), MESSAGE.length(), 0) < 0)
    {
        EXIT_WITH_LOG_CRITICAL("Error in sending data to server");
    }
    close(socket);
    LOG_INFO("Finished.");
}

int connectServer(sockaddr_in *server_addr)
{
    int socket_fd;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(socket_fd, (sockaddr *)server_addr, sizeof(*server_addr)))
    {
        EXIT_WITH_LOG_CRITICAL("Error in connecting server");
    }
    LOG_INFO("Creating connection succeed.")

    return socket_fd;
}

int runClient(std::string address, int port)
{
    int socket_fd;
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address.c_str());
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, address.c_str(), &server_addr.sin_addr) <= 0)
    {
        LOG_ERROR("Error in converting server address")
        EXIT
    }

    socket_fd = connectServer(&server_addr);
    waitHello(socket_fd);
    sendHello(socket_fd);
    close(socket_fd);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        LOG_ERROR(
            "Invalid arguments. \n"
            "Use like sample below\n\n"
            "client [HOST] [PORT]")
        EXIT
    }
    auto port = atoi(argv[2]);
    if (port <= 1 || port >= 65536)
    {
        LOG_ERROR("Invalid ports. Use port between 1 to 65535")
        EXIT
    }
    runClient(argv[1], port);
}
