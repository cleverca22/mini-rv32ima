#pragma once

#include <stdint.h>

void pl011_create(void *v_fdt, uint32_t reg_base);
void pl011_poll(void);
