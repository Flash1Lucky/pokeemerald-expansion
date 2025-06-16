#ifndef GUARD_CRAFT_LOGIC_H
#define GUARD_CRAFT_LOGIC_H

#include "item.h"
#include "craft_recipes.h"

#define CRAFT_SLOT_COUNT 9

extern struct ItemSlot gCraftSlots[CRAFT_SLOT_COUNT];
extern u8 gCraftActiveSlot;

void CraftLogic_InitSlots(void);
void CraftLogic_SetSlot(u8 slot, u16 itemId, u16 quantity);
void CraftLogic_SwapSlots(u8 slotA, u8 slotB);

const struct CraftRecipe *CraftLogic_FindMatchingRecipe(void);
void CraftLogic_ConsumeRecipe(const struct CraftRecipe *recipe);

#endif // GUARD_CRAFT_LOGIC_H
