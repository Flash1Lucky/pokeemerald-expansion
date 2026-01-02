#ifndef GUARD_CRAFTING_RECIPES_H
#define GUARD_CRAFTING_RECIPES_H

#include "craft_logic.h"
#include "constants/flags.h"

struct CraftRecipe
{
    u16 pattern[CRAFT_ROWS][CRAFT_COLS];
    u16 resultQuantity;
    u16 unlockFlag; // FLAG_NONE for always unlocked
};

struct CraftRecipeList
{
    const struct CraftRecipe *recipes;
    u8 count;
};

static const struct CraftRecipeList gCraftRecipes[ITEMS_COUNT] =
{
    [ITEM_ANTIDOTE] =
    {
        .recipes = (const struct CraftRecipe[])
        {
            {
                .pattern =
                {
                    { ITEM_POTION },
                    { ITEM_PECHA_BERRY },
                },
                .resultQuantity = 3,
                .unlockFlag = 0,
            },
        },
        .count = 1,
    },
    [ITEM_POTION] =
    {
        .recipes = (const struct CraftRecipe[])
        {
            {
                .pattern =
                {
                    { ITEM_ORAN_BERRY, },
                    { ITEM_FRESH_WATER, },
                },
                .resultQuantity = 1,
                .unlockFlag = 0,
            },
        },
        .count = 1,
    },
    [ITEM_SUPER_POTION] =
    {
        .recipes = (const struct CraftRecipe[])
        {
            {
                .pattern =
                {
                    { ITEM_POTION, ITEM_POTION, ITEM_POTION },
                },
                .resultQuantity = 2,
                .unlockFlag = FLAG_ITEM_ROUTE_102_POTION,
            },
            {
                .pattern =
                {
                    { ITEM_ORAN_BERRY, ITEM_ORAN_BERRY },
                    { ITEM_FRESH_WATER },
                },
                .resultQuantity = 2,
                .unlockFlag = 0,
            },
        },
        .count = 2,
    },
    [ITEM_RARE_CANDY] =
    {
        .recipes = (const struct CraftRecipe[])
        {
            {
                .pattern =
                {
                    { ITEM_UNREMARKABLE_TEACUP, ITEM_TWICE_SPICED_RADISH, ITEM_FIGHTING_TERA_SHARD, },
                    { ITEM_ELECTRIC_TERA_SHARD, ITEM_STELLAR_TERA_SHARD, ITEM_SCROLL_OF_DARKNESS, },
                    { ITEM_AUSPICIOUS_ARMOR, ITEM_MASTERPIECE_TEACUP, ITEM_TINY_BAMBOO_SHOOT}
                },
                .resultQuantity = 1,
                .unlockFlag = 0,
            },
        },
        .count = 1,
    },
    [ITEM_POKE_BALL] =
    {
        .recipes = (const struct CraftRecipe[])
        {
            {
                .pattern =
                {
                    { ITEM_RED_APRICORN, ITEM_RED_APRICORN},
                },
                .resultQuantity = 1,
                .unlockFlag = 0,
            },
        },
        .count = 1,
    },
    [ITEM_GREAT_BALL] =
    {
        .recipes = (const struct CraftRecipe[])
        {
            {
                .pattern =
                {
                    { ITEM_BLUE_APRICORN, },
                    { ITEM_POKE_BALL, },
                },
                .resultQuantity = 1,
                .unlockFlag = 0,
            },
        },
        .count = 1,
    },
    /*[ITEM_ACRO_BIKE] =
    {
        .recipes = (const struct CraftRecipe[])
        {
            {
                .pattern =
                {
                    { ITEM_ORAN_BERRY },
                    { ITEM_PECHA_BERRY, },
                },
                .resultQuantity = 1,
                .unlockFlag = 0,
            },
        },
        .count = 1,
    },*/
    [ITEM_MASTERPIECE_TEACUP] =
    {
        .recipes = (const struct CraftRecipe[])
        {
            {
                .pattern =
                {
                    { ITEM_CHIPPED_POT, },
                    { ITEM_FRESH_WATER, },
                },
                .resultQuantity = 1,
                .unlockFlag = 0,
            },
            {
                .pattern =
                {
                    { ITEM_GREEN_APRICORN, ITEM_YELLOW_APRICORN, ITEM_BLACK_APRICORN },
                },
                .resultQuantity = 2,
                .unlockFlag = 0,
            },
        },
        .count = 2,
    },
    [ITEM_TWICE_SPICED_RADISH] =
    {
        .recipes = (const struct CraftRecipe[])
        {
            {
                .pattern =
                {
                    { ITEM_BLUE_APRICORN, },
                    { ITEM_RED_APRICORN, },
                },
                .resultQuantity = 1,
                .unlockFlag = 0,
            },
            {
                .pattern =
                {
                    { ITEM_ORAN_BERRY, },
                    { ITEM_PECHA_BERRY, },
                    { ITEM_SITRUS_BERRY, },
                },
                .resultQuantity = 1,
                .unlockFlag = 0,
            },
        },
        .count = 2,
    },
};

static const u16 gCraftRecipeCount = ARRAY_COUNT(gCraftRecipes);

#endif // GUARD_CRAFTING_RECIPES_H
