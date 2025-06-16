#include "global.h"
#include "constants/items.h"
#include "item.h"
#include "craft_recipes.h"

// Example shaped recipe: placing a POTION above a PECHA BERRY yields
// three ANTIDOTEs. The pattern may be placed in any column.
static const struct ItemSlot sRecipe_AntidotePattern[] =
{
    { ITEM_POTION, 1 },      // row 0, column 0
    { ITEM_PECHA_BERRY, 1 }, // row 1, column 0
};

const struct CraftRecipe gCraftRecipes[] =
{
    {
        .pattern = sRecipe_AntidotePattern,
        .width = 1,
        .height = 2,
        .result = ITEM_ANTIDOTE,
        .resultQuantity = 3,
    },
};

const u8 gCraftRecipeCount = ARRAY_COUNT(gCraftRecipes);
