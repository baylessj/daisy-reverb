#pragma once

#include "daisy_petal.h"

namespace audioLib {
namespace valueTables {
const int TableSize = 40001;

// octave response. value float every step (2,3,4,5 or 6 steps)
extern DSY_SDRAM_BSS float Response2Oct[TableSize];
extern DSY_SDRAM_BSS float Response3Oct[TableSize];
extern DSY_SDRAM_BSS float Response4Oct[TableSize];
extern DSY_SDRAM_BSS float Response5Oct[TableSize];
extern DSY_SDRAM_BSS float Response6Oct[TableSize];

// decade response, value multiplies by 10 every step
extern DSY_SDRAM_BSS float Response2Dec[TableSize];
extern DSY_SDRAM_BSS float Response3Dec[TableSize];
extern DSY_SDRAM_BSS float Response4Dec[TableSize];

void Init();
float Get(float index, float* table);
} // namespace valueTables
} // namespace audioLib
