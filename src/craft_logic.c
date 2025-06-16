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

static bool8 TableMatchesRecipe(const struct CraftRecipe *recipe)
{
    int i, j;

    // Check ingredient quantities
    for (i = 0; i < recipe->ingredientCount; i++)
    {
        u16 itemId = recipe->ingredients[i].itemId;
        u16 need = recipe->ingredients[i].quantity;
        u16 have = 0;

        for (j = 0; j < CRAFT_SLOT_COUNT; j++)
        {
            if (gCraftSlots[j].itemId == itemId)
                have += gCraftSlots[j].quantity;
        }

        if (have != need)
            return FALSE;
    }

    // Ensure no extra items present
    for (j = 0; j < CRAFT_SLOT_COUNT; j++)
    {
        if (gCraftSlots[j].itemId == ITEM_NONE)
            continue;

        bool8 found = FALSE;
        for (i = 0; i < recipe->ingredientCount; i++)
        {
            if (gCraftSlots[j].itemId == recipe->ingredients[i].itemId)
            {
                found = TRUE;
                break;
            }
        }

        if (!found)
            return FALSE;
    }

    return TRUE;
}

const struct CraftRecipe *CraftLogic_FindMatchingRecipe(void)
{
    int i;

    for (i = 0; i < gCraftRecipeCount; i++)
    {
        if (TableMatchesRecipe(&gCraftRecipes[i]))
            return &gCraftRecipes[i];
    }

    return NULL;
}

void CraftLogic_ConsumeRecipe(const struct CraftRecipe *recipe)
{
    int i, j;

    for (i = 0; i < recipe->ingredientCount; i++)
    {
        u16 itemId = recipe->ingredients[i].itemId;
        u16 remaining = recipe->ingredients[i].quantity;

        for (j = 0; j < CRAFT_SLOT_COUNT && remaining > 0; j++)
        {
            struct ItemSlot *slot = &gCraftSlots[j];

            if (slot->itemId != itemId)
                continue;

            if (slot->quantity > remaining)
            {
                slot->quantity -= remaining;
                remaining = 0;
            }
            else
            {
                remaining -= slot->quantity;
                slot->itemId = ITEM_NONE;
                slot->quantity = 0;
            }
        }
    }
}
