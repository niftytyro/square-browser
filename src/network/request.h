#ifndef REQUEST
#define REQUEST

#include "URL.h"

void make_url(char *input,  struct URL *url);
void make_addr_from_URL(struct URL *url, struct addrinfo **server_address);
char *form_http_request(struct URL *url, char *method, char *http_version);
int open_connection(struct addrinfo *server_address);
void send_request(int socket_fd, char *request);
void read_response(int socket_fd, char **response);

#endif
