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

  if (argc != 2) {
    printf("Usage: square <url>\n\n");
    exit(1);
  }

  fetch_response_from_url(argv[1]);

  return 0;
}
