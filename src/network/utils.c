#define _POSIX_C_SOURCE 200112L

#include <netdb.h>
#include <netinet/in.h>

char *derive_hostname_from_addrinfo(struct addrinfo *server_address) {
  return server_address->ai_canonname;
}
