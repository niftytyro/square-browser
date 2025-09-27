#define _POSIX_C_SOURCE 200112L

#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "./network/request.h"

int main(int argc, char *argv[]) {
  struct addrinfo *server_address;
  int socket_fd;
  char *response;

  if (argc != 2) {
    printf("Usage: square <url>\n\n");
    exit(1);
  }

  struct URL url = {
      .hostname = NULL, .path = NULL, .port = NULL, .scheme = NULL};

  split_url(argv[1], &url);
  make_addr_from_URL(&url, &server_address);
  socket_fd = open_connection(server_address);
  char *request = form_http_request(&url, "GET", "1.0");
  send_request(socket_fd, request);
  read_response(socket_fd, &response);

  free(request);
  free(response);
  freeaddrinfo(server_address);

  return 0;
}
