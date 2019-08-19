#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define MAXSLEEP (1000 * 1000) /* 1sec */
#define DEBUG

void pauserecv(struct timeval *start, struct timeval *now, long amount, int rate);

int main(int argc, char *argv[])
{
  int duration, rate, chunksize = 10 * 1024; /* 10KiB */
  char *deststr, *buf;
  int sockfd;
  struct sockaddr_in addr_s = {0};
  unsigned int **addr_list;
  int optval, n;
  long received = 0;
  struct timespec res;
  struct timeval start, end, now, diff;
  suseconds_t diff_in_usec;

  /* parse arguments */
  if (argc < 4 || 5 < argc) {
    fprintf(stderr, "usage: %s <destaddr> <duration(in sec)> <rate(in Kibps)> [chunksize(in KiB)(default=10)]\n", argv[0]);
    return 1;
  }
  deststr = argv[1];
  duration = atoi(argv[2]);
  rate = atoi(argv[3]);
  if (argc == 5)
    chunksize = atoi(argv[4]) * 1024; /* byte */

#ifdef DEBUG
  fprintf(stderr, "duration = %d sec, rate = %d Kibps, chunksize = %d KiB\n", duration, rate, chunksize / 1024);
#endif

  /* create socket */
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return 1;
  }

  /* connect to server */
  addr_s.sin_family = AF_INET;
  addr_s.sin_port = htons(50000);
  addr_s.sin_addr.s_addr = inet_addr(deststr);

  if (addr_s.sin_addr.s_addr != INADDR_NONE) {
    if (connect(sockfd, (struct sockaddr *)&addr_s, sizeof(addr_s)) != 0) {
      perror("connect");
      return 1;
    }
  } else {
    struct hostent *host;

    if ((host = gethostbyname(deststr)) == NULL) {
      fprintf(stderr, "%s: %s\n", hstrerror(h_errno), deststr);
      close(sockfd);
      return 1;
    }

    for (addr_list = (unsigned int **)host->h_addr_list; *addr_list != NULL; addr_list++) {
      addr_s.sin_addr.s_addr = **addr_list;

      if (connect(sockfd, (struct sockaddr *)&addr_s, sizeof(addr_s)) == 0)
	break;
    }

    if (*addr_list == NULL) {
      perror("connect");
      close(sockfd);
      return 1;
    }
  }

#if 0
  optval = chunksize;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const void *)&optval, sizeof(optval)) < 0) {
    perror("setsockopt(SO_RCVBUF)");
    return 1;
  }
#endif

  buf = calloc(chunksize, sizeof(*buf));

  clock_gettime(CLOCK_MONOTONIC, &res);
  start.tv_sec = res.tv_sec; start.tv_usec = res.tv_nsec / 1000;
  end = start;
  end.tv_sec += duration;

  /* receive loop */
  while (1) {
    if ((n = read(sockfd, buf, chunksize)) < 0) {
      perror("read");
      break;
    }
    received += n;

    clock_gettime(CLOCK_MONOTONIC, &res);
    now.tv_sec = res.tv_sec; now.tv_usec = res.tv_nsec / 1000;
    timersub(&now, &start, &diff);
    diff_in_usec = diff.tv_sec * 1000 * 1000 + diff.tv_usec;
#ifdef DEBUG
    fprintf(stderr, "received = %ld bytes, diff = %ld usec, %.3f Kibps = %.3f Mibps\n",
	   received, diff_in_usec, received * 8 / (diff_in_usec / 1000 / 1000.0) / 1024.0, received * 8 / (diff_in_usec / 1000 / 1000.0) / 1024 / 1024.0);
#endif

    if (timercmp(&now, &end, >=))
      break;

    pauserecv(&start, &now, received + chunksize, rate); /* add final chunk at first */
  }

#ifdef DEBUG
  fprintf(stderr, "received = %ld bytes, %.3f Kibps = %.3f Mibps\n", received, received * 8 / duration / 1024.0, received * 8 / duration / 1024 / 1024.0);
#endif

  close(sockfd);
  return 0;
}

/* pause recv for target rate */
void pauserecv(struct timeval *start, struct timeval *now, long received, int rate)
{
  struct timeval diff;
  suseconds_t actual_in_usec, expected_in_usec, sleep_in_usec;

  if (rate <= 0) /* avoid divison by zero */
    return;

  timersub(now, start, &diff);
  actual_in_usec = diff.tv_sec * 1000 * 1000 + diff.tv_usec;
  expected_in_usec = received * 1000 * 1000 / (rate * 1024 / 8);

  if (expected_in_usec > actual_in_usec) {
    /* fast */
    sleep_in_usec = expected_in_usec - actual_in_usec;

    if (sleep_in_usec > 0) {
      if (sleep_in_usec < MAXSLEEP) {
#ifdef DEBUG
	fprintf(stderr, "fast -> %.3f msec sleep\n", sleep_in_usec / 1000.0);
#endif
      } else {
#ifdef DEBUG
	fprintf(stderr, "fast, but %.3f msec is too big -> %.3f msec sleep\n", sleep_in_usec / 1000.0, MAXSLEEP / 1000.0);
#endif
	sleep_in_usec = MAXSLEEP;
      }

      /* sleep */
      usleep(sleep_in_usec);

    } else {
#ifdef DEBUG
      fprintf(stderr, "fast, but sleep <= 0 -> no sleep\n");
#endif
    }

  } else {
    /* slow */
#ifdef DEBUG
    fprintf(stderr, "slow -> no sleep\n");
#endif
  }
}
