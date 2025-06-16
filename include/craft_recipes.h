#ifndef GUARD_CRAFT_RECIPES_H
#define GUARD_CRAFT_RECIPES_H

#include "item.h"
#include "craft_logic.h"

struct CraftRecipe
{
    // Pattern of items laid out in a grid. Only the rectangle
    // defined by `width` and `height` is used.
    const struct ItemSlot *pattern; // array of ItemSlot of size width * height
    u8 width;   // recipe width  (1-3)
    u8 height;  // recipe height (1-3)
    u16 result;
    u16 resultQuantity;
};

extern const struct CraftRecipe gCraftRecipes[];
extern const u8 gCraftRecipeCount;

#endif // GUARD_CRAFT_RECIPES_H
