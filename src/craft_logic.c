#include "global.h"
#include "constants/items.h"
#include "craft_logic.h"

EWRAM_DATA struct ItemSlot gCraftSlots[CRAFT_SLOT_COUNT];
EWRAM_DATA u8 gCraftActiveSlot = 0;

void CraftLogic_InitSlots(void)
{
    int i;
    for (i = 0; i < CRAFT_SLOT_COUNT; i++)
    {
        gCraftSlots[i].itemId = ITEM_NONE;
        gCraftSlots[i].quantity = 0;
    }
}

void CraftLogic_SetSlot(u8 slot, u16 itemId, u16 quantity)
{
    if (slot >= CRAFT_SLOT_COUNT)
        return;

    gCraftSlots[slot].itemId = itemId;
    gCraftSlots[slot].quantity = quantity;
}
