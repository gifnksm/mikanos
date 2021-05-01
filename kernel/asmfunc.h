#pragma once

#include <stdint.h>

extern "C" {
void IoOut32(uint16_t addr, uint32_t data);
uint32_t IoIn32(uint16_t addr);
uint16_t GetCs(void);
void LoadIdt(uint16_t limit, uint64_t offset);
void LoadGdt(uint16_t limit, uint64_t offset);
void SetCsSs(uint16_t cs, uint16_t ss);
void SetDsAll(uint16_t value);
void SetCr3(uint64_t value);
}
