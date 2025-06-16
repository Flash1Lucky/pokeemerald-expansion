#ifndef GUARD_CRAFT_LOGIC_H
#define GUARD_CRAFT_LOGIC_H

#include "item.h"
#include "craft_recipes.h"

#define CRAFT_GRID_SIZE 3
#define CRAFT_SLOT_COUNT (CRAFT_GRID_SIZE * CRAFT_GRID_SIZE)

extern struct ItemSlot gCraftSlots[CRAFT_SLOT_COUNT];
extern u8 gCraftActiveSlot;

void CraftLogic_InitSlots(void);
void CraftLogic_SetSlot(u8 slot, u16 itemId, u16 quantity);
void CraftLogic_SwapSlots(u8 slotA, u8 slotB);


struct CraftMatch
{
    const struct CraftRecipe *recipe;
    u8 x; // offset in grid (0-2)
    u8 y; // offset in grid (0-2)
};

bool8 CraftLogic_FindMatchingRecipe(struct CraftMatch *match);
void CraftLogic_ConsumeRecipe(const struct CraftMatch *match);

#endif // GUARD_CRAFT_LOGIC_H
