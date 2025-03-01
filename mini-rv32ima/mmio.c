#include <assert.h>
#include <stdio.h>
#include "mmio.h"

struct mmio_range *root;
static uint32_t next_base = 0x10000000;
#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))

bool debug_blacklist(uint32_t addr) {
  switch(addr) {
  case 0x10000000: return false;
  case 0x10000001: return false;
  case 0x10000003: return false;
  case 0x10000004: return false;
  case 0x10000005: return false;
  }
  return true;
}

uint32_t mmio_routed_load(uint32_t addr) {
  //if (debug_blacklist(addr)) printf("mmio_routed_load(0x%x)\n", addr);
  for (struct mmio_range *i=root; i; i = i->next) {
    if ((addr >= i->addr) && (addr < (i->addr + i->size))) {
      //printf("0x%x - 0x%x\n", i->addr, (i->addr + i->size)-1);
      //printf("0x%x\n", addr);
      return i->mmio_load(i->state, addr - i->addr);
    }
  }
  return 0;
}

void mmio_routed_store(uint32_t addr, uint32_t val) {
  //if (debug_blacklist(addr)) printf("mmio_routed_store(0x%x, 0x%x)\n", addr, val);
  for (struct mmio_range *i=root; i; i = i->next) {
    if ((addr >= i->addr) && (addr < (i->addr + i->size))) {
      //printf("0x%x - 0x%x\n", i->addr, (i->addr + i->size)-1);
      //printf("0x%x\n", addr);
      i->mmio_store(i->state, addr - i->addr, val);
      break;
    }
  }
}

uint32_t get_next_base(uint32_t size) {
  size = ROUNDUP(size, 0x1000);
  uint32_t ret = next_base;
  next_base += size;
  printf("MMIO: 0x%08x + 0x%08x allocated\n", ret, size);
  return ret;
}
