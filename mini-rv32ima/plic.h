#pragma once

#include <stdint.h>

uint32_t plic_load(uint32_t addr);
void plic_store(uint32_t addr, uint32_t val);
void plic_raise_irq(int irq);
void plic_clear_irq(int irq);
void hart_clear_irq(int irq);
void hart_raise_irq(int irq);

extern int next_irq;

inline int get_next_irq(void) {
  return next_irq++;
}
