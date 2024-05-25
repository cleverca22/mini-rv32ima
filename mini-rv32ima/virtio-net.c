#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "virtio.h"
#include "plic.h"
#include "network.h"

#define VIRTIO_NET_F_MTU 3
#define VIRTIO_NET_F_MAC 5
#define VIRTIO_NET_F_SPEED_DUPLEX 63

static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

struct virtio_net_config {
  uint8_t mac[6];             // only valid if VIRTIO_NET_F_MAC is set
  uint16_t status;            // only valid if VIRTIO_NET_F_STATUS is set
  // 8
  uint16_t max_virtqueue_pairs; // VIRTIO_NET_F_MQ or VIRTIO_NET_F_RSS
  uint16_t mtu;               // VIRTIO_NET_F_MTU
  uint32_t speed;             // in mbit VIRTIO_NET_F_SPEED_DUPLEX
  // 16
  uint8_t duplex;             // 1=full, 0=half, VIRTIO_NET_F_SPEED_DUPLEX
  uint8_t rss_max_key_size;
  uint16_t rss_max_indirection_table_length;
  uint32_t supported_hash_types;
};

static uint64_t bytes_sent = 0;
static uint64_t packets_sent = 0;
static uint64_t bytes_received = 0;
static uint64_t packets_received = 0;
static uint64_t rx_dropped = 0;

struct virtio_net_hdr {
  uint8_t flags;
  uint8_t gso_type;
  uint16_t hdr_len;
  uint16_t csum_start;
  uint16_t csum_offset;
  uint16_t num_buffers;
  uint32_t hash_value;
  uint16_t hash_report;
  uint16_t padding;
  uint8_t packet[0];
} __attribute__((packed));

static const struct virtio_net_config cfg = {
  .mac = {0x74, 0x56, 0x3c, 0x44, 0x7b, 0xb3 },
  .mtu = 9000,
  .speed = 1000,
  .duplex = 1,
};

struct rx_queue {
  struct virtio_device *dev;
  virtio_chain *chain;
  uint16_t start_idx;
  struct rx_queue *next;
};

static struct rx_queue *queue_head = NULL;

static uint32_t virtio_net_config_load(struct virtio_device *dev, uint32_t offset) {
  uint32_t ret = 0;
  if (offset < sizeof(cfg)) { // TODO, 32bit read may leak 3 bytes past array
    memcpy(&ret, ((uint8_t*)&cfg) + offset, 4);
  }
  printf("virtio_net_config_load(%p, %d) == 0x%x\n", dev, offset, ret);
  return ret;
}

static void hexdump_packet(uint8_t *packet, uint32_t len) {
  for (int i=0; i<len; i++) {
    if ((i % 16) == 0) printf("0x%2x: ", i);

    uint8_t t = *((uint8_t*)(packet + i));
    printf("%02x ", t);
    if ((i % 16) == 15) printf("\n");
  }
  printf("\n");
}

static void virtio_net_config_store(struct virtio_device *dev, uint32_t offset, uint32_t val) {
  printf("virtio_net_config_store(%p, %d, %d)\n", dev, offset, val);
}

static void virtio_process_rx(struct virtio_device *dev, virtio_chain *chain, uint16_t start_idx) {
  //virtio_dump_chain(chain);

  struct rx_queue *node = malloc(sizeof(struct rx_queue));
  node->dev = dev;
  node->chain = chain;
  node->start_idx = start_idx;
  node->next = NULL;

  pthread_mutex_lock(&queue_mutex);
  // TODO, finding the tail is cpu intensive
  if (queue_head) {
    struct rx_queue *tail = queue_head;

    while (tail->next)
      tail = tail->next;

    tail->next = node;
  } else {
    queue_head = node;
  }
  pthread_mutex_unlock(&queue_mutex);
}

static void virtio_net_print_hdr(const struct virtio_net_hdr *hdr) {
  printf("  flags: %d\n", hdr->flags);
  printf("  gso_type: %d\n", hdr->gso_type);
  printf("  hdr_len: %d vs %ld\n", hdr->hdr_len, sizeof(*hdr));
}

static void virtio_process_tx(struct virtio_device *dev, virtio_chain *chain, uint16_t start_idx) {
  //puts("virtio_process_tx");
  //virtio_dump_chain(chain);
  assert(chain->chain_length == 1);
  //printf("tx 0x%lx, length=%d\n", (uint64_t)chain[0].message, chain[0].message_len);
  struct virtio_net_hdr *hdr = chain->chain[0].message;
  //virtio_net_print_hdr(hdr);

  // TODO, figure out what the 12 byte prefix is
  uint8_t *packet = chain->chain[0].message + 12;
  uint32_t packet_size = chain->chain[0].message_len - 12;
  //hexdump_packet(chain[0].message, 12);
  //hexdump_packet(packet, packet_size);
  packets_sent++;
  bytes_sent += packet_size;
  network_transmit(packet, packet_size);
  virtio_flag_completion(dev, chain, 1, start_idx, false);
}

static void virtio_net_process_command(struct virtio_device *dev, virtio_chain *chain, int queue, uint16_t start_idx) {
  //printf("virtio_net_process_command(%p, %p, length %d, queue %d, %d)\n", dev, chain, chain_length, queue, start_idx);
  switch (queue) {
  case 0: // receive, host->guest
    virtio_process_rx(dev, chain, start_idx);
    break;
  case 1: // transmit, guest->host
    virtio_process_tx(dev, chain, start_idx);
    break;
  default:
    assert(0);
  }
}

static const uint32_t feature_array[] = {
  [0] = (1<<VIRTIO_NET_F_MTU) | (1<<VIRTIO_NET_F_MAC),
  [1] = 1 | (1 << (VIRTIO_NET_F_SPEED_DUPLEX-32)),
};

static const int queue_sizes[] = {
  [0] = 4096,
  [1] = 64,
};

static const virtio_device_type virtio_net_type = {
  .device_type = 1,
  .queue_count = 2, // 3 if VIRTIO_NET_F_CTRL_VQ, more if multi-queue
  .queue_sizes = queue_sizes,
  .feature_array = feature_array,
  .feature_array_length = sizeof(feature_array) / sizeof(feature_array[0]),
  .config_load = virtio_net_config_load,
  .config_store = virtio_net_config_store,
  .process_command = virtio_net_process_command,
};

static void callback(uint8_t *packet, uint32_t size) {
  //printf("rx %d\n", size);
  //hexdump_packet(packet, size);
  packets_received++;
  bytes_received += size;

  pthread_mutex_lock(&queue_mutex);
  struct rx_queue *node = queue_head;
  if (node) {
    queue_head = node->next;
    pthread_mutex_unlock(&queue_mutex);

    //virtio_dump_chain(node->chain);
    virtio_chain_zero(node->chain, 12);
    virtio_chain_write(node->chain, packet, size);

    virtio_flag_completion(node->dev, node->chain, 0, node->start_idx, true);
    free(node);
  } else {
    printf("%d byte packet dropped\n", size);
    rx_dropped++;
    pthread_mutex_unlock(&queue_mutex);
  }
}

struct virtio_device *virtio_net_create(void *ram_image, uint32_t base) {
  if (network_init(&callback)) {
    return NULL;
  }
  return virtio_create(ram_image, &virtio_net_type, base, 0x200, get_next_irq());
}

void virtio_net_teardown() {
  network_teardown();
  printf("  packets | bytes\n");
  printf("tx %6ld | %ld\n", packets_sent, bytes_sent);
  printf("rx %6ld | %ld, dropped %ld packets\n", packets_received, bytes_received, rx_dropped);
}
