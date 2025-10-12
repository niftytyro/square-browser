#ifndef SQUARE_NETWORK_UTILS
#define SQUARE_NETWORK_UTILS

#include <netdb.h>

char *derive_hostname_from_addrinfo(struct addrinfo *server_address);

#endif
