#ifndef GUARD_CRAFT_LOGIC_H
#define GUARD_CRAFT_LOGIC_H

#include "item.h"

#define CRAFT_ROWS 3
#define CRAFT_COLS 3
#define CRAFT_SLOT_COUNT (CRAFT_ROWS * CRAFT_COLS)

extern struct ItemSlot gCraftSlots[CRAFT_ROWS][CRAFT_COLS];
extern u8 gCraftActiveSlot;

void CraftLogic_InitSlots(void);
void CraftLogic_SetSlot(u8 slot, u16 itemId, u16 quantity);
void CraftLogic_SwapSlots(u8 slotA, u8 slotB);

#endif // GUARD_CRAFT_LOGIC_H
