#ifndef GUARD_CRAFTING_RECIPES_H
#define GUARD_CRAFTING_RECIPES_H

#include "craft_logic.h"

struct CraftRecipe
{
    u16 pattern[CRAFT_ROWS][CRAFT_COLS];
    u16 resultQuantity;
};

struct CraftRecipeList
{
    const struct CraftRecipe *recipes;
    u8 count;
};

static const struct CraftRecipe sAntidoteRecipes[] =
{
    {
        .pattern = {
            { ITEM_POTION },
            { ITEM_PECHA_BERRY },
        },
        .resultQuantity = 3,
    },
};

static const struct CraftRecipe sSuperPotionRecipes[] =
{
    {
        .pattern = {
            { ITEM_POTION, ITEM_POTION, ITEM_POTION },
        },
        .resultQuantity = 2,
    },
    {
        .pattern = {
            { ITEM_ORAN_BERRY, ITEM_ORAN_BERRY },
            { ITEM_FRESH_WATER },
        },
        .resultQuantity = 2,
    },
};

static const struct CraftRecipeList gCraftRecipes[ITEMS_COUNT] =
{
    [ITEM_ANTIDOTE] = { sAntidoteRecipes, ARRAY_COUNT(sAntidoteRecipes) },
    [ITEM_SUPER_POTION] = { sSuperPotionRecipes, ARRAY_COUNT(sSuperPotionRecipes) },
};

static const u16 gCraftRecipeCount = ARRAY_COUNT(gCraftRecipes);

#endif // GUARD_CRAFTING_RECIPES_H
