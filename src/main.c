#define _POSIX_C_SOURCE 200112L

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: square <url>\n\n");
    exit(1);
  }

  struct addrinfo hints;
  struct addrinfo *serverInfo;
  short int status;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  status = getaddrinfo(argv[1], "3490", &hints, &serverInfo);

  if (status != 0) {
    fprintf(stderr, "Could not getaddressinfo(): %s\n", gai_strerror(status));
    exit(2);
  }

  if (serverInfo->ai_family == AF_INET) {
    void *addr;
    char buffer[INET_ADDRSTRLEN];

    addr = &((struct sockaddr_in *)(serverInfo->ai_addr))->sin_addr;

    inet_ntop(AF_INET, addr, buffer, INET_ADDRSTRLEN);

    printf("IPv4 Address: %s\n", buffer);

  } else if (serverInfo->ai_family == AF_INET6) {
    void *addr;
    char buffer[INET6_ADDRSTRLEN];

    addr = &((struct sockaddr_in6 *)(serverInfo->ai_addr))->sin6_addr;

    inet_ntop(AF_INET6, addr, buffer, INET6_ADDRSTRLEN);

    printf("IPv6 Address: %s\n", buffer);
  }

  freeaddrinfo(serverInfo);
  return 0;
}
