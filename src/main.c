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

char *form_http_request(char *hostname, char *port, char *route,
                        char *http_version, char *method) {
  char *request;
  char *buffer;

  // TODO use port for localhost
  printf("%s", port);

  short int req_line_1_len =
      ((strlen(method) + 1 + strlen(route) + 1 + strlen("HTTP/") +
        strlen(http_version))) +
      1; // first two 1s are for spaces and last one is for \n
  short int headers_len = strlen("Host: ") + strlen(hostname) +
                          2; // the 1 is for a ':' 2 is for two '\n's

  request = calloc(req_line_1_len + headers_len, sizeof(char));
  buffer = calloc(req_line_1_len + headers_len, sizeof(char));

  sprintf(buffer, "%s %s HTTP/%s", method, route, http_version);
  request = strcat(request, buffer);
  request = strcat(request, "\n");
  sprintf(buffer, "Host: %s", hostname);
  request = strcat(request, buffer);
  sprintf(request, request, hostname);
  request = strcat(request, "\n\n");

  free(buffer);

  return request;
}

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
    fprintf(stderr, "Could not getaddressinfo(): %s\n", gai_strerror(errno));
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

  printf("status: %d\n", status);
  if (status != 0) {
    fprintf(stderr, "Failed to bind socket: %s", gai_strerror(errno));
  }

  char *request = form_http_request(argv[1], "80", "/index.html", "1.0", "GET");

  printf("------\n");
  printf("Request:\n%s", request);
  printf("------\n\n");

  unsigned long bytes_sent = send(socket_fd, request, strlen(request), 0);
  printf("Bytes sent: %lu\n", bytes_sent);
  printf("Request length: %lu\n", strlen(request));

  if (bytes_sent == (unsigned long)-1) {
    fprintf(stderr, "Error sending out the data: %s\n", gai_strerror(errno));
    exit(3);
  }

  if (bytes_sent != strlen(request)) {
    printf("ERROR: Bytes sent is less");
  }

  char *response = malloc(1024);
  unsigned long bytes_received = 0;

  printf("------\n");
  printf("Response:\n");
  int count = 0;
  while (1) {

    memset(response, 0, strlen(response));
    int new_bytes = recv(socket_fd, response, 1024, 0);
    bytes_received += new_bytes;

    if (new_bytes == -1) {
      fprintf(stderr, "Error sending out the data: %s\n", gai_strerror(errno));
      exit(3);
    } else if (new_bytes == 0) {
      printf("\nConnection closed.\n");
      break;
    } else {
      printf("%s", response);
    }

    count++;
  }

  printf("------\n\n");
  printf("Bytes received: %lu\n", bytes_received);

  freeaddrinfo(server_address);
  return 0;
}

/* void split_url(char *url) {} */

/* void connect_to_host() {} */
