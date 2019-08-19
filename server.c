#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG

int main(int argc, char *argv[])
{
  int chunksize = 100 * 1024; /* 100KiB */
  int sockfd_l, sockfd_a;
  struct sockaddr_in addr_s = {0}, addr_c = {0};
  int optval;
  socklen_t addrlen;
  char *buf;

  if (argc < 1 || 2 < argc) {
    fprintf(stderr, "usage: %s [chunksize(in KiB)(default=100)]\n", argv[0]);
    return 1;
  }
  if (argc == 2)
    chunksize = atoi(argv[1]) * 1024; /* byte */

#ifdef DEBUG
  fprintf(stderr, "chunksize = %d KiB\n", chunksize / 1024);
#endif

  sockfd_l = socket(AF_INET, SOCK_STREAM, 0);

  optval = 1;
  setsockopt(sockfd_l, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(optval));

  addr_s.sin_family = AF_INET;
  addr_s.sin_port = htons(50000);
  addr_s.sin_addr.s_addr = INADDR_ANY;

  bind(sockfd_l, (struct sockaddr *)&addr_s, sizeof(addr_s));
  listen(sockfd_l, 5);

  buf = calloc(chunksize, sizeof(*buf));

  while (1) {
    addrlen = sizeof(addr_c);
    sockfd_a = accept(sockfd_l, (struct sockaddr *)&addr_c, &addrlen);
    printf("connection accepted\n");

    while (write(sockfd_a, buf, chunksize) >= 0)
      ;

    close(sockfd_a);
  }

  close(sockfd_l);
  return 0;
}
