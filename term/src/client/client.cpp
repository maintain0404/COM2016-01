#include "src/logging/logging.hpp"
#include "src/mychat/mychat.hpp"
#include "src/protocol/packet.hpp"
#include "src/protocol/protocol.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <error.h>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <variant>
#include <vector>

Handle handle = Handle();

void enterServer(int socket, std::string name) {
  Header header;
  header.version = 1;
  header.type = MessageType::ENTER;
  header.size = name.length();

  std::vector<uint8_t> packet;
  packet.resize(sizeof(Header) + name.length());
  std::memcpy(packet.data(), &header, sizeof(Header));
  std::memcpy(packet.data() + sizeof(Header), name.c_str(), name.length());

  if (mychat_send(socket, packet.data(), packet.size()) < 0) {
    EXIT_WITH_LOG_CRITICAL("Error in sending data to server");
  }
}

void sendMessage(int socket, std::string message) {
  Header header(MESSAGE, message.size());

  std::vector<uint8_t> packet;
  packet.resize(sizeof(Header) + message.length());
  std::memcpy(packet.data(), &header, sizeof(Header));
  std::memcpy(packet.data() + sizeof(Header), message.c_str(),
              message.length());

  if (mychat_send(socket, packet.data(), packet.size()) < 0) {
    EXIT_WITH_LOG_CRITICAL("Error in sending data to server");
  }
}

void handleMessage(Data buffer) {
  if (buffer.size() < sizeof(Header)) {
    throw HandleReturn::SHORTER_THAN_HEADER;
  }
  auto recv = handle.parseRecv(buffer);
  if (std::holds_alternative<RecvMessage>(recv)) {
    auto msg = std::get<RecvMessage>(recv);

    std::cout << "\033[33m" << msg.sender_name << "\033[0m : " << msg.content
              << std::endl;
  } else if (std::holds_alternative<RecvNotice>(recv)) {
    auto msg = std::get<RecvNotice>(recv);

    std::cout << "\033[36mNOTICE"
              << "\033[0m : " << msg.content << std::endl;
  }
}

int connectServer(sockaddr_in *server_addr) {
  int socket_fd;

  socket_fd = mychat_enter((sockaddr *)server_addr, sizeof(*server_addr));

  if (socket_fd <= 0) {
    EXIT_WITH_LOG_CRITICAL("Error in connecting server");
    exit(-1);
  }
  fcntl(socket_fd, F_SETFL, O_NONBLOCK);
  LOG_INFO("Creating connection succeed.");

  return socket_fd;
}

void infinite(int socket) {
  int bytes_received;
  char buffer[1024] = {0};
  std::string line;
  RecvMessage msg;

  fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
  fcntl(STDOUT_FILENO, F_SETFL, O_NONBLOCK);

  while (true) {
    bytes_received = read(STDIN_FILENO, buffer, 1024);
    if (bytes_received > 0) {
      sendMessage(socket, std::string(buffer));
    }

    memset(buffer, 0, 1024);
    bytes_received = mychat_recv(socket, buffer, 1024);
    if (bytes_received > 0) {
      handleMessage(Data(buffer, buffer + 1024));
    } else if (bytes_received == 0) {
      LOG_ERROR("Server closed.");
      break;
    }
  }
}

void runClient(std::string address, int port, std::string name) {
  int socket_fd;
  struct sockaddr_in server_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(address.c_str());
  server_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, address.c_str(), &server_addr.sin_addr) <= 0) {
    LOG_ERROR("Error in converting server address")
    EXIT
  }

  socket_fd = connectServer(&server_addr);
  enterServer(socket_fd, name);
  infinite(socket_fd);
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    LOG_ERROR("Invalid arguments. \n"
              "Use like sample below\n\n"
              "client [HOST] [PORT]")
    EXIT
  }
  _LOG_LEVEL = ERROR;
  auto port = atoi(argv[2]);
  if (port <= 1 || port >= 65536) {
    LOG_ERROR("Invalid ports. Use port between 1 to 65535")
    EXIT
  }
  runClient(argv[1], port, std::string(argv[3]));
}
