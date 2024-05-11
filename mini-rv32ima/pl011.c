#include <libfdt.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "pl011.h"
#include "plic.h"
#include "mmio.h"

static int uart_irq;
static int is_eofd;
uint32_t irq_status = 0;
uint32_t irq_mask = 0;

static void pl011_maybe_irq(void) {
  if (irq_status & irq_mask) {
    plic_raise_irq(uart_irq);
  } else {
    plic_clear_irq(uart_irq);
  }
}

static int ReadKBByte(void)
{
	if( is_eofd ) return 0xffffffff;
	char rxchar = 0;
	int rread = read(fileno(stdin), (char*)&rxchar, 1);

	if( rread > 0 ) // Tricky: getchar can't be used with arrow keys.
		return rxchar;
	else
		return -1;
}

static int IsKBHit(void)
{
	if( is_eofd ) return -1;
	int byteswaiting;
	ioctl(0, FIONREAD, &byteswaiting);
	if( !byteswaiting && write( fileno(stdin), 0, 0 ) != 0 ) { is_eofd = 1; return -1; } // Is end-of-file for 
	return !!byteswaiting;
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
  int soc = fdt_path_offset(v_fdt, "/soc");
  char buffer[32];
  snprintf(buffer, 32, "pl011@%x", reg_base);
  int uart = fdt_add_subnode(v_fdt, soc, buffer);
  fdt_appendprop_addrrange(v_fdt, soc, uart, "reg", reg_base, 0x100);
  uart_irq = get_next_irq();
  fdt_setprop_u32(v_fdt, uart, "interrupts", uart_irq);
  fdt_setprop_u32(v_fdt, uart, "reg-io-width", 4);
  fdt_setprop_u32(v_fdt, uart, "current-speed", 115200);
  fdt_setprop_string(v_fdt, uart, "compatible", "arm,pl011");
  fdt_appendprop_string(v_fdt, uart, "compatible", "arm,sbsa-uart");
  mmio_add_handler(reg_base, 0x100, pl011_load, pl011_store, NULL);
}
