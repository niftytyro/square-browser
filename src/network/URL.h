#ifndef URL_H
#define URL_H

#include <stdbool.h>

struct URL {
  char *scheme;
  char *hostname;
  char *port;
  char *path;

  bool is_tls;
};

#endif
