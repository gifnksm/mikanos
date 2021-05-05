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
uint64_t GetCr0();
void SetCr0(uint64_t value);
uint64_t GetCr2();
void SetCr3(uint64_t value);
uint64_t GetCr3();
void SwitchContext(void *next_ctx, void *current_ctx);
void RestoreContext(void *task_context);
int CallApp(int argc, char **argv, uint16_t ss, uint64_t rip, uint64_t rsp, uint64_t *os_stack_ptr);
void IntHandlerLapicTimer();
void LoadTr(uint16_t sel);
void WriteMsr(uint32_t msr, uint64_t value);
void SyscallEntry(void);
void ExitApp(uint64_t rsp, int32_t ret_val);
void InvalidateTlb(uint64_t addr);
}
