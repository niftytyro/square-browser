#define _POSIX_C_SOURCE 200112L

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int main(int argc, char *argv[]) {

  struct addrinfo hints;
  struct addrinfo *server_address;
  short int status;
  int socket_fd;

  if (argc != 2) {
    printf("Usage: square <url>\n\n");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(argv[1], "80", &hints, &server_address);

  if (status != 0) {
    fprintf(stderr, "Could not getaddressinfo(): %s\n", gai_strerror(status));
    exit(2);
  }

  if (server_address->ai_family == AF_INET) {
    void *addr;
    char buffer[INET_ADDRSTRLEN];

    addr = &((struct sockaddr_in *)(server_address->ai_addr))->sin_addr;

    inet_ntop(AF_INET, addr, buffer, INET_ADDRSTRLEN);

    printf("IPv4 Address: %s\n", buffer);

  } else if (server_address->ai_family == AF_INET6) {
    void *addr;
    char buffer[INET6_ADDRSTRLEN];

    addr = &((struct sockaddr_in6 *)(server_address->ai_addr))->sin6_addr;

    inet_ntop(AF_INET6, addr, buffer, INET6_ADDRSTRLEN);

    printf("IPv6 Address: %s\n", buffer);
  }

  printf("Creating a socket now.\n");

  socket_fd = socket(server_address->ai_family, server_address->ai_socktype,
                     server_address->ai_protocol);

  printf("Connection attempt started.\n");

  status =
      connect(socket_fd, server_address->ai_addr, server_address->ai_addrlen);

  printf("status: %d", status);
  if (status != 0) {
    fprintf(stderr, "Failed to bind socket: %s", gai_strerror(status));
  }

  freeaddrinfo(server_address);
  return 0;
}

void split_url(char *url) {}

void connect_to_host() {}

char *form_http_request() { return ""; }
