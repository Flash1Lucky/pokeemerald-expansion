#ifndef GUARD_CRAFTING_RECIPES_H
#define GUARD_CRAFTING_RECIPES_H

#include "craft_logic.h"

struct CraftRecipe
{
    u16 pattern[CRAFT_ROWS][CRAFT_COLS];
    u16 resultItemId;
    u16 resultQuantity;
};

static const struct CraftRecipe sRecipe_Antidote =
{
    .pattern = {
        { ITEM_POTION },
        { ITEM_PECHA_BERRY },
    },
    .resultItemId = ITEM_ANTIDOTE,
    .resultQuantity = 3,
};

static const struct CraftRecipe gCraftRecipes[] =
{
    sRecipe_Antidote,
};

static const u16 gCraftRecipeCount = ARRAY_COUNT(gCraftRecipes);

#endif // GUARD_CRAFTING_RECIPES_H
