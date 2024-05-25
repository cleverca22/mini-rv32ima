#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "network.h"

static int tun_fd;
static pthread_t reader;

static const int mtu = 9000 + 1000;

static void *read_thread(void *callback) {
  rx_callback cb = callback;
  uint8_t *buffer = malloc(mtu);
  while (true) {
    int32_t size = read(tun_fd, buffer, mtu);
    if (size == -1) {
      if (errno == EBADF) break;
      perror("unable to read TUN");
      break;
    }
    cb(buffer, size);
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
  // IFF_NO_CARRIER in flags keeps the link down on open
  // TUNSETCARRIER to change the link later
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
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

void network_teardown() {
  close(tun_fd);
}

void network_transmit(uint8_t *packet, uint32_t size) {
  write(tun_fd, packet, size);
}
