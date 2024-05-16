#include <libfdt.h>
#include <stdio.h>
#include <unistd.h>

#include "pl011.h"
#include "plic.h"
#include "mmio.h"
#include "stdio.h"

static int uart_irq;
uint32_t irq_status = 0;
uint32_t irq_mask = 0;

static void pl011_maybe_irq(void) {
  if (irq_status & irq_mask) {
    plic_raise_irq(uart_irq);
  } else {
    plic_clear_irq(uart_irq);
  }
}

void pl011_poll(void) {
  if (IsKBHit() == true) {
    irq_status |= 1<<4;
    pl011_maybe_irq();
  } else {
    irq_status &= ~(1<<4);
    pl011_maybe_irq();
  }
}

static uint32_t pl011_load(void *state, uint32_t addr) {
  uint32_t ret = 0;
  switch (addr) {
  case 0x0: // data register
    if (IsKBHit()) ret = ReadKBByte();
    pl011_poll();
    break;
  case 0x18: // flag register
    if (!IsKBHit()) ret |= 1<<4; // rx fifo empty
    ret |= 1<<7; // tx fifo empty
    break;
  case 0x3c: // irq status
    ret = irq_status;
    break;
  case 0x40: // masked irq
    ret = irq_status & irq_mask;
    break;
  }
  //printf("pl011_load(0x%x) == 0x%x\n", addr, ret);
  return ret;
}

static void pl011_store(void *state, uint32_t addr, uint32_t val) {
  //printf("pl011_store(0x%x, 0x%x)\n", addr, val);
  switch (addr) {
  case 0: // data reg
    printf("%c", val);
    fflush( stdout );
    break;
  case 0x38: // uart interrupt mask
    irq_mask = val;
    pl011_maybe_irq();
    break;
  case 0x44: // irq clear
    irq_status &= ~val;
    break;
  }
}

void pl011_create(void *v_fdt, uint32_t reg_base) {
  int uart = fdt_node_offset_by_compatible(v_fdt, 0, "arm,pl011");
  int parent = fdt_parent_offset(v_fdt, uart);

  uart_irq = get_next_irq();

  fdt_setprop_u32(v_fdt, uart, "interrupts", uart_irq);
  fdt_appendprop_addrrange(v_fdt, parent, uart, "reg", reg_base, 0x100);

  mmio_add_handler(reg_base, 0x100, pl011_load, pl011_store, NULL, "PL011");
}
