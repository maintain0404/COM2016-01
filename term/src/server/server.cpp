#include "src/logging/logging.hpp"
#include "src/mychat/mychat.hpp"
#include "src/protocol/protocol.hpp"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <csignal>
#include <cstdint>
#include <fcntl.h>
#include <functional>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

class Connection {
  bool is_entered;
  int sock;
  std::function<void(int, std::vector<uint8_t>)> broadcast;
  std::function<bool(std::string)> checkExists;
  std::function<void(int)> disconnect;
  Handle handle;

public:
  std::string name;

  Connection(int sock, std::function<void(int, std::vector<uint8_t>)> broadcast,
             std::function<bool(std::string)> checkExists,
             std::function<void(int)> disconnect, Handle &handle)
      : is_entered(false), sock(sock), broadcast(broadcast),
        checkExists(checkExists), disconnect(disconnect), handle(handle),
        name(""){};

  void feed(std::vector<uint8_t> packet) {
    try {
      auto res = this->handle.feed(packet);
      if (std::holds_alternative<SendEnter>(res)) {
        //clang-format off
        if (this->is_entered) {
          this->disconnect(sock);
        }
        auto new_name = std::get<SendEnter>(res).name;
        if (!this->checkExists(new_name)) {
          std::string msg = "User " + std::get<SendEnter>(res).name +
                            " entered. Please say hello.";
          auto ntc = RecvNotice(msg);
          this->broadcast(this->sock, this->handle.buildRecvNotice(ntc));
          this->is_entered = true;
          this->name = new_name;
        } else {
          this->disconnect(sock);
        }
        //clang-format on
      } else if (std::holds_alternative<SendMessage>(res)) {
        if (this->is_entered) {
          std::string msg = std::get<SendMessage>(res).content;
          auto recv = RecvMessage(this->name, msg);
          this->broadcast(this->sock, this->handle.buildRecvMessage(recv));
        } else {
          this->disconnect(sock);
        }
      } else {
        this->disconnect(sock);
      }
    } catch (HandleReturn e) {
      this->disconnect(sock);
    }
  }
};

class Server {
private:
  // configuration
  const int port;
  const int max_connection;
  const int max_events;

  // managing
  bool stopflag;
  std::unordered_map<int, Connection> clients;
  int server_socket;
  int epoll_fd;
  epoll_event _epoll_event;
  Handle handle;

  void registerEpoll() {

    this->epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
      EXIT_WITH_LOG_CRITICAL("Error in attaching io events.");
      clear();
      exit(-1);
    }

    _epoll_event.events = EPOLLIN;
    _epoll_event.data.fd = this->server_socket;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->server_socket, &_epoll_event) <
        0) {
      EXIT_WITH_LOG_CRITICAL("Error in handling io events.");
      exit(-1);
    }
  }

  void sendMessage(std::string msg, int client) {
    if (mychat_send(client, msg.c_str(), msg.length()) < 0) {
      EXIT_WITH_LOG_CRITICAL("Error in sending data to client");
    }
  };

  void acceptNewClient(int epoll_fd) {
    int client_socket;
    struct sockaddr client_addr;
    epoll_event event;
    socklen_t client_sz = sizeof(client_addr);

    client_socket = mychat_accept(server_socket,
                                  (struct sockaddr *)&client_addr, &client_sz);
    if (client_socket < 0) {
      EXIT_WITH_LOG_CRITICAL("Error in accepting new connection.");
    }
    LOG_INFO("New client connected. Current connection " +
             std::to_string(getConnectionCount() + 1));

    // Set client socket nonblocking.
    fcntl(client_socket, F_SETFL, O_NONBLOCK);

    // Register epoll
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = client_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
      LOG_ERROR("Register client to epoll failed.");
      disconnect(client_socket);
    };

    // Register Connection
    Connection conn = Connection(
        client_socket,
        [&](int sender, std::vector<uint8_t> content) {
          return broadcast(sender, content);
        },
        [&](std::string name) {
          auto res = checkExists(name);
          LOG_DEBUG("Username " + name + " already exists cheking is " +
                    std::to_string(res));
          return res;
        },
        [&](int sock) { return disconnect(sock); }, this->handle);
    clients.insert(std::make_pair(client_socket, conn));
    handleMessage(client_socket);
  }

  void handleInterrupt(int signal) {
    if (signal == SIGINT) {
      this->stopflag = true;
    }
  }

  static void handleInterruptHelper(int signal) {
    if (globalInteruptHandler != nullptr) {
      globalInteruptHandler->handleInterrupt(signal);
    }
  }

  void clear() {
    LOG_ERROR("Clearing sockets...");
    close(this->server_socket);
    for (auto iter = this->clients.begin(); iter != clients.end(); ++iter) {
      close(iter->first);
    }
    close(this->epoll_fd);
    LOG_ERROR("Clear!");
  }

public:
  static Server *globalInteruptHandler;

  Server(int port, int max_connection, int max_events)
      : port(port), max_connection(max_connection), max_events(max_events),
        stopflag(false), handle(Handle()){};
  Server &operator=(const Server &x) { return *this; };

  void runServer() {
    int sockopt = 1;
    this->server_socket = mychat_serve(port, max_connection);
    setsockopt(this->server_socket, SOL_SOCKET, SO_REUSEADDR, &sockopt,
               sizeof(sockopt));
    setsockopt(this->server_socket, SOL_SOCKET, SO_REUSEPORT, &sockopt,
               sizeof(sockopt));
    registerEpoll();

    int event_count;
    epoll_event events[this->max_events];

    // Register Interrupt handler
    globalInteruptHandler = this;
    struct sigaction sa;
    sa.sa_handler = handleInterruptHelper;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);

    // Start Server
    LOG_INFO("Server starts...");

    while (true) {
      event_count = epoll_wait(epoll_fd, events, this->max_events, 100);
      if (this->stopflag == true) {
        this->clear();
        break;
      }

      if (event_count < 0) {
        EXIT_WITH_LOG_CRITICAL("Error in waiting for io events");
        this->clear();
        exit(-1);
      }

      for (int i = 0; i < event_count; i++) {
        if (events[i].data.fd == this->server_socket) {
          acceptNewClient(epoll_fd);
        } else {
          handleMessage(events[i].data.fd);
        }
      }
    };

    mychat_close(server_socket);
  }

  void handleMessage(int fd) {
    int val_read;
    uint8_t buffer[1024] = {0};
    val_read = read(fd, buffer, 1024);
    if (val_read == 0) {
      auto ntc = RecvNotice("User " + this->clients.find(fd)->second.name +
                            " get out.");
      this->broadcast(fd, this->handle.buildRecvNotice(ntc));
      this->disconnect(fd);
      return;
    } else if (val_read < 0) {
      LOG_ERROR("Error in reading from client.");
      return;
    }
    auto msg = std::vector<uint8_t>(buffer, buffer + val_read);
    this->clients.find(fd)->second.feed(msg);
  };

  void broadcast(int fd_sender, std::vector<uint8_t> msg) {
    LOG_INFO(std::string("Broadcast from ") + std::to_string(fd_sender));
    for (auto iter = this->clients.begin(); iter != clients.end(); ++iter) {
      if (iter->first != fd_sender) {
        if (mychat_send(iter->first, msg.data(), msg.size()) == -1) {
          LOG_ERROR("Send failed to" + std::to_string(fd_sender));
        };
      }
    }
  };

  bool checkExists(std::string name) {
    for (auto iter = this->clients.begin(); iter != this->clients.end();
         ++iter) {
      if (iter->second.name == name) {
        return true;
      }
    }
    return false;
  }

  void disconnect(int fd) {
    mychat_close(fd);
    clients.erase(fd);
    LOG_INFO("Client disconnected. Current connection is " +
             std::to_string(getConnectionCount()));
  }

  int getConnectionCount() { return this->clients.size(); }
};

Server *Server::globalInteruptHandler = nullptr;

int main(int argc, char **argv) { Server(9999, 99, 99).runServer(); }