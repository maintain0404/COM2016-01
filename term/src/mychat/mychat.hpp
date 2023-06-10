#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#define MYCHAT_SERVE_NUM
#define MYCHAT_ACCEPT_NUM
#define MYCHAT_CONNECT_NUM
#define MYCHAT_READ_NUM
#define MYCHAT_SEND_NUM
#define MYCHAT_CLOSE_NUM

#define MYCHAT_SERVE_ERR_SOCKET_BINDING_FAILED -1;
#define MYCHAT_SERVE_ERR_SOCKET_LISTENING_FAILED -2;

inline int mychat_serve(int port, int max_conn) {
#ifndef MYCHAT_USE_SYSCALL
  int sock;
  int sockopt = 1;
  struct sockaddr_in server_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    printf("Mychat:serve creating socket failed.\n");
    return sock;
  }
  printf("Mychat:serve creating socket success\n");

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
  setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &sockopt, sizeof(sockopt));

  if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    printf("Mychat:serve binding socket failed.\n");
    return MYCHAT_SERVE_ERR_SOCKET_BINDING_FAILED;
  };
  printf("Mychat:serve binding socket success.\n");

  if (listen(sock, max_conn) < 0) {
    printf("Mychat:serve listening failed\n");
    return MYCHAT_SERVE_ERR_SOCKET_LISTENING_FAILED;
  }

  return sock;
#else
  return syscall(MYCHAT_SERVE_NUM, dist);
#endif
};

inline int mychat_accept(int fd, __SOCKADDR_ARG addr,
                         socklen_t *__restrict size) {
  return accept(fd, addr, size);
};

#define MYCHAT_ENTER_SOCKET_CREATING_FAILED -1;
#define MYCHAT_ENTER_SOCKET_CONNECTING_FAILED -2;

inline int mychat_enter(__CONST_SOCKADDR_ARG addr, socklen_t socksize) {
#ifndef MYCHAT_USE_SYSCALL
  int res;
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    printf("Mychat:enter socket create failed\n");
    return MYCHAT_ENTER_SOCKET_CREATING_FAILED;
  }
  printf("Mychat:enter creating socket success.\n");

  res = connect(sock, addr, socksize);
  if (res < 0) {
    printf("Mychat:enter socket connect failed.\n");
    return MYCHAT_ENTER_SOCKET_CONNECTING_FAILED;
  }
  printf("Mychat:enter connection success.\n");

  return sock;
#else
  return connect(fd, addr, size);
#endif
};

inline int mychat_recv(int fd, void *buffer, size_t size) {
#ifndef MYCHAT_USE_SYSCALL
  return recv(fd, buffer, size, 0);
#else
  return syscall(MYCHAT_READ_NUM, port);
#endif
};

inline int mychat_send(int fd, const void *buffer, socklen_t size) {
#ifndef MYCHAT_USE_SYSCALL
  return send(fd, buffer, size, 0);
#else
  return syscall(MYCHAT_SEND_NUM, fd, buffer, size);
#endif
};

inline int mychat_close(int fd) { return close(fd); };