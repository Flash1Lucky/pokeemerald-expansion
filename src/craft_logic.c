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

static void GetRecipeDimensions(const struct CraftRecipe *recipe, int *rows, int *cols)
{
    int r, c;
    int maxRow = -1;
    int maxCol = -1;

    for (r = 0; r < CRAFT_ROWS; r++)
    {
        for (c = 0; c < CRAFT_COLS; c++)
        {
            if (recipe->pattern[r][c] != ITEM_NONE)
            {
                if (r > maxRow)
                    maxRow = r;
                if (c > maxCol)
                    maxCol = c;
            }
        }
    }

    *rows = maxRow + 1;
    *cols = maxCol + 1;
}

static u16 TryCraftRecipeAt(const struct CraftRecipe *recipe, int baseRow, int baseCol, int patRows, int patCols)
{
    int r, c;
    u16 minQty = 0xFFFF;

    for (r = 0; r < patRows; r++)
    {
        for (c = 0; c < patCols; c++)
        {
            u16 itemId = recipe->pattern[r][c];
            if (itemId != ITEM_NONE)
            {
                struct ItemSlot *slot = &gCraftSlots[baseRow + r][baseCol + c];
                if (slot->itemId != itemId || slot->quantity == 0)
                    return 0;
                if (slot->quantity < minQty)
                    minQty = slot->quantity;
            }
        }
    }

    if (minQty == 0 || minQty == 0xFFFF)
        return 0;

    for (r = 0; r < patRows; r++)
    {
        for (c = 0; c < patCols; c++)
        {
            if (recipe->pattern[r][c] != ITEM_NONE)
            {
                struct ItemSlot *slot = &gCraftSlots[baseRow + r][baseCol + c];
                slot->quantity -= minQty;
                if (slot->quantity == 0)
                    slot->itemId = ITEM_NONE;
            }
        }
    }

    return minQty;
}

static u16 ApplyRecipe(const struct CraftRecipe *recipe)
{
    int patRows, patCols;
    u16 crafted = 0;

    GetRecipeDimensions(recipe, &patRows, &patCols);

    if (patRows == 0 || patCols == 0)
        return 0;

    bool8 progress;
    do
    {
        int baseRow, baseCol;
        progress = FALSE;

        for (baseRow = 0; baseRow <= CRAFT_ROWS - patRows && !progress; baseRow++)
        {
            for (baseCol = 0; baseCol <= CRAFT_COLS - patCols && !progress; baseCol++)
            {
                u16 sets = TryCraftRecipeAt(recipe, baseRow, baseCol, patRows, patCols);
                if (sets)
                {
                    crafted += sets;
                    progress = TRUE;
                }
            }
        }
    } while (progress);

    if (crafted)
        AddBagItem(recipe->resultItemId, crafted * recipe->resultQuantity);

    return crafted * recipe->resultQuantity;
}

u16 CraftLogic_Craft(const struct CraftRecipe *recipes, u16 recipeCount)
{
    u16 i;
    for (i = 0; i < recipeCount; i++)
    {
        u16 crafted = ApplyRecipe(&recipes[i]);
        if (crafted)
            return crafted;
    }

    return 0;
}
