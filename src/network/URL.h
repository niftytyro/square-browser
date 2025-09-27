#ifndef URL_H
#define URL_H

struct URL {
  char *scheme;
  char *hostname;
  char *port;
  char *path;
};

#endif
