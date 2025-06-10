#ifndef GUARD_CRAFT_LOGIC_H
#define GUARD_CRAFT_LOGIC_H

#include "item.h"

#define CRAFT_SLOT_COUNT 9

extern u16 gCraftSlots[CRAFT_SLOT_COUNT];

void CraftLogic_InitSlots(void);

#endif // GUARD_CRAFT_LOGIC_H
