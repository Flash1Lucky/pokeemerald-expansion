#include "global.h"
#include "constants/items.h"
#include "item.h"
#include "craft_recipes.h"

static const struct ItemSlot sRecipe_AntidoteIngredients[] =
{
    { ITEM_POTION, 1 },
    { ITEM_PECHA_BERRY, 1 },
};

const struct CraftRecipe gCraftRecipes[] =
{
    {
        .ingredients = sRecipe_AntidoteIngredients,
        .ingredientCount = ARRAY_COUNT(sRecipe_AntidoteIngredients),
        .result = ITEM_ANTIDOTE,
        .resultQuantity = 3,
    },
};

const u8 gCraftRecipeCount = ARRAY_COUNT(gCraftRecipes);
