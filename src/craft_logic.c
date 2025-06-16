#include "global.h"
#include "constants/items.h"
#include "craft_logic.h"
#include "craft_recipes.h"
#include "item.h"

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

void CraftLogic_SwapSlots(u8 slotA, u8 slotB)
{
    if (slotA >= CRAFT_SLOT_COUNT || slotB >= CRAFT_SLOT_COUNT || slotA == slotB)
        return;

    struct ItemSlot temp = gCraftSlots[slotA];
    gCraftSlots[slotA] = gCraftSlots[slotB];
    gCraftSlots[slotB] = temp;
}

static bool8 CheckRecipeAtOffset(const struct CraftRecipe *recipe, u8 offX, u8 offY)
{
    u8 x, y;

    // Verify pattern region
    for (y = 0; y < recipe->height; y++)
    {
        for (x = 0; x < recipe->width; x++)
        {
            const struct ItemSlot *expect = &recipe->pattern[y * recipe->width + x];
            const struct ItemSlot *slot = &gCraftSlots[(offY + y) * CRAFT_GRID_SIZE + (offX + x)];

            if (expect->itemId == ITEM_NONE)
            {
                if (slot->itemId != ITEM_NONE)
                    return FALSE;
            }
            else
            {
                if (slot->itemId != expect->itemId || slot->quantity != expect->quantity)
                    return FALSE;
            }
        }
    }

    // Ensure no items outside pattern area
    for (y = 0; y < CRAFT_GRID_SIZE; y++)
    {
        for (x = 0; x < CRAFT_GRID_SIZE; x++)
        {
            if (x >= offX && x < offX + recipe->width && y >= offY && y < offY + recipe->height)
                continue;

            if (gCraftSlots[y * CRAFT_GRID_SIZE + x].itemId != ITEM_NONE)
                return FALSE;
        }
    }

    return TRUE;
}

static bool8 TableMatchesRecipe(const struct CraftRecipe *recipe, struct CraftMatch *match)
{
    u8 offX, offY;

    for (offY = 0; offY <= CRAFT_GRID_SIZE - recipe->height; offY++)
    {
        for (offX = 0; offX <= CRAFT_GRID_SIZE - recipe->width; offX++)
        {
            if (CheckRecipeAtOffset(recipe, offX, offY))
            {
                match->recipe = recipe;
                match->x = offX;
                match->y = offY;
                return TRUE;
            }
        }
    }

    return FALSE;
}

bool8 CraftLogic_FindMatchingRecipe(struct CraftMatch *match)
{
    int i;

    for (i = 0; i < gCraftRecipeCount; i++)
    {
        if (TableMatchesRecipe(&gCraftRecipes[i], match))
            return TRUE;
    }

    return FALSE;
}

void CraftLogic_ConsumeRecipe(const struct CraftMatch *match)
{
    u8 x, y;
    const struct CraftRecipe *recipe = match->recipe;

    for (y = 0; y < recipe->height; y++)
    {
        for (x = 0; x < recipe->width; x++)
        {
            const struct ItemSlot *expect = &recipe->pattern[y * recipe->width + x];
            struct ItemSlot *slot = &gCraftSlots[(match->y + y) * CRAFT_GRID_SIZE + (match->x + x)];

            if (expect->itemId != ITEM_NONE)
            {
                slot->itemId = ITEM_NONE;
                slot->quantity = 0;
            }
        }
    }
}
