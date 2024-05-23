#pragma once

#include <stdint.h>
#include <stdbool.h>

uint32_t plic_load(void *, uint32_t addr);
void plic_store(void *, uint32_t addr, uint32_t val);
void plic_raise_irq(int irq, bool need_lock);
void plic_clear_irq(int irq, bool need_lock);
void hart_clear_irq(int irq, bool need_lock);
void hart_raise_irq(int irq, bool need_lock);

extern int next_irq;

inline int get_next_irq(void) {
  return next_irq++;
}
