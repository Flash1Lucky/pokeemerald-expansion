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

    gCraftSlots[slot / CRAFT_COLS][slot % CRAFT_COLS].itemId = itemId;
    gCraftSlots[slot / CRAFT_COLS][slot % CRAFT_COLS].quantity = quantity;
}

void CraftLogic_SwapSlots(u8 slotA, u8 slotB)
{
    if (slotA >= CRAFT_SLOT_COUNT || slotB >= CRAFT_SLOT_COUNT || slotA == slotB)
        return;

    struct ItemSlot temp = gCraftSlots[slotA / CRAFT_COLS][slotA % CRAFT_COLS];
    gCraftSlots[slotA / CRAFT_COLS][slotA % CRAFT_COLS] = gCraftSlots[slotB / CRAFT_COLS][slotB % CRAFT_COLS];
    gCraftSlots[slotB / CRAFT_COLS][slotB % CRAFT_COLS] = temp;
}
