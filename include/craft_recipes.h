#ifndef GUARD_CRAFT_RECIPES_H
#define GUARD_CRAFT_RECIPES_H

#include "item.h"
#include "craft_logic.h"

struct CraftRecipe
{
    const struct ItemSlot *ingredients; // array of required ingredients
    u8 ingredientCount;
    u16 result;
    u16 resultQuantity;
};

extern const struct CraftRecipe gCraftRecipes[];
extern const u8 gCraftRecipeCount;

#endif // GUARD_CRAFT_RECIPES_H
