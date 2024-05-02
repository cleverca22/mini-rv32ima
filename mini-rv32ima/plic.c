#include <stdio.h>
#include "plic.h"

#define CONTEXT_BASE                    0x200000
#define     CONTEXT_SIZE                0x1000
#define     CONTEXT_THRESHOLD           0x00
#define     CONTEXT_CLAIM               0x04

static const uint32_t irq_mask = 0xffffffff;
static uint32_t irq_pending = 0;
static uint32_t irq_claimed = 0;

int next_irq = 1;

uint32_t plic_load(void* state, uint32_t addr) {
  uint32_t ret = 0;
  if ((addr >= CONTEXT_BASE) && (addr < (CONTEXT_BASE + CONTEXT_SIZE))) { // context 0
    uint32_t context_offset = addr - CONTEXT_BASE;
    switch (context_offset) {
    case 4: // claim register
      uint32_t unmasked_pending_irq = (irq_mask & irq_pending) & ~irq_claimed;
      for (int i=0; i<32; i++) {
        if (unmasked_pending_irq & (1<<i)) {
          ret = i;
          irq_claimed |= 1<<i;
          break;
        }
      }
      break;
    }
  }
  //printf("plic_load(0x%x) == 0x%x\n", addr, ret);
  return ret;
}

void plic_store(void *state, uint32_t addr, uint32_t val) {
  //printf("plic_store(0x%x, %d)\n", addr, val);
  if ((addr >= CONTEXT_BASE) && (addr < (CONTEXT_BASE + CONTEXT_SIZE))) { // context 0
    uint32_t context_offset = addr - CONTEXT_BASE;
    switch (context_offset) {
    case 4: // claim register
      irq_claimed &= ~(1<<val);
      break;
    }
  }
}

void plic_raise_irq(int irq) {
  irq_pending |= 1<<irq;
  uint32_t unmasked_pending_irq = (irq_mask & irq_pending) & ~irq_claimed;
  if (unmasked_pending_irq) hart_raise_irq(11);
}

void plic_clear_irq(int irq) {
  irq_pending &= ~(1<<irq);
  uint32_t unmasked_pending_irq = (irq_mask & irq_pending) & ~irq_claimed;
  if (unmasked_pending_irq == 0) hart_clear_irq(11);
}
