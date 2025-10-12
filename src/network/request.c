#define _POSIX_C_SOURCE 200112L

#include "URL.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RESPONSE_READ_BUFFER_SIZE 1024 * 4

void make_url(char *input, struct URL *url) {
  char *scheme_delim = strstr(input, "://");
  char *port_delim;
  char *path_delim;
  int input_len = strlen(input);

  char *host_start;

  if (scheme_delim == NULL) {
    url->scheme = "http";
    host_start = input;
  } else {
    int scheme_len = scheme_delim - input + 1;
    url->scheme = malloc(scheme_len);
    url->scheme[scheme_len - 1] = '\0';
    strncpy(url->scheme, input, scheme_len - 1);
    host_start = scheme_delim + 3;
  }

  path_delim = strchr(host_start, '/');
  if (path_delim == NULL) {
    url->path = "/";
    path_delim = input + input_len;
  } else {
    int path_len = input_len - (path_delim - input);
    url->path = malloc(path_len);
    strcpy(url->path, path_delim);
  }

  port_delim = strchr(host_start, ':');
  if (port_delim == NULL) {
    url->port = "80";
    port_delim = path_delim;
  } else {
    if (port_delim < path_delim) {
      int port_len = path_delim - port_delim;
      url->port = malloc(port_len);
      url->port[port_len - 1] = '\0';
      strncpy(url->port, port_delim + 1, port_len - 1);

    } else {
      url->port = "80";
      port_delim = path_delim;
    }
  }

  int host_len = port_delim - (host_start) + 1;
  url->hostname = malloc(host_len);
  url->hostname[host_len - 1] = '\0';
  strncpy(url->hostname, host_start, host_len - 1);
}

void make_addr_from_URL(struct URL *url, struct addrinfo **_server_address) {
  struct addrinfo hints;
  int status;
  struct addrinfo *server_address;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(url->hostname, url->port, &hints, _server_address);
  server_address = *_server_address;

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
}

int open_connection(struct addrinfo *server_address) {
  int status;
  int socket_fd;

  socket_fd = socket(server_address->ai_family, server_address->ai_socktype,
                     server_address->ai_protocol);

  printf("family: %d | type: %d | protocol: %d\n", server_address->ai_family,
         server_address->ai_socktype, server_address->ai_protocol);
  printf("Connection attempt started.\n");

  status =
      connect(socket_fd, server_address->ai_addr, server_address->ai_addrlen);

  if (status != 0) {
    fprintf(stderr, "Failed to connect socket: %s\n", gai_strerror(errno));
    exit(1);
  }

  printf("status: %d\n", status);

  return socket_fd;
}

char *form_http_request(struct URL *url, char *method, char *http_version) {
  char *request;
  char *buffer;

  printf("%s\n", url->port);

  short int req_line_1_len =
      ((strlen(method) + 1 + strlen(url->path) + 1 + strlen("HTTP/") +
        strlen(http_version))) +
      1; // first two 1s are for spaces and last one is for \n
  short int headers_len = strlen("Host: ") + strlen(url->hostname) +
                          2; // the 1 is for a ':' 2 is for two '\n's

  request = calloc(req_line_1_len + headers_len, sizeof(char));
  buffer = calloc(req_line_1_len + headers_len, sizeof(char));

  sprintf(buffer, "%s %s HTTP/%s", method, url->path, http_version);
  request = strcat(request, buffer);
  request = strcat(request, "\n");
  sprintf(buffer, "Host: %s", url->hostname);
  request = strcat(request, buffer);
  sprintf(request, request, url->hostname);
  request = strcat(request, "\n\n");

  free(buffer);

  printf("------\n");
  printf("Request:\n%s", request);
  printf("------\n");

  return request;
}

void send_request(int socket_fd, char *request) {
  printf("Sending request to %d\n", socket_fd);
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
}

void read_response(int socket_fd, char **response_buffer) {
  *response_buffer = malloc(1);
  unsigned long bytes_received = 0;

  printf("\n------\n");
  printf("Response:\n");
  int count = 0;

  while (1) {
    *response_buffer =
        realloc(*response_buffer, RESPONSE_READ_BUFFER_SIZE * (count + 1));

    memset(*response_buffer + count * RESPONSE_READ_BUFFER_SIZE, 0,
           RESPONSE_READ_BUFFER_SIZE);

    int new_bytes = recv(socket_fd, *response_buffer,
                         RESPONSE_READ_BUFFER_SIZE * (count + 1), 0);
    bytes_received += new_bytes;

    if (new_bytes == -1) {
      fprintf(stderr, "Error reading the data: %s\n", gai_strerror(errno));
      exit(3);
    } else if (new_bytes == 0) {
      printf("\nConnection closed.\n");
      break;
    } else {
      printf("%s", *response_buffer);
    }

    count++;
  }

  printf("------\n\n");
  printf("Bytes received: %lu\n", bytes_received);
}
