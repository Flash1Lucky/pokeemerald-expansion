#include "global.h"
#include "constants/items.h"
#include "craft_logic.h"

EWRAM_DATA u16 gCraftSlots[CRAFT_SLOT_COUNT];

void CraftLogic_InitSlots(void)
{
    int i;
    for (i = 0; i < CRAFT_SLOT_COUNT; i++)
        gCraftSlots[i] = ITEM_NONE;
}
