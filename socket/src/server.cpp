#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <iostream>
#include "util.hpp"
#include <thread>
#include <tuple>

#define MAX_CONNECTION 1
#define MAX_EVENTS 1

std::string MESSAGE = "Hello Client! 👋";

int startListening(int port)
{
    int server_socket;
    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    // Opening Socket Failed
    if (server_socket < 0)
    {
        EXIT_WITH_LOG_CRITICAL("Error in creating socket.");
    }

    // Bind Address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        EXIT_WITH_LOG_CRITICAL("Error in binding socket.")
    }

    // Listen for incoming connections
    if (listen(server_socket, MAX_CONNECTION) < 0)
    {
        EXIT_WITH_LOG_CRITICAL("Error in listening socket")
    }

    return server_socket;
}

std::tuple<int, epoll_event> registerEpoll(int server_socket)
{
    epoll_event event;
    int epoll_fd;
    ;

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
    {
        EXIT_WITH_LOG_CRITICAL("Error in attaching io events.")
    }

    event.events = EPOLLIN;
    event.data.fd = server_socket;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) < 0)
    {
        EXIT_WITH_LOG_CRITICAL("Error in handling io events.")
    }

    return std::make_tuple(epoll_fd, event);
}

void handleConn(int socket)
{
    char buffer[1024] = {0};
    if (send(socket, MESSAGE.c_str(), MESSAGE.length(), 0) < 0)
    {
        EXIT_WITH_LOG_CRITICAL("Error in sending data to client")
    }
    LOG_INFO("Send message suceed.")

    int valRead = recv(socket, buffer, 1024, 0);
    if (valRead == 0)
    {
        EXIT_WITH_LOG_CRITICAL("Client disconnected")
    }
    else if (valRead < 0)
    {
        EXIT_WITH_LOG_CRITICAL("Error in reading from client")
    }
    std::cout << "Received message from client: " << buffer << std::endl;

    close(socket);
    LOG_INFO("Finished");
    exit(EXIT_SUCCESS);
}

int runServer(int port)
{
    int event_count, server_socket, client_socket;
    struct sockaddr client_addr;
    socklen_t client_sz = sizeof(client_addr);
    epoll_event events[MAX_EVENTS];

    server_socket = startListening(port);
    auto [epoll_fd, event] = registerEpoll(server_socket);

    LOG_INFO("Server starts...")

    while (true)
    {
        event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (event_count < 0)
        {
            EXIT_WITH_LOG_CRITICAL("Error in waiting for io events")
        }

        for (int i = 0; i < event_count; i++)
        {
            if (events[i].data.fd == server_socket)
            {
                client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_sz);
                if (client_socket < 0)
                {
                    EXIT_WITH_LOG_CRITICAL("Error in accepting new connection.")
                    continue;
                }
                LOG_INFO("New client connected.")

                handleConn(client_socket);
                break;
            }
        }
    };

    close(server_socket);

    return 0;
}

#ifdef COMPILE_MAIN
int main(int argc, char *argv[])
{
    runServer(9999);
}
#endif