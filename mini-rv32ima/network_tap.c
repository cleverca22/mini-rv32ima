#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "network.h"

static int tun_fd;
static pthread_t reader;

static void *read_thread(void *callback) {
  rx_callback cb = callback;
  uint8_t *buffer = malloc(3000);
  while (true) {
    uint32_t size = read(tun_fd, buffer, 3000);
    cb(buffer+4, size-4);
  }
  free(buffer);
}

bool network_init(rx_callback callback) {
  tun_fd = open("/dev/net/tun", O_RDWR);
  if (tun_fd < 0) {
    perror("unable to open tun device");
    return true;
  }

  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, "tap0", IFNAMSIZ);
  ifr.ifr_flags = IFF_TAP;
  int ret = ioctl(tun_fd, TUNSETIFF, &ifr);
  if (ret < 0) {
    perror("unable to TUNSETIFF");
    return true;
  }

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&reader, &attr, &read_thread, callback);
  pthread_attr_destroy(&attr);
  return false;
}

void network_transmit(uint8_t *packet, uint32_t size) {
  uint8_t *dummy = malloc(size+4);
  memset(dummy, 0, 4);
  memcpy(dummy+4, packet, size);
  write(tun_fd, dummy, size+4);
  free(dummy);
}
