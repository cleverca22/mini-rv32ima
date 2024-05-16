#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

struct mmio_range {
  uint32_t addr;
  uint32_t size;
  uint32_t (*mmio_load)(void*, uint32_t);
  void (*mmio_store)(void*, uint32_t, uint32_t);
  void *state;
  struct mmio_range *next;
};

uint32_t mmio_routed_load(uint32_t addr);
void mmio_routed_store(uint32_t addr, uint32_t val);

extern struct mmio_range *root;

static inline struct mmio_range *mmio_add_handler(uint32_t addr, uint32_t size, uint32_t (*mmio_load)(void*, uint32_t), void (*mmio_store)(void*, uint32_t, uint32_t), void *state, const char *name) {
  struct mmio_range *rng = malloc(sizeof(struct mmio_range));
  rng->addr = addr;
  rng->size = size;
  rng->mmio_load = mmio_load;
  rng->mmio_store = mmio_store;
  rng->state = state;
  rng->next = root;

  root = rng;

  printf("MMIO 0x%08x + 0x%08x == %s\n", addr, size, name);
  return rng;
}

uint32_t get_next_base(uint32_t size);
