#ifndef GUARD_CRAFTING_RECIPES_H
#define GUARD_CRAFTING_RECIPES_H

#include "craft_logic.h"

#ifndef ITEM_TUMBLESTONE
#define ITEM_TUMBLESTONE ITEM_STARDUST
#endif

#ifndef ITEM_IRON_CHUNK
#define ITEM_IRON_CHUNK ITEM_NUGGET
#endif

static const struct CraftRecipe sPokeballRecipe = {
    .pattern = {
        { ITEM_TUMBLESTONE, ITEM_IRON_CHUNK, ITEM_NONE },
        { ITEM_RED_APRICORN, ITEM_NONE, ITEM_NONE },
        { ITEM_NONE, ITEM_NONE, ITEM_NONE },
    },
    .resultItemId = ITEM_POKE_BALL,
    .resultQuantity = 3,
};

static const struct CraftRecipe gCraftRecipes[] = {
    sPokeballRecipe,
};

static const u16 gCraftRecipeCount = ARRAY_COUNT(gCraftRecipes);

#endif // GUARD_CRAFTING_RECIPES_H
