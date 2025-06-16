#include "global.h"
#include "constants/items.h"
#include "craft_logic.h"

EWRAM_DATA struct ItemSlot gCraftSlots[CRAFT_ROWS][CRAFT_COLS];
EWRAM_DATA u8 gCraftActiveSlot = 0;

void CraftLogic_InitSlots(void)
{
    int row, col;
    for (row = 0; row < CRAFT_ROWS; row++)
    {
        for (col = 0; col < CRAFT_COLS; col++)
        {
            gCraftSlots[row][col].itemId = ITEM_NONE;
            gCraftSlots[row][col].quantity = 0;
        }
    }
}

void CraftLogic_SetSlot(u8 slot, u16 itemId, u16 quantity)
{
    if (slot >= CRAFT_SLOT_COUNT)
        return;

    gCraftSlots[CRAFT_SLOT_ROW(slot)][CRAFT_SLOT_COL(slot)].itemId = itemId;
    gCraftSlots[CRAFT_SLOT_ROW(slot)][CRAFT_SLOT_COL(slot)].quantity = quantity;
}

void CraftLogic_SwapSlots(u8 slotA, u8 slotB)
{
    if (slotA >= CRAFT_SLOT_COUNT || slotB >= CRAFT_SLOT_COUNT || slotA == slotB)
        return;

    struct ItemSlot temp = gCraftSlots[CRAFT_SLOT_ROW(slotA)][CRAFT_SLOT_COL(slotA)];
    gCraftSlots[CRAFT_SLOT_ROW(slotA)][CRAFT_SLOT_COL(slotA)] = gCraftSlots[CRAFT_SLOT_ROW(slotB)][CRAFT_SLOT_COL(slotB)];
    gCraftSlots[CRAFT_SLOT_ROW(slotB)][CRAFT_SLOT_COL(slotB)] = temp;
}
