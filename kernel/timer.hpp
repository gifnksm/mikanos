#pragma once

#include <cstdint>

void InitializeLapicTimer();
void StartLapicTimer();
uint32_t LapicTimerElapsed();
void StopLapicTimer();
