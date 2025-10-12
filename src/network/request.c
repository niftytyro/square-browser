#define _POSIX_C_SOURCE 200112L

#include "URL.h"
#include "utils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

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

int get_server_addrinfo(struct URL *url, struct addrinfo **server_address) {
  struct addrinfo hints;
  int status;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(url->hostname, url->port, &hints, server_address);

  if (status != 0) {
    fprintf(stderr, "Could not getaddrinfo().\n");
    return 1;
  }

  if ((*server_address)->ai_family == AF_INET) {
    void *addr;
    char buffer[INET_ADDRSTRLEN];

    addr = &((struct sockaddr_in *)((*server_address)->ai_addr))->sin_addr;
    inet_ntop(AF_INET, addr, buffer, INET_ADDRSTRLEN);

    printf("IPv4 Address: %s\n", buffer);
  } else if ((*server_address)->ai_family == AF_INET6) {
    void *addr;
    char buffer[INET6_ADDRSTRLEN];

    addr = &((struct sockaddr_in6 *)((*server_address)->ai_addr))->sin6_addr;
    inet_ntop(AF_INET, addr, buffer, INET_ADDRSTRLEN);

    printf("IPv6 Address: %s\n", buffer);
  }

  return 0;
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

/* void send_request(int socket_fd, char *request) { */
/*   printf("Sending request to %d\n", socket_fd); */
/*   unsigned long bytes_sent = send(socket_fd, request, strlen(request), 0); */
/*   printf("Bytes sent: %lu\n", bytes_sent); */
/*   printf("Request length: %lu\n", strlen(request)); */

/*   if (bytes_sent == (unsigned long)-1) { */
/*     fprintf(stderr, "Error sending out the data: %s\n", gai_strerror(errno));
 */

/*     exit(2); */
/*   } */

/*   if (bytes_sent != strlen(request)) { */
/*     printf("ERROR: Bytes sent is less"); */
/*   } */
/* } */

/* void read_response(int socket_fd, char **response_buffer) { */
/*   *response_buffer = malloc(1); */
/*   unsigned long bytes_received = 0; */

/*   printf("\n------\n"); */
/*   printf("Response:\n"); */
/*   int count = 0; */

/*   while (1) { */
/*     *response_buffer = */
/*         realloc(*response_buffer, RESPONSE_READ_BUFFER_SIZE * (count + 1));
 */

/*     memset(*response_buffer + count * RESPONSE_READ_BUFFER_SIZE, 0, */
/*            RESPONSE_READ_BUFFER_SIZE); */

/*     int new_bytes = recv(socket_fd, *response_buffer, */
/*                          RESPONSE_READ_BUFFER_SIZE * (count + 1), 0); */
/*     bytes_received += new_bytes; */

/*     if (new_bytes == -1) { */
/*       fprintf(stderr, "Error reading the data: %s\n", gai_strerror(errno));
 */
/*       exit(2); */
/*     } else if (new_bytes == 0) { */
/*       printf("\nConnection closed.\n"); */
/*       break; */
/*     } else { */
/*       printf("%s", *response_buffer); */
/*     } */

/*     count++; */
/*   } */

/*   printf("------\n\n"); */
/*   printf("Bytes received: %lu\n", bytes_received); */
/* } */

void cleanup(SSL_CTX *ctx, SSL *ssl) {
  ERR_print_errors_fp(stderr);
  SSL_free(ssl);
  SSL_CTX_free(ctx);
}

int make_socket(struct addrinfo *server_address, int *socket_fd) {
  *socket_fd = socket(server_address->ai_family, server_address->ai_socktype,
                      server_address->ai_protocol);

  if (socket_fd == NULL) {
    fprintf(stderr, "Error creating a socket.\n");
    return 1;
  }

  printf("family: %d | type: %d | protocol: %d\n", server_address->ai_family,
         server_address->ai_socktype, server_address->ai_protocol);
  printf("Connection attempt started.\n");

  return 0;
}

int connect_socket(struct addrinfo *server_address, int socket_fd) {
  int status =
      connect(socket_fd, server_address->ai_addr, server_address->ai_addrlen);

  if (status != 0) {
    fprintf(stderr, "Failed to connect socket: %s\n", gai_strerror(errno));
    return 1;
  }

  return 0;
}

int prepare_SSL(SSL_CTX **ctx, SSL **ssl, BIO **bio) {
  *ctx = SSL_CTX_new(TLS_client_method());

  if (ctx == NULL) {
    fprintf(stderr, "Error creating an OpenSSL context.\n");
    return 1;
  }

  // Context configuration
  SSL_CTX_set_verify(*ctx, SSL_VERIFY_PEER, NULL);
  if (!SSL_CTX_set_default_verify_paths(*ctx)) {
    fprintf(stderr, "Failed to set the default trusted certificate store.\n");
    return 1;
  }
  if (!SSL_CTX_set_min_proto_version(*ctx, TLS1_2_VERSION)) {
    fprintf(stderr, "Failed to set the minimum TLS protocol version.\n");
    return 1;
  }

  // Create the SSL object
  *ssl = SSL_new(*ctx);
  if (ssl == NULL) {
    fprintf(stderr, "Failed to create a new SSL object.\n");
    return 1;
  }

  *bio = BIO_new(BIO_s_socket());
  if (bio == NULL) {
    fprintf(stderr, "Failed to creata a bio.\n");
    return 1;
  }

  return 0;
}

int attach_socket_to_ssl(struct addrinfo *server_address, SSL *ssl, BIO *bio,
                         int socket_fd) {
  char *hostname = derive_hostname_from_addrinfo(server_address);

  BIO_set_fd(bio, socket_fd, BIO_CLOSE);
  SSL_set_bio(ssl, bio, bio);

  if (!SSL_set_tlsext_host_name(ssl, hostname)) {
    fprintf(stderr, "Failed to set the SNI hostname.\n");
    return 1;
  }
  if (!SSL_set1_host(ssl, hostname)) {
    fprintf(stderr, "Failed to set the certificate verification hostname.\n");
    return 1;
  }

  return 0;
}

int ssl_handshake(SSL *ssl) {
  if (SSL_connect(ssl) < 1) {
    fprintf(stderr, "Failed to connect to the server");
    if (SSL_get_verify_result(ssl) != X509_V_OK) {
      const char *error_msg =
          X509_verify_cert_error_string(SSL_get_verify_result(ssl));
      fprintf(stderr, "Verification error: %s\n", error_msg);
    }
    return 1;
  }

  return 0;
}

int send_request(int socket_fd, SSL *ssl, char *request, struct URL *url) {
  size_t bytes_written;

  if (url->is_tls) {
    if (!SSL_write_ex(ssl, request, strlen(request), &bytes_written)) {
      fprintf(stderr, "Failed to write to ssl socket.\n");
      return 1;
    }
  } else {
    bytes_written = send(socket_fd, request, strlen(request), 0);
    if (bytes_written == (unsigned long)-1) {
      fprintf(stderr, "Failed to write to TCP socket.\n");
      return 1;
    }
  }

  return 0;
}

int read_response(int socket_fd, SSL *ssl, struct URL *url) {
  // TODO use dynamic array here
  char buffer[RESPONSE_READ_BUFFER_SIZE];
  size_t bytes_read = 0;

  if (url->is_tls) {
    while (SSL_read_ex(ssl, buffer, sizeof(buffer), &bytes_read)) {
      fwrite(buffer, 1, bytes_read, stdout);
    }

    if (SSL_get_error(ssl, 0) != SSL_ERROR_ZERO_RETURN) {
      fprintf(stderr, "Failed to read remaining response.\n");
      return 1;
    }
  } else {
    while (1) {
      bytes_read = recv(socket_fd, buffer, sizeof buffer, 0);

      if (bytes_read == (unsigned long)-1) {
        fprintf(stderr, "Failed to read remaining response.");
        return 1;
      }
      if (bytes_read == 0) {
        break;
      }

      fwrite(buffer, 1, bytes_read, stdout);
    }
  }

  return 0;
}

int fetch_response_from_url(char *_url) {
  int socket_fd;
  /* char *request, *response; */
  char *request;
  SSL_CTX *ctx;
  SSL *ssl;
  BIO *bio;
  struct addrinfo *server_address;
  struct URL url = {
      .hostname = NULL, .path = NULL, .port = NULL, .scheme = NULL};

  make_url(_url, &url);

  if (url.is_tls) {
    prepare_SSL(&ctx, &ssl, &bio);
  } else {
    ctx = NULL, ssl = NULL, bio = NULL;
  }

  // TODO solve for IPv6
  get_server_addrinfo(&url, &server_address);
  make_socket(server_address, &socket_fd);
  connect_socket(server_address, socket_fd);
  if (url.is_tls) {
    attach_socket_to_ssl(server_address, ssl, bio, socket_fd);
    ssl_handshake(ssl);
  }

  request = form_http_request(&url, "GET", "1.0");

  send_request(socket_fd, ssl, request, &url);
  /* read_response(socket_fd, ssl, &response, &url); */
  read_response(socket_fd, ssl, &url);

  return 0;
}

int open_connection_ssl() {
  char *hostname = "www.google.com";
  char *port = "443";
  int family = AF_INET;

  struct URL url = {
      .scheme = "https", .hostname = hostname, .port = port, .path = "/"};

  SSL_CTX *ctx = NULL;
  SSL *ssl = NULL;
  int sock = -1;
  BIO *bio;
  BIO_ADDRINFO *res;
  const BIO_ADDRINFO *ai = NULL;

  // Create SSL Context
  ctx = SSL_CTX_new(TLS_client_method());
  if (ctx == NULL) {
    fprintf(stderr, "Error creating an OpenSSL context.\n");
    cleanup(ctx, ssl);
    return 1;
  }

  // Context configuration
  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
  if (!SSL_CTX_set_default_verify_paths(ctx)) {
    fprintf(stderr, "Failed to set the default trusted certificate store.\n");
    cleanup(ctx, ssl);
    return 1;
  }
  if (!SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION)) {
    fprintf(stderr, "Failed to set the minimum TLS protocol version.\n");
    cleanup(ctx, ssl);
    return 1;
  }

  // Create the SSL object
  ssl = SSL_new(ctx);
  if (ssl == NULL) {
    fprintf(stderr, "Failed to create a new SSL object.\n");
    cleanup(ctx, ssl);
    return 1;
  }

  if (!BIO_lookup_ex(hostname, port, BIO_LOOKUP_CLIENT, family, SOCK_STREAM, 0,
                     &res)) {
    fprintf(stderr, "Failed to lookup host.\n");
    cleanup(ctx, ssl);
    return 1;
  }

  for (ai = res; ai != NULL; ai = BIO_ADDRINFO_next(ai)) {
    sock = BIO_socket(BIO_ADDRINFO_family(ai), SOCK_STREAM, 0, 0);
    if (sock == -1) {
      continue;
    }

    if (!BIO_connect(sock, BIO_ADDRINFO_address(ai), BIO_SOCK_NODELAY)) {
      BIO_closesocket(sock);
      sock = -1;
      continue;
    }

    break;
  }

  bio = BIO_new(BIO_s_socket());
  if (bio == NULL) {
    fprintf(stderr, "Failed to creata a bio.\n");
    cleanup(ctx, ssl);
    return 1;
  }

  BIO_set_fd(bio, sock, BIO_CLOSE);
  SSL_set_bio(ssl, bio, bio);

  if (!SSL_set_tlsext_host_name(ssl, hostname)) {
    fprintf(stderr, "Failed to set the SNI hostname.\n");
    cleanup(ctx, ssl);
    return 1;
  }
  if (!SSL_set1_host(ssl, hostname)) {
    fprintf(stderr, "Failed to set the certificate verification hostname.\n");
    cleanup(ctx, ssl);
    return 1;
  }

  if (SSL_connect(ssl) < 1) {
    fprintf(stderr, "Failed to connect to the server");
    if (SSL_get_verify_result(ssl) != X509_V_OK) {
      const char *error_msg =
          X509_verify_cert_error_string(SSL_get_verify_result(ssl));
      fprintf(stderr, "Verification error: %s\n", error_msg);
    }
    cleanup(ctx, ssl);
    return 1;
  }

  size_t written;
  char *request = form_http_request(&url, "GET", "1.0");

  if (!SSL_write_ex(ssl, request, strlen(request), &written)) {
    fprintf(stderr, "Failed to write to ssl socket.\n");
    cleanup(ctx, ssl);
    return 1;
  }

  size_t bytes_read;
  char buf[60];

  while (SSL_read_ex(ssl, buf, sizeof(buf), &bytes_read)) {
    fwrite(buf, 1, bytes_read, stdout);
  }

  if (SSL_get_error(ssl, 0) != SSL_ERROR_ZERO_RETURN) {
    fprintf(stderr, "Failed to read remaining data.\n");
    cleanup(ctx, ssl);
    return 1;
  }

  int ret = SSL_shutdown(ssl);
  if (ret < 1) {
    fprintf(stderr, "Error shutting down.\n");
    cleanup(ctx, ssl);
    return 1;
  }

  BIO_ADDRINFO_free(res);
  return 0;
}
