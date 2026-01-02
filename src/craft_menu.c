#include "global.h"
#include "task.h"
#include "script.h"
#include "sound.h"
#include "main.h"
#include "bg.h"
#include "gpu_regs.h"
#include "window.h"
#include "sprite.h"
#include "list_menu.h"
#include "event_data.h"
#include "graphics.h"
#include "item_icon.h"
#include "constants/songs.h"
#include "constants/rgb.h"
#include "start_menu.h"
#include "menu.h"
#include "menu_helpers.h"
#include "text.h"
#include "international_string_util.h"
#include "field_player_avatar.h"
#include "event_object_lock.h"
#include "craft_menu_ui.h"
#include "craft_logic.h"
#include "data/crafting_recipes.h"
#include "item.h"
#include "item_menu.h"
#include "strings.h"
#include "craft_menu.h"
#include "craft_debug.h"
#include "config/crafting.h"
#include "constants/rtc.h"
#include "field_screen_effect.h"
#include "string_util.h"
#include "field_weather.h"
#include "palette.h"
#include "overworld.h"

static void Task_EnterCraftMenu(u8 taskId);
static void Task_RunCraftMenu(u8 taskId);
static void InitCraftMenu(void);
static bool8 HandleCraftMenuInput(void);
static bool8 HandleSlotActionInput(void);
static bool8 HandleSwapSlotInput(void);
static void CloseCraftMenu(void);
static void Task_PackUpAsk(u8 taskId);
static void Task_ShowPackUpYesNo(u8 taskId);
static void PackUpYes(u8 taskId);
static void PackUpNo(u8 taskId);
static bool8 CraftMenu_HasItemsOnTable(void);
static void Task_AdjustQuantity_Start(u8 taskId);
static void Task_AdjustQuantity_Init(u8 taskId);
static void Task_AdjustQuantity_HandleInput(u8 taskId);
static void CraftMenu_ReshowAfterBagMenu(void);
static void Task_WaitFadeAndOpenRecipeBook(u8 taskId);
static void Task_WaitForCraftMessageAck(u8 taskId);
static void Task_WaitForCraftResultAck(u8 taskId);
static void Task_NoCraftItems(u8 taskId);
static void Task_InvalidCraft(u8 taskId);
static void Task_CraftAsk(u8 taskId);
static void Task_ShowCraftYesNo(u8 taskId);
static void CraftYes(u8 taskId);
static void CraftNo(u8 taskId);
void CB2_ReturnToCraftMenu(void);
static void CB2_CraftRecipeBookMenu(void);
static void Task_RecipeBookMenu(u8 taskId);
static void RecipeBookMainCB2(void);
static void RecipeBookVBlankCB(void);
static void RecipeBook_Init(void);
static void RecipeBook_Cleanup(void);
static void RecipeBook_BuildPocketList(void);
static void RecipeBook_BuildItemList(enum Pocket pocketId);
static void RecipeBook_InitWindows(void);
static void RecipeBook_DestroyWindows(void);
static void RecipeBook_RefreshListMenu(void);
static void RecipeBook_Print(u8 windowId, u8 fontId, const u8 *str, u8 left, u8 top, u8 colorIndex);
static void RecipeBook_PrintPocketNames(const u8 *pocketName1, const u8 *pocketName2);
static void RecipeBook_CopyPocketNameToWindow(u32 offset);
static void RecipeBook_DrawPocketIndicatorSquares(void);
static void RecipeBook_CreatePocketSwitchArrows(void);
static void RecipeBook_DestroyPocketSwitchArrows(void);
static void RecipeBook_DestroyPatternIcons(void);
static void RecipeBook_UpdatePatternPreview(void);
static void RecipeBook_SchedulePatternPreview(void);
static void RecipeBook_TickPatternPreview(void);
static void RecipeBook_UpdateIngredientsWindow(void);
static u8 RecipeBook_GetUniqueIngredientCount(const struct CraftRecipe *recipe);
static u8 RecipeBook_GetUnlockedVariantCount(u16 itemId);
static u8 RecipeBook_GetVariantIndex(u16 itemId);
static const struct CraftRecipe *RecipeBook_GetRecipeVariant(u16 itemId, u8 variantIndex);
static void RecipeBook_PrintListItem(u8 windowId, u32 itemId, u8 y);
static void RecipeBook_RedrawListRow(u16 itemIndex, u8 row);
static void RecipeBook_RedrawVisibleListRows(void);
static void RecipeBook_UpdateListScrollArrows(void);
static void RecipeBook_DestroyListScrollArrows(void);
static void RecipeBook_DisableTimeOfDayBlend(void);
static void RecipeBook_RestoreTimeOfDayBlend(void);

//---------------------------------------------------------------------------
// Entry points
//---------------------------------------------------------------------------

void StartCraftMenu(void)
{
    PlayerFreeze();
    StopPlayerAvatar();
    ScriptContext_Stop();
    LockPlayerFieldControls();
    CreateTask(Task_EnterCraftMenu, 0);
}

static void Task_EnterCraftMenu(u8 taskId)
{
    InitCraftMenu();
    gMenuCallback = HandleCraftMenuInput;
    gTasks[taskId].func = Task_RunCraftMenu;
}

static void Task_RunCraftMenu(u8 taskId)
{
    if (gMenuCallback && gMenuCallback() == TRUE)
        DestroyTask(taskId);
}

//---------------------------------------------------------------------------
// Menu workflow and state
//---------------------------------------------------------------------------

static bool8 sKeepSlots = FALSE;

enum {
    SLOT_ACTION_SWAP_ITEM,
    SLOT_ACTION_ADJUST_QTY,
    SLOT_ACTION_SWAP_SLOT,
    SLOT_ACTION_REMOVE_ITEM,
    SLOT_ACTION_CANCEL,
};

static u8 sSwapFromSlot = 0;

static const u8 sText_CraftSuccess[] = _("{STR_VAR_1} {STR_VAR_2} were crafted and\nput away in the {STR_VAR_3} POCKET.");
static const u8 sText_CraftExcess[] = _("Used {STR_VAR_1} of each ingredient. Excess\ningredients were left on the table.");

static u16 GetUsedPerIngredient(const struct ItemSlot slotsBefore[CRAFT_ROWS][CRAFT_COLS], bool8 *hasRemaining)
{
    u16 usedPerIngredient = 0;
    int row;
    int col;

    *hasRemaining = FALSE;
    for (row = 0; row < CRAFT_ROWS; row++)
    {
        for (col = 0; col < CRAFT_COLS; col++)
        {
            const struct ItemSlot *before = &slotsBefore[row][col];
            const struct ItemSlot *after = &gCraftSlots[row][col];
            u16 used = 0;

            if (after->itemId != ITEM_NONE)
                *hasRemaining = TRUE;

            if (before->itemId != ITEM_NONE)
            {
                if (after->itemId == before->itemId && after->quantity <= before->quantity)
                    used = before->quantity - after->quantity;
                else
                    used = before->quantity;

                if (used != 0 && (usedPerIngredient == 0 || used < usedPerIngredient))
                    usedPerIngredient = used;
            }
        }
    }

    return usedPerIngredient;
}

static void SetCraftSuccessVars(u16 resultItemId, u16 resultQty)
{
    ConvertIntToDecimalStringN(gStringVar1, resultQty, STR_CONV_MODE_LEFT_ALIGN, 3);
    CopyItemNameHandlePlural(resultItemId, gStringVar2, resultQty);
    StringCopy(gStringVar3, gPocketNamesStringsTable[GetItemPocket(resultItemId)]);
}

static void SetCraftExcessVars(u16 usedPerIngredient)
{
    ConvertIntToDecimalStringN(gStringVar1, usedPerIngredient, STR_CONV_MODE_LEFT_ALIGN, 3);
}

static void InitCraftMenu(void)
{
    if (!sKeepSlots)
        CraftLogic_InitSlots();
    CraftMenuUI_Init();
    if (sKeepSlots)
        CraftMenuUI_SetCursorPos(gCraftActiveSlot);
    sKeepSlots = FALSE;
}

void CB2_ReturnToCraftMenu(void)
{
    gFieldCallback = CraftMenu_ReshowAfterBagMenu;
    SetMainCallback2(CB2_ReturnToField);
}
void CB2_OpenCraftMenu(void)
{
    StartCraftMenu();
    SetMainCallback2(CB2_Overworld);
}

enum
{
    RECIPEBOOK_WIN_POCKET_NAME,
    RECIPEBOOK_WIN_LIST,
    RECIPEBOOK_WIN_INGREDIENTS,
    RECIPEBOOK_WIN_COUNT
};

static u8 sRecipeBookWindowIds[RECIPEBOOK_WIN_COUNT];

static const struct WindowTemplate sRecipeBookWindowTemplates[] =
{
    [RECIPEBOOK_WIN_POCKET_NAME] = {
        .bg = 0,
        .tilemapLeft = 4,
        .tilemapTop = 1,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 1
    },
    [RECIPEBOOK_WIN_LIST] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 6,
        .width = 15,
        .height = 12,
        .paletteNum = 15,
        .baseBlock = 17
    },
    [RECIPEBOOK_WIN_INGREDIENTS] = {
        .bg = 0,
        .tilemapLeft = 16,
        .tilemapTop = 0,
        .width = 14,
        .height = 8,
        .paletteNum = 15,
        .baseBlock = 197
    },
    DUMMY_WIN_TEMPLATE,
};

static const u8 sText_RecipeBookNoRecipes[] = _("No recipes.");
static const u8 sText_RecipeBookClose[] = _("CLOSE RECIPE BOOK");
static const u8 sText_RecipeBookIngredients[] = _("Required Ingredients");
static const u8 sText_RecipeBookEllipsis[] = _("...");
static const u8 sText_RecipeBookLButton[] = _("{L_BUTTON}");
static const u8 sText_RecipeBookRButton[] = _("{R_BUTTON}");

#define MAX_RECIPEBOOK_ITEMS  ((max(BAG_TMHM_COUNT,              \
                                max(BAG_BERRIES_COUNT,           \
                                max(BAG_ITEMS_COUNT,             \
                                max(BAG_KEYITEMS_COUNT,          \
                                    BAG_POKEBALLS_COUNT))))) + 1)

#define TAG_RECIPEBOOK_PATTERN_ICON_BASE 0x5400
#define RECIPEBOOK_POCKET_INDICATOR_CENTER_X 8
#define RECIPEBOOK_POCKET_INDICATOR_Y 3
#define RECIPEBOOK_BG_TILE_COUNT 48
#define RECIPEBOOK_BG_TILE_BYTES (RECIPEBOOK_BG_TILE_COUNT * 32)
#define RECIPEBOOK_BG_TILEMAP_BYTES (32 * 32 * 2)
#define RECIPEBOOK_PATTERN_BASE_X 148
#define RECIPEBOOK_PATTERN_BASE_Y 78

#define RECIPEBOOK_POCKET_SCROLL_ARROW_TAG 112
#define RECIPEBOOK_LIST_SCROLL_ARROW_TAG 113

#define RECIPEBOOK_INDICATOR_TILE_INACTIVE 0x0C
#define RECIPEBOOK_INDICATOR_TILE_ACTIVE 0x0D
#define RECIPEBOOK_INDICATOR_TILE_RIGHT_END_ACTIVE 0x12
#define RECIPEBOOK_INDICATOR_TILE_LEFT_END_INACTIVE 0x15
#define RECIPEBOOK_INDICATOR_TILE_LINK_INACTIVE 0x14
#define RECIPEBOOK_INDICATOR_TILE_LINK_LEFT_ACTIVE 0x13

#define RECIPEBOOK_POCKET_NAME_TEXT_X_OFFSET 4
#define RECIPEBOOK_POCKET_NAME_TEXT_Y_OFFSET 0
#define RECIPEBOOK_LIST_TEXT_X_OFFSET 0
#define RECIPEBOOK_LIST_TEXT_Y_OFFSET 0
#define RECIPEBOOK_INGREDIENTS_TEXT_X_OFFSET 2
#define RECIPEBOOK_INGREDIENTS_TEXT_Y_OFFSET 1
#define RECIPEBOOK_INGREDIENTS_LIST_X_OFFSET 11

#define RECIPEBOOK_INGREDIENTS_HEADER_Y 0
#define RECIPEBOOK_INGREDIENTS_HEADER_TO_PAGING_GAP -3
#define RECIPEBOOK_INGREDIENTS_PAGING_TO_LIST_GAP 0
#define RECIPEBOOK_INGREDIENTS_HEADER_TO_LIST_GAP (RECIPEBOOK_INGREDIENTS_HEADER_TO_PAGING_GAP + RECIPEBOOK_INGREDIENTS_PAGING_TO_LIST_GAP)
#define RECIPEBOOK_INGREDIENTS_MAX 9
#define RECIPEBOOK_INGREDIENTS_LINE_BUFFER (ITEM_NAME_LENGTH + 8)
#define RECIPEBOOK_INGREDIENTS_PAGE_BUFFER 32
#define RECIPEBOOK_INGREDIENTS_PER_PAGE 3
#define RECIPEBOOK_INGREDIENTS_LINE_SPACING -2

#define RECIPEBOOK_VARIANT_INDICATOR_RIGHT_PADDING 8
#define RECIPEBOOK_VARIANT_INDICATOR_GAP 7
#define RECIPEBOOK_VARIANT_INDICATOR_Y_OFFSET -3
#define RECIPEBOOK_VARIANT_INDICATOR_LEFT_ARROW_GAP 1
#define RECIPEBOOK_VARIANT_INDICATOR_RIGHT_ARROW_GAP 1
#define RECIPEBOOK_VARIANT_INDICATOR_LEFT_ARROW_X_OFFSET 1

enum
{
    RECIPEBOOK_COLOR_POCKET_NAME,
    RECIPEBOOK_COLOR_INGREDIENT_HEADER,
    RECIPEBOOK_COLOR_INGREDIENT_HAVE,
    RECIPEBOOK_COLOR_INGREDIENT_MISS
};

static const u8 sRecipeBookFontColorTable[][3] =
{
    [RECIPEBOOK_COLOR_POCKET_NAME] = {TEXT_COLOR_TRANSPARENT, TEXT_DYNAMIC_COLOR_1, TEXT_COLOR_DARK_GRAY},
    [RECIPEBOOK_COLOR_INGREDIENT_HEADER] = {TEXT_COLOR_TRANSPARENT, TEXT_DYNAMIC_COLOR_1, TEXT_COLOR_DARK_GRAY},
    [RECIPEBOOK_COLOR_INGREDIENT_HAVE] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_GREEN, TEXT_COLOR_LIGHT_GREEN},
    [RECIPEBOOK_COLOR_INGREDIENT_MISS] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_RED, TEXT_COLOR_LIGHT_RED}
};

static const struct ScrollArrowsTemplate sRecipeBookPocketArrowTemplate =
{
    .firstArrowType = SCROLL_ARROW_LEFT,
    .firstX = 28,
    .firstY = 16,
    .secondArrowType = SCROLL_ARROW_RIGHT,
    .secondX = 108,
    .secondY = 16,
    .fullyUpThreshold = -1,
    .fullyDownThreshold = -1,
    .tileTag = RECIPEBOOK_POCKET_SCROLL_ARROW_TAG,
    .palTag = RECIPEBOOK_POCKET_SCROLL_ARROW_TAG,
    .palNum = 0,
};

static const struct ScrollArrowsTemplate sRecipeBookListScrollArrowTemplate =
{
    .firstArrowType = SCROLL_ARROW_UP,
    .firstX = 68,
    .firstY = 45,
    .secondArrowType = SCROLL_ARROW_DOWN,
    .secondX = 68,
    .secondY = 147,
    .fullyUpThreshold = -1,
    .fullyDownThreshold = -1,
    .tileTag = RECIPEBOOK_LIST_SCROLL_ARROW_TAG,
    .palTag = RECIPEBOOK_LIST_SCROLL_ARROW_TAG,
    .palNum = 0,
};

static const struct ListMenuTemplate sRecipeBookListMenuTemplate =
{
    .items = NULL,
    .moveCursorFunc = NULL,
    .itemPrintFunc = RecipeBook_PrintListItem,
    .totalItems = 0,
    .maxShowed = 6,
    .windowId = RECIPEBOOK_WIN_LIST,
    .header_X = (0 + RECIPEBOOK_LIST_TEXT_X_OFFSET),
    .item_X = (8 + RECIPEBOOK_LIST_TEXT_X_OFFSET),
    .cursor_X = (0 + RECIPEBOOK_LIST_TEXT_X_OFFSET),
    .upText_Y = (1 + RECIPEBOOK_LIST_TEXT_Y_OFFSET),
    .cursorPal = TEXT_DYNAMIC_COLOR_1,
    .fillValue = 0,
    .cursorShadowPal = TEXT_COLOR_DARK_GRAY,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = LIST_NO_MULTIPLE_SCROLL,
    .fontId = FONT_NARROW,
    .cursorKind = CURSOR_BLACK_ARROW
};

EWRAM_DATA static u8 sRecipeBookPockets[POCKETS_COUNT];
EWRAM_DATA static bool8 sRecipeBookPocketHasRecipes[POCKETS_COUNT];
EWRAM_DATA static u8 sRecipeBookPocketCount;
EWRAM_DATA static u8 sRecipeBookPocketIndex;
EWRAM_DATA static u16 sRecipeBookItemCount;
EWRAM_DATA static struct ListMenuItem sRecipeBookListItems[MAX_RECIPEBOOK_ITEMS];
EWRAM_DATA static u8 sRecipeBookListTaskId;
EWRAM_DATA static u16 sRecipeBookListScroll;
EWRAM_DATA static u16 sRecipeBookListCursor;
EWRAM_DATA static u16 sRecipeBookPocketScroll[POCKETS_COUNT];
EWRAM_DATA static u16 sRecipeBookPocketCursor[POCKETS_COUNT];
EWRAM_DATA static u8 ALIGNED(4) sRecipeBookPocketNameBuffer[32][32];
EWRAM_DATA static u8 sRecipeBookPocketArrowTask;
EWRAM_DATA static u16 sRecipeBookPocketArrowPos;
EWRAM_DATA static u8 sRecipeBookBg2TilemapBuffer[BG_SCREEN_SIZE];
EWRAM_DATA static u8 sRecipeBookPatternSpriteIds[CRAFT_SLOT_COUNT];
EWRAM_DATA static u16 sRecipeBookPreviewItemId;
EWRAM_DATA static u8 sRecipeBookPreviewVariantIndex;
EWRAM_DATA static u8 sRecipeBookListArrowTask;
EWRAM_DATA static u16 sRecipeBookPendingItemId;
EWRAM_DATA static u8 sRecipeBookPendingVariantIndex;
EWRAM_DATA static u8 sRecipeBookPatternDelay;
EWRAM_DATA static u16 sRecipeBookIngredientsItemId;
EWRAM_DATA static u8 sRecipeBookIngredientsPage;
EWRAM_DATA static u8 sRecipeBookIngredientsTotalPages;
static const struct CraftRecipe *sRecipeBookIngredientsRecipe;
static bool8 sRecipeBookVariantModeEnabled;
EWRAM_DATA static u8 sRecipeBookVariantIndex[ITEMS_COUNT];
static bool8 sRecipeBookTimeBlendOverridden;

static const struct BgTemplate sRecipeBookBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 3,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    }
};

static void RecipeBookMainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void RecipeBookVBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_CraftRecipeBookMenu(void)
{
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        gMain.state++;
        break;
    case 1:
        ResetVramOamAndBgCntRegs();
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sRecipeBookBgTemplates, ARRAY_COUNT(sRecipeBookBgTemplates));
        ResetAllBgsCoordinates();
        FreeAllWindowBuffers();
        DeactivateAllTextPrinters();
        ResetTasks();
        ResetSpriteData();
        SetBackdropFromColor(RGB_BLACK);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        FillBgTilemapBufferRect(0, 0, 0, 0, DISPLAY_TILE_WIDTH, DISPLAY_TILE_HEIGHT, 15);
        SetBgTilemapBuffer(2, sRecipeBookBg2TilemapBuffer);
        memset(sRecipeBookBg2TilemapBuffer, 0, sizeof(sRecipeBookBg2TilemapBuffer));
        LoadBgTiles(2, gRecipeBookMenu_Gfx, RECIPEBOOK_BG_TILE_BYTES, 0);
        CopyToBgTilemapBuffer(2, gRecipeBookMenu_Tilemap, RECIPEBOOK_BG_TILEMAP_BYTES, 0);
        CopyBgTilemapBufferToVram(2);
        LoadPalette(gRecipeBookMenu_Pal, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        RecipeBook_Init();
        gMain.state++;
        break;
    case 3:
        CreateTask(Task_RecipeBookMenu, 0);
        SetVBlankCallback(RecipeBookVBlankCB);
        SetMainCallback2(RecipeBookMainCB2);
        gMain.state++;
        break;
    case 4:
        break;
    }
}

static bool8 RecipeBook_IsRecipeUnlocked(const struct CraftRecipeList *list)
{
    u8 i;

    for (i = 0; i < list->count; i++)
    {
        u16 flag = list->recipes[i].unlockFlag;
        if (flag == 0 || FlagGet(flag))
            return TRUE;
    }

    return FALSE;
}

static void RecipeBook_BuildPocketList(void)
{
    u16 itemId;
    u8 pocket;

    for (pocket = 0; pocket < POCKETS_COUNT; pocket++)
        sRecipeBookPocketHasRecipes[pocket] = FALSE;

    for (itemId = 0; itemId < gCraftRecipeCount; itemId++)
    {
        const struct CraftRecipeList *list = &gCraftRecipes[itemId];

        if (list->count == 0)
            continue;
        if (!RecipeBook_IsRecipeUnlocked(list))
            continue;

        pocket = GetItemPocket(itemId);
        if (pocket >= POCKETS_COUNT)
            continue;
        sRecipeBookPocketHasRecipes[pocket] = TRUE;
    }

    sRecipeBookPocketCount = 0;
    for (pocket = 0; pocket < POCKETS_COUNT; pocket++)
    {
        if (sRecipeBookPocketHasRecipes[pocket])
            sRecipeBookPockets[sRecipeBookPocketCount++] = pocket;
    }

    if (sRecipeBookPocketIndex >= sRecipeBookPocketCount)
        sRecipeBookPocketIndex = 0;
}

static void RecipeBook_BuildItemList(enum Pocket pocketId)
{
    u16 itemId;

    sRecipeBookItemCount = 0;
    for (itemId = 0; itemId < gCraftRecipeCount; itemId++)
    {
        const struct CraftRecipeList *list = &gCraftRecipes[itemId];

        if (list->count == 0)
            continue;
        if (!RecipeBook_IsRecipeUnlocked(list))
            continue;
        if (GetItemPocket(itemId) != pocketId)
            continue;

        if (sRecipeBookItemCount >= MAX_RECIPEBOOK_ITEMS - 1)
            break;

        sRecipeBookListItems[sRecipeBookItemCount].name = gText_EmptyString7;
        sRecipeBookListItems[sRecipeBookItemCount].id = itemId;
        sRecipeBookItemCount++;
    }

    if (sRecipeBookItemCount == 0)
    {
        sRecipeBookListItems[0].name = gText_EmptyString7;
        sRecipeBookListItems[0].id = ITEM_NONE;
        sRecipeBookItemCount = 1;
    }
    else
    {
        if (sRecipeBookItemCount < MAX_RECIPEBOOK_ITEMS)
        {
            sRecipeBookListItems[sRecipeBookItemCount].name = gText_EmptyString7;
            sRecipeBookListItems[sRecipeBookItemCount].id = LIST_CANCEL;
            sRecipeBookItemCount++;
        }
    }
}

static void RecipeBook_InitWindows(void)
{
    InitWindows(sRecipeBookWindowTemplates);
    ListMenuLoadStdPalAt(BG_PLTT_ID(12), 1);
    LoadPalette(&gStandardMenuPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);

    sRecipeBookWindowIds[RECIPEBOOK_WIN_POCKET_NAME] = AddWindow(&sRecipeBookWindowTemplates[RECIPEBOOK_WIN_POCKET_NAME]);
    sRecipeBookWindowIds[RECIPEBOOK_WIN_LIST] = AddWindow(&sRecipeBookWindowTemplates[RECIPEBOOK_WIN_LIST]);
    sRecipeBookWindowIds[RECIPEBOOK_WIN_INGREDIENTS] = AddWindow(&sRecipeBookWindowTemplates[RECIPEBOOK_WIN_INGREDIENTS]);

    FillWindowPixelBuffer(sRecipeBookWindowIds[RECIPEBOOK_WIN_POCKET_NAME], PIXEL_FILL(0));
    PutWindowTilemap(sRecipeBookWindowIds[RECIPEBOOK_WIN_POCKET_NAME]);
    CopyWindowToVram(sRecipeBookWindowIds[RECIPEBOOK_WIN_POCKET_NAME], COPYWIN_FULL);

    FillWindowPixelBuffer(sRecipeBookWindowIds[RECIPEBOOK_WIN_LIST], PIXEL_FILL(0));
    PutWindowTilemap(sRecipeBookWindowIds[RECIPEBOOK_WIN_LIST]);
    CopyWindowToVram(sRecipeBookWindowIds[RECIPEBOOK_WIN_LIST], COPYWIN_FULL);

    DrawStdFrameWithCustomTileAndPalette(sRecipeBookWindowIds[RECIPEBOOK_WIN_INGREDIENTS], TRUE, 0x214, 14);
    FillWindowPixelBuffer(sRecipeBookWindowIds[RECIPEBOOK_WIN_INGREDIENTS], PIXEL_FILL(1));
    PutWindowTilemap(sRecipeBookWindowIds[RECIPEBOOK_WIN_INGREDIENTS]);
    CopyWindowToVram(sRecipeBookWindowIds[RECIPEBOOK_WIN_INGREDIENTS], COPYWIN_FULL);
}

static void RecipeBook_DestroyWindows(void)
{
    u8 i;

    if (sRecipeBookListTaskId != TASK_NONE)
    {
        DestroyListMenuTask(sRecipeBookListTaskId, &sRecipeBookListScroll, &sRecipeBookListCursor);
        sRecipeBookListTaskId = TASK_NONE;
    }

    for (i = 0; i < RECIPEBOOK_WIN_COUNT; i++)
    {
        if (sRecipeBookWindowIds[i] != WINDOW_NONE)
        {
            ClearStdWindowAndFrameToTransparent(sRecipeBookWindowIds[i], TRUE);
            RemoveWindow(sRecipeBookWindowIds[i]);
            sRecipeBookWindowIds[i] = WINDOW_NONE;
        }
    }
}

static void RecipeBook_RefreshListMenu(void)
{
    struct ListMenuTemplate listTemplate = sRecipeBookListMenuTemplate;
    enum Pocket pocketId = POCKET_ITEMS;

    if (sRecipeBookPocketCount != 0)
        pocketId = sRecipeBookPockets[sRecipeBookPocketIndex];
    RecipeBook_BuildItemList(pocketId);

    FillWindowPixelBuffer(sRecipeBookWindowIds[RECIPEBOOK_WIN_LIST], PIXEL_FILL(0));
    CopyWindowToVram(sRecipeBookWindowIds[RECIPEBOOK_WIN_LIST], COPYWIN_FULL);

    if (sRecipeBookListTaskId != TASK_NONE)
    {
        DestroyListMenuTask(sRecipeBookListTaskId, &sRecipeBookListScroll, &sRecipeBookListCursor);
        sRecipeBookListTaskId = TASK_NONE;
    }

    if (sRecipeBookPocketCount != 0)
    {
        sRecipeBookListScroll = sRecipeBookPocketScroll[pocketId];
        sRecipeBookListCursor = sRecipeBookPocketCursor[pocketId];
        SetCursorWithinListBounds(&sRecipeBookListScroll, &sRecipeBookListCursor, listTemplate.maxShowed, sRecipeBookItemCount);
        sRecipeBookPocketScroll[pocketId] = sRecipeBookListScroll;
        sRecipeBookPocketCursor[pocketId] = sRecipeBookListCursor;
    }
    else
    {
        sRecipeBookListScroll = 0;
        sRecipeBookListCursor = 0;
    }

    listTemplate.items = sRecipeBookListItems;
    listTemplate.totalItems = sRecipeBookItemCount;
    listTemplate.windowId = sRecipeBookWindowIds[RECIPEBOOK_WIN_LIST];
    sRecipeBookListTaskId = ListMenuInit(&listTemplate, sRecipeBookListScroll, sRecipeBookListCursor);
    RecipeBook_UpdateListScrollArrows();
}

static void RecipeBook_SavePocketListState(void)
{
    enum Pocket pocketId;

    if (sRecipeBookPocketCount == 0)
        return;

    pocketId = sRecipeBookPockets[sRecipeBookPocketIndex];
    sRecipeBookPocketScroll[pocketId] = sRecipeBookListScroll;
    sRecipeBookPocketCursor[pocketId] = sRecipeBookListCursor;
}

static void RecipeBook_Print(u8 windowId, u8 fontId, const u8 *str, u8 left, u8 top, u8 colorIndex)
{
    AddTextPrinterParameterized4(windowId, fontId, left, top, 0, 0, sRecipeBookFontColorTable[colorIndex], TEXT_SKIP_DRAW, str);
}

static void RecipeBook_PrintPocketNames(const u8 *pocketName1, const u8 *pocketName2)
{
    struct WindowTemplate window = {0};
    u16 windowId;
    int offset;

    window.width = 16;
    window.height = 2;
    windowId = AddWindow(&window);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
    offset = GetStringCenterAlignXOffset(FONT_NORMAL, pocketName1, 0x40) + RECIPEBOOK_POCKET_NAME_TEXT_X_OFFSET;
    RecipeBook_Print(windowId, FONT_NORMAL, pocketName1, offset, 1 + RECIPEBOOK_POCKET_NAME_TEXT_Y_OFFSET,
                     RECIPEBOOK_COLOR_POCKET_NAME);
    if (pocketName2)
    {
        offset = GetStringCenterAlignXOffset(FONT_NORMAL, pocketName2, 0x40) + RECIPEBOOK_POCKET_NAME_TEXT_X_OFFSET;
        RecipeBook_Print(windowId, FONT_NORMAL, pocketName2, offset + 0x40, 1 + RECIPEBOOK_POCKET_NAME_TEXT_Y_OFFSET,
                         RECIPEBOOK_COLOR_POCKET_NAME);
    }
    CpuCopy32((u8 *)GetWindowAttribute(windowId, WINDOW_TILE_DATA), sRecipeBookPocketNameBuffer, sizeof(sRecipeBookPocketNameBuffer));
    RemoveWindow(windowId);
}

static void RecipeBook_CopyPocketNameToWindow(u32 offset)
{
    u8 (*tileDataBuffer)[32][32];
    u8 *windowTileData;
    int bottomOffset;

    if (offset > 8)
        offset = 8;
    tileDataBuffer = &sRecipeBookPocketNameBuffer;
    windowTileData = (u8 *)GetWindowAttribute(sRecipeBookWindowIds[RECIPEBOOK_WIN_POCKET_NAME], WINDOW_TILE_DATA);
    CpuCopy32(&tileDataBuffer[0][offset], windowTileData, 0x100);
    bottomOffset = offset + 16;
    CpuCopy32(&tileDataBuffer[0][bottomOffset], windowTileData + 0x100, 0x100);
    CopyWindowToVram(sRecipeBookWindowIds[RECIPEBOOK_WIN_POCKET_NAME], COPYWIN_GFX);
}

static u16 RecipeBook_GetSelectedItemId(void)
{
    u16 index = sRecipeBookListScroll + sRecipeBookListCursor;

    if (index >= sRecipeBookItemCount)
        return ITEM_NONE;

    return sRecipeBookListItems[index].id;
}

static const struct CraftRecipe *RecipeBook_GetPreviewRecipe(u16 itemId)
{
    u8 variantIndex = RecipeBook_GetVariantIndex(itemId);

    return RecipeBook_GetRecipeVariant(itemId, variantIndex);
}

static void RecipeBook_ClearPatternIcons(void)
{
    int i;

    for (i = 0; i < CRAFT_SLOT_COUNT; i++)
    {
        if (sRecipeBookPatternSpriteIds[i] != SPRITE_NONE)
        {
            DestroySprite(&gSprites[sRecipeBookPatternSpriteIds[i]]);
            FreeSpriteTilesByTag(TAG_RECIPEBOOK_PATTERN_ICON_BASE + i);
            FreeSpritePaletteByTag(TAG_RECIPEBOOK_PATTERN_ICON_BASE + i);
            sRecipeBookPatternSpriteIds[i] = SPRITE_NONE;
        }
    }
}

static void RecipeBook_UpdatePatternPreview(void)
{
    const struct CraftRecipe *recipe;
    u16 itemId;
    u8 variantIndex;
    int r, c;

    itemId = RecipeBook_GetSelectedItemId();
    variantIndex = RecipeBook_GetVariantIndex(itemId);
    if (itemId == sRecipeBookPreviewItemId && variantIndex == sRecipeBookPreviewVariantIndex)
        return;

    RecipeBook_ClearPatternIcons();
    sRecipeBookPreviewItemId = itemId;
    sRecipeBookPreviewVariantIndex = variantIndex;
    recipe = RecipeBook_GetPreviewRecipe(itemId);
    if (recipe == NULL)
        return;

    for (r = 0; r < CRAFT_ROWS; r++)
    {
        for (c = 0; c < CRAFT_COLS; c++)
        {
            u16 ingredientId = recipe->pattern[r][c];
            u8 slot = r * CRAFT_COLS + c;

            if (ingredientId != ITEM_NONE)
            {
                u16 tag = TAG_RECIPEBOOK_PATTERN_ICON_BASE + slot;
                u8 spriteId = AddItemIconSprite(tag, tag, ingredientId);
                s16 x;
                s16 y;

                if (spriteId != MAX_SPRITES)
                {
                    UpdateSpritePaletteWithTime(IndexOfSpritePaletteTag(tag));
                    CraftMenuUI_GetItemIconPos(slot, RECIPEBOOK_PATTERN_BASE_X, RECIPEBOOK_PATTERN_BASE_Y, &x, &y);
                    gSprites[spriteId].x = x;
                    gSprites[spriteId].y = y;
                    gSprites[spriteId].oam.priority = 0;
                    sRecipeBookPatternSpriteIds[slot] = spriteId;
                }
            }
        }
    }
}

static void RecipeBook_SchedulePatternPreviewWithDelay(u8 delay)
{
    u16 itemId = RecipeBook_GetSelectedItemId();
    u8 variantIndex = RecipeBook_GetVariantIndex(itemId);

    if (itemId == sRecipeBookPreviewItemId
        && variantIndex == sRecipeBookPreviewVariantIndex
        && sRecipeBookPatternDelay == 0)
        return;
    if (itemId == sRecipeBookPendingItemId
        && variantIndex == sRecipeBookPendingVariantIndex
        && sRecipeBookPatternDelay != 0)
        return;

    RecipeBook_ClearPatternIcons();
    sRecipeBookPreviewItemId = ITEM_NONE;
    sRecipeBookPendingItemId = itemId;
    sRecipeBookPendingVariantIndex = variantIndex;
    sRecipeBookPatternDelay = delay;
}

static void RecipeBook_SchedulePatternPreview(void)
{
    RecipeBook_SchedulePatternPreviewWithDelay(2);
}

static void RecipeBook_TickPatternPreview(void)
{
    u16 itemId;
    u8 variantIndex;

    if (sRecipeBookPatternDelay == 0)
        return;

    sRecipeBookPatternDelay--;
    if (sRecipeBookPatternDelay != 0)
        return;

    itemId = RecipeBook_GetSelectedItemId();
    variantIndex = RecipeBook_GetVariantIndex(itemId);
    if (sRecipeBookPendingItemId != itemId || sRecipeBookPendingVariantIndex != variantIndex)
    {
        RecipeBook_SchedulePatternPreview();
        return;
    }

    RecipeBook_UpdatePatternPreview();
}

static s32 RecipeBook_FindBreakPos(const u8 *str, u32 maxWidth, u8 fontId)
{
    s32 best = -1;
    u32 i;
    u8 temp[RECIPEBOOK_INGREDIENTS_LINE_BUFFER];

    for (i = 0; str[i] != EOS; i++)
    {
        if (str[i] == CHAR_SPACE)
        {
            StringCopyN(temp, str, i);
            temp[i] = EOS;
            if (GetStringWidth(fontId, temp, 0) <= maxWidth)
                best = i;
            else
                break;
        }
    }

    return best;
}

static void RecipeBook_TruncateIngredientLine(const u8 *src, u8 *dest, u32 width, u8 fontId)
{
    u32 ellipsisWidth = GetStringWidth(fontId, sText_RecipeBookEllipsis, 0);
    u32 maxWidth = (width > ellipsisWidth) ? (width - ellipsisWidth) : 0;
    s32 truncPos = RecipeBook_FindBreakPos(src, maxWidth, fontId);

    if (truncPos < 0)
    {
        s32 i;
        u8 temp[RECIPEBOOK_INGREDIENTS_LINE_BUFFER];

        if (maxWidth == 0)
        {
            StringCopy(dest, sText_RecipeBookEllipsis);
            return;
        }

        truncPos = -1;
        for (i = 0; src[i] != EOS; i++)
        {
            StringCopyN(temp, src, i + 1);
            temp[i + 1] = EOS;
            if (GetStringWidth(fontId, temp, 0) > maxWidth)
                break;
            truncPos = i;
        }

        if (truncPos < 0)
        {
            StringCopy(dest, sText_RecipeBookEllipsis);
        }
        else
        {
            StringCopyN(dest, src, truncPos + 1);
            dest[truncPos + 1] = EOS;
            StringAppend(dest, sText_RecipeBookEllipsis);
        }
    }
    else
    {
        StringCopyN(dest, src, truncPos);
        dest[truncPos] = EOS;
        StringAppend(dest, sText_RecipeBookEllipsis);
    }
}

static u8 RecipeBook_GetUniqueIngredientCount(const struct CraftRecipe *recipe)
{
    u16 ingredientIds[RECIPEBOOK_INGREDIENTS_MAX];
    u8 ingredientCount = 0;
    u8 i;
    int r, c;

    if (recipe == NULL)
        return 0;

    for (r = 0; r < CRAFT_ROWS; r++)
    {
        for (c = 0; c < CRAFT_COLS; c++)
        {
            u16 ingredientId = recipe->pattern[r][c];

            if (ingredientId == ITEM_NONE)
                continue;

            for (i = 0; i < ingredientCount; i++)
            {
                if (ingredientIds[i] == ingredientId)
                    break;
            }

            if (i == ingredientCount && ingredientCount < RECIPEBOOK_INGREDIENTS_MAX)
            {
                ingredientIds[ingredientCount] = ingredientId;
                ingredientCount++;
            }
        }
    }

    return ingredientCount;
}

static u8 RecipeBook_GetUnlockedVariantCount(u16 itemId)
{
    const struct CraftRecipeList *list;
    u8 count = 0;
    u8 i;

    if (itemId == ITEM_NONE || itemId == LIST_CANCEL)
        return 0;
    if (itemId >= gCraftRecipeCount)
        return 0;

    list = &gCraftRecipes[itemId];
    if (list->count == 0)
        return 0;

    for (i = 0; i < list->count; i++)
    {
        const struct CraftRecipe *recipe = &list->recipes[i];
        if (recipe->unlockFlag == 0 || FlagGet(recipe->unlockFlag))
            count++;
    }

    return count;
}

static u8 RecipeBook_GetVariantIndex(u16 itemId)
{
    u8 count;

    if (itemId >= ITEMS_COUNT)
        return 0;

    count = RecipeBook_GetUnlockedVariantCount(itemId);
    if (count == 0)
        return 0;

    if (sRecipeBookVariantIndex[itemId] >= count)
        sRecipeBookVariantIndex[itemId] = count - 1;

    return sRecipeBookVariantIndex[itemId];
}

static const struct CraftRecipe *RecipeBook_GetRecipeVariant(u16 itemId, u8 variantIndex)
{
    const struct CraftRecipeList *list;
    u8 count = 0;
    u8 i;

    if (itemId == ITEM_NONE || itemId == LIST_CANCEL)
        return NULL;
    if (itemId >= gCraftRecipeCount)
        return NULL;

    list = &gCraftRecipes[itemId];
    if (list->count == 0)
        return NULL;

    for (i = 0; i < list->count; i++)
    {
        const struct CraftRecipe *recipe = &list->recipes[i];
        if (recipe->unlockFlag == 0 || FlagGet(recipe->unlockFlag))
        {
            if (count == variantIndex)
                return recipe;
            count++;
        }
    }

    return NULL;
}

static void RecipeBook_PrintListItem(u8 windowId, u32 itemId, u8 y)
{
    u8 name[ITEM_NAME_LENGTH + 1];
    u8 render[ITEM_NAME_LENGTH + 8];
    u8 fraction[RECIPEBOOK_INGREDIENTS_PAGE_BUFFER];
    u8 maxFraction[RECIPEBOOK_INGREDIENTS_PAGE_BUFFER];
    u8 fontId = sRecipeBookListMenuTemplate.fontId;
    u8 indicatorFontId = FONT_SMALL_NARROWER;
    u8 bigH = GetFontAttribute(fontId, FONTATTR_MAX_LETTER_HEIGHT);
    u8 smallH = GetFontAttribute(indicatorFontId, FONTATTR_MAX_LETTER_HEIGHT);
    u8 nameX = sRecipeBookListMenuTemplate.item_X;
    u32 windowWidth = WindowWidthPx(windowId);
    u32 maxNameWidth;
    u8 colors[3] = {sRecipeBookListMenuTemplate.fillValue, sRecipeBookListMenuTemplate.cursorPal, sRecipeBookListMenuTemplate.cursorShadowPal};
    u8 variantCount;

    if (itemId == LIST_CANCEL)
    {
        AddTextPrinterParameterized4(windowId, fontId, nameX, y, 0, 0, colors, TEXT_SKIP_DRAW, sText_RecipeBookClose);
        return;
    }
    if (itemId == ITEM_NONE && sRecipeBookItemCount == 1)
    {
        AddTextPrinterParameterized4(windowId, fontId, nameX, y, 0, 0, colors, TEXT_SKIP_DRAW, sText_RecipeBookNoRecipes);
        return;
    }
    if (itemId >= gCraftRecipeCount)
        return;

    variantCount = RecipeBook_GetUnlockedVariantCount(itemId);

    if (variantCount > 1)
    {
        u8 variantIndex = RecipeBook_GetVariantIndex(itemId);
        u8 indicatorY = y + (bigH - smallH) / 2 + RECIPEBOOK_VARIANT_INDICATOR_Y_OFFSET;
        u8 indicatorColors[3];
        u32 arrowWidth;
        u32 fractionWidth;
        u32 maxFractionWidth;
        u32 indicatorBaseX;
        u32 indicatorX;
        u32 rightArrowX;
        u32 fractionX;
        u32 leftArrowX;
        u8 leftArrowStr[] = {CHAR_LEFT_ARROW, EOS};
        u8 rightArrowStr[] = {CHAR_RIGHT_ARROW, EOS};
        u8 *ptr;

        ptr = fraction;
        ptr = ConvertIntToDecimalStringN(ptr, variantIndex + 1, STR_CONV_MODE_LEFT_ALIGN, 2);
        *ptr++ = CHAR_SLASH;
        ptr = ConvertIntToDecimalStringN(ptr, variantCount, STR_CONV_MODE_LEFT_ALIGN, 2);
        *ptr = EOS;

        ptr = maxFraction;
        ptr = ConvertIntToDecimalStringN(ptr, variantCount, STR_CONV_MODE_LEFT_ALIGN, 2);
        *ptr++ = CHAR_SLASH;
        ptr = ConvertIntToDecimalStringN(ptr, variantCount, STR_CONV_MODE_LEFT_ALIGN, 2);
        *ptr = EOS;

        arrowWidth = GetStringWidth(indicatorFontId, leftArrowStr, 0);
        fractionWidth = GetStringWidth(indicatorFontId, fraction, 0);
        maxFractionWidth = GetStringWidth(indicatorFontId, maxFraction, 0);

        indicatorBaseX = windowWidth - RECIPEBOOK_VARIANT_INDICATOR_RIGHT_PADDING - maxFractionWidth;
        maxNameWidth = (indicatorBaseX > nameX + RECIPEBOOK_VARIANT_INDICATOR_GAP)
            ? (indicatorBaseX - nameX - RECIPEBOOK_VARIANT_INDICATOR_GAP)
            : 0;
        indicatorX = indicatorBaseX;

        fractionX = indicatorX;
        leftArrowX = fractionX - RECIPEBOOK_VARIANT_INDICATOR_LEFT_ARROW_GAP - arrowWidth
                   + RECIPEBOOK_VARIANT_INDICATOR_LEFT_ARROW_X_OFFSET;
        rightArrowX = fractionX + maxFractionWidth + RECIPEBOOK_VARIANT_INDICATOR_RIGHT_ARROW_GAP;

        {
            u32 fitFontId;
            u8 *end;

            CopyItemName(itemId, name);
            fitFontId = GetFontIdToFit(name, fontId, 0, maxNameWidth);
            if (GetStringWidth(fitFontId, name, 0) <= maxNameWidth)
                StringCopy(render, name);
            else
                RecipeBook_TruncateIngredientLine(name, render, maxNameWidth, fitFontId);

            end = render + StringLength(render);
            if (fitFontId != fontId)
                PrependFontIdToFit(render, end, fontId, maxNameWidth);

            AddTextPrinterParameterized4(windowId, fontId, nameX, y, 0, 0, colors, TEXT_SKIP_DRAW, render);
        }

        if (sRecipeBookVariantModeEnabled)
        {
            indicatorColors[0] = TEXT_COLOR_TRANSPARENT;
            indicatorColors[1] = TEXT_DYNAMIC_COLOR_1;
            indicatorColors[2] = TEXT_COLOR_DARK_GRAY;
        }
        else
        {
            indicatorColors[0] = TEXT_COLOR_TRANSPARENT;
            indicatorColors[1] = TEXT_DYNAMIC_COLOR_1;
            indicatorColors[2] = TEXT_COLOR_DARK_GRAY;
        }

        if (sRecipeBookVariantModeEnabled)
        {
            AddTextPrinterParameterized4(windowId, indicatorFontId, leftArrowX, indicatorY, 0, 0,
                                         indicatorColors, TEXT_SKIP_DRAW, leftArrowStr);
            AddTextPrinterParameterized4(windowId, indicatorFontId, rightArrowX, indicatorY, 0, 0,
                                         indicatorColors, TEXT_SKIP_DRAW, rightArrowStr);
        }

        if (fractionWidth <= maxFractionWidth)
        {
            AddTextPrinterParameterized4(windowId, indicatorFontId, fractionX, indicatorY, 0, 0,
                                         indicatorColors, TEXT_SKIP_DRAW, fraction);
        }
    }
    else
    {
        maxNameWidth = (windowWidth > nameX + RECIPEBOOK_VARIANT_INDICATOR_RIGHT_PADDING)
            ? (windowWidth - nameX - RECIPEBOOK_VARIANT_INDICATOR_RIGHT_PADDING)
            : 0;
        {
            u32 fitFontId;
            u8 *end;

            CopyItemName(itemId, name);
            fitFontId = GetFontIdToFit(name, fontId, 0, maxNameWidth);
            if (GetStringWidth(fitFontId, name, 0) <= maxNameWidth)
                StringCopy(render, name);
            else
                RecipeBook_TruncateIngredientLine(name, render, maxNameWidth, fitFontId);

            end = render + StringLength(render);
            if (fitFontId != fontId)
                PrependFontIdToFit(render, end, fontId, maxNameWidth);

            AddTextPrinterParameterized4(windowId, fontId, nameX, y, 0, 0, colors, TEXT_SKIP_DRAW, render);
        }
    }
}

static void RecipeBook_RedrawListRow(u16 itemIndex, u8 row)
{
    u8 windowId = sRecipeBookWindowIds[RECIPEBOOK_WIN_LIST];
    u8 yMultiplier = GetFontAttribute(sRecipeBookListMenuTemplate.fontId, FONTATTR_MAX_LETTER_HEIGHT)
                   + sRecipeBookListMenuTemplate.itemVerticalPadding;
    u8 y = row * yMultiplier + sRecipeBookListMenuTemplate.upText_Y;
    u8 x = sRecipeBookListMenuTemplate.item_X;
    u8 width = WindowWidthPx(windowId) - x;
    u8 height = yMultiplier;

    if (windowId == WINDOW_NONE)
        return;
    if (itemIndex >= sRecipeBookItemCount)
        return;

    FillWindowPixelRect(windowId, PIXEL_FILL(sRecipeBookListMenuTemplate.fillValue), x, y, width, height);
    RecipeBook_PrintListItem(windowId, sRecipeBookListItems[itemIndex].id, y);
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void RecipeBook_RedrawVisibleListRows(void)
{
    u8 windowId = sRecipeBookWindowIds[RECIPEBOOK_WIN_LIST];
    u8 row;
    u16 index = sRecipeBookListScroll;
    u8 visible = sRecipeBookListMenuTemplate.maxShowed;
    u8 yMultiplier = GetFontAttribute(sRecipeBookListMenuTemplate.fontId, FONTATTR_MAX_LETTER_HEIGHT)
                   + sRecipeBookListMenuTemplate.itemVerticalPadding;
    u8 x = sRecipeBookListMenuTemplate.item_X;
    u8 width = WindowWidthPx(windowId) - x;
    u8 height = yMultiplier;

    if (windowId == WINDOW_NONE)
        return;

    for (row = 0; row < visible && index < sRecipeBookItemCount; row++, index++)
    {
        u8 y = row * yMultiplier + sRecipeBookListMenuTemplate.upText_Y;
        FillWindowPixelRect(windowId, PIXEL_FILL(sRecipeBookListMenuTemplate.fillValue), x, y, width, height);
        RecipeBook_PrintListItem(windowId, sRecipeBookListItems[index].id, y);
    }

    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void RecipeBook_UpdateIngredientsWindow(void)
{
    u8 windowId = sRecipeBookWindowIds[RECIPEBOOK_WIN_INGREDIENTS];
    const struct CraftRecipe *recipe;
    u16 itemId;
    u16 ingredientIds[RECIPEBOOK_INGREDIENTS_MAX];
    u8 ingredientCounts[RECIPEBOOK_INGREDIENTS_MAX];
    u8 ingredientCount = 0;
    u8 i;
    u8 pageText[RECIPEBOOK_INGREDIENTS_PAGE_BUFFER];
    int r, c;

    if (windowId == WINDOW_NONE)
        return;

    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));

    u8 headerFontId = FONT_SHORT_NARROW;
    u8 bodyFontId = FONT_SMALL;
    u8 headerLineHeight = GetFontAttribute(headerFontId, FONTATTR_MAX_LETTER_HEIGHT);
    u8 bodyLineHeight = GetFontAttribute(bodyFontId, FONTATTR_MAX_LETTER_HEIGHT);
    u8 headerY = RECIPEBOOK_INGREDIENTS_HEADER_Y + RECIPEBOOK_INGREDIENTS_TEXT_Y_OFFSET;
    u32 windowWidth = WindowWidthPx(windowId);
    u32 usableWidth = windowWidth;

    {
        u8 headerX = GetStringCenterAlignXOffset(headerFontId, sText_RecipeBookIngredients, windowWidth)
                   + RECIPEBOOK_INGREDIENTS_TEXT_X_OFFSET;
        AddTextPrinterParameterized4(windowId, headerFontId, headerX, headerY, 0, 0,
                                     sRecipeBookFontColorTable[RECIPEBOOK_COLOR_INGREDIENT_HEADER], TEXT_SKIP_DRAW,
                                     sText_RecipeBookIngredients);
    }

    itemId = RecipeBook_GetSelectedItemId();
    if (itemId != sRecipeBookIngredientsItemId)
    {
        sRecipeBookIngredientsItemId = itemId;
        sRecipeBookIngredientsPage = 0;
    }

    recipe = RecipeBook_GetPreviewRecipe(itemId);
    if (recipe == NULL)
    {
        sRecipeBookIngredientsRecipe = NULL;
        sRecipeBookIngredientsTotalPages = 0;
        sRecipeBookIngredientsPage = 0;
        CopyWindowToVram(windowId, COPYWIN_FULL);
        return;
    }

    if (recipe != sRecipeBookIngredientsRecipe)
    {
        sRecipeBookIngredientsRecipe = recipe;
        sRecipeBookIngredientsPage = 0;
    }

    for (r = 0; r < CRAFT_ROWS; r++)
    {
        for (c = 0; c < CRAFT_COLS; c++)
        {
            u16 ingredientId = recipe->pattern[r][c];

            if (ingredientId == ITEM_NONE)
                continue;

            for (i = 0; i < ingredientCount; i++)
            {
                if (ingredientIds[i] == ingredientId)
                {
                    ingredientCounts[i]++;
                    break;
                }
            }

            if (i == ingredientCount && ingredientCount < RECIPEBOOK_INGREDIENTS_MAX)
            {
                ingredientIds[ingredientCount] = ingredientId;
                ingredientCounts[ingredientCount] = 1;
                ingredientCount++;
            }
        }
    }

    if (ingredientCount == 0)
    {
        sRecipeBookIngredientsTotalPages = 0;
        sRecipeBookIngredientsPage = 0;
        CopyWindowToVram(windowId, COPYWIN_FULL);
        return;
    }

    u8 totalPages = (ingredientCount + RECIPEBOOK_INGREDIENTS_PER_PAGE - 1) / RECIPEBOOK_INGREDIENTS_PER_PAGE;
    u8 listStartY = headerY + headerLineHeight + RECIPEBOOK_INGREDIENTS_HEADER_TO_LIST_GAP;

    if (totalPages > 1)
    {
        u8 pageY = headerY + headerLineHeight + RECIPEBOOK_INGREDIENTS_HEADER_TO_PAGING_GAP;
        u8 pageX;
        u8 *ptr = pageText;

        listStartY = pageY + bodyLineHeight + RECIPEBOOK_INGREDIENTS_PAGING_TO_LIST_GAP;

        if (sRecipeBookIngredientsPage >= totalPages)
            sRecipeBookIngredientsPage = totalPages - 1;

        ptr = StringCopy(ptr, sText_RecipeBookLButton);
        *ptr++ = CHAR_SPACE;
        ptr = ConvertIntToDecimalStringN(ptr, sRecipeBookIngredientsPage + 1, STR_CONV_MODE_LEFT_ALIGN, 2);
        *ptr++ = CHAR_SPACE;
        *ptr++ = CHAR_SLASH;
        *ptr++ = CHAR_SPACE;
        ptr = ConvertIntToDecimalStringN(ptr, totalPages, STR_CONV_MODE_LEFT_ALIGN, 2);
        *ptr++ = CHAR_SPACE;
        ptr = StringCopy(ptr, sText_RecipeBookRButton);
        *ptr = EOS;

        pageX = GetStringCenterAlignXOffset(bodyFontId, pageText, windowWidth)
              + RECIPEBOOK_INGREDIENTS_TEXT_X_OFFSET;
        AddTextPrinterParameterized4(windowId, bodyFontId, pageX, pageY, 0, 0,
                                     sRecipeBookFontColorTable[RECIPEBOOK_COLOR_INGREDIENT_HEADER], TEXT_SKIP_DRAW,
                                     pageText);
    }

    sRecipeBookIngredientsTotalPages = totalPages;
    if (sRecipeBookIngredientsTotalPages == 0)
    {
        sRecipeBookIngredientsPage = 0;
        CopyWindowToVram(windowId, COPYWIN_FULL);
        return;
    }
    if (sRecipeBookIngredientsPage >= sRecipeBookIngredientsTotalPages)
        sRecipeBookIngredientsPage = sRecipeBookIngredientsTotalPages - 1;

    {
        u8 start = sRecipeBookIngredientsPage * RECIPEBOOK_INGREDIENTS_PER_PAGE;
        u8 end = start + RECIPEBOOK_INGREDIENTS_PER_PAGE;
        u8 x = RECIPEBOOK_INGREDIENTS_TEXT_X_OFFSET + RECIPEBOOK_INGREDIENTS_LIST_X_OFFSET;
        u8 y = listStartY;
        u8 line[RECIPEBOOK_INGREDIENTS_LINE_BUFFER];
        u8 render[RECIPEBOOK_INGREDIENTS_LINE_BUFFER];

        if (end > ingredientCount)
            end = ingredientCount;

        for (i = start; i < end; i++)
        {
            u16 required = ingredientCounts[i];
            u16 have = CountTotalItemQuantityInBag(ingredientIds[i]);
            u8 colorId = (have >= required) ? RECIPEBOOK_COLOR_INGREDIENT_HAVE : RECIPEBOOK_COLOR_INGREDIENT_MISS;
            u8 *endPtr = CopyItemName(ingredientIds[i], line);

            if (required > 1)
            {
                *endPtr++ = CHAR_SPACE;
                *endPtr++ = CHAR_x;
                endPtr = ConvertIntToDecimalStringN(endPtr, required, STR_CONV_MODE_LEFT_ALIGN, 2);
                *endPtr = EOS;
            }

        if (GetStringWidth(bodyFontId, line, 0) <= usableWidth)
            StringCopy(render, line);
        else
            RecipeBook_TruncateIngredientLine(line, render, usableWidth, bodyFontId);

        AddTextPrinterParameterized4(windowId, bodyFontId, x, y, 0, 0,
                                     sRecipeBookFontColorTable[colorId], TEXT_SKIP_DRAW, render);
        y += bodyLineHeight + RECIPEBOOK_INGREDIENTS_LINE_SPACING;
    }
    }

    CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void RecipeBook_DestroyPatternIcons(void)
{
    RecipeBook_ClearPatternIcons();
    sRecipeBookPreviewItemId = ITEM_NONE;
    sRecipeBookPreviewVariantIndex = 0;
}

static u8 RecipeBook_GetOddPocketIndicatorX(u8 pocketIndex)
{
    return RECIPEBOOK_POCKET_INDICATOR_CENTER_X - (sRecipeBookPocketCount / 2) + pocketIndex;
}

static void RecipeBook_ClearPocketIndicatorRow(void)
{
    u8 maxWidth = 18;
    s8 clearLeft = RECIPEBOOK_POCKET_INDICATOR_CENTER_X - (maxWidth / 2);
    u8 clearWidth = maxWidth;

    if (clearLeft < 0)
    {
        clearWidth = maxWidth + clearLeft;
        clearLeft = 0;
    }
    if (clearLeft + clearWidth > DISPLAY_TILE_WIDTH)
        clearWidth = DISPLAY_TILE_WIDTH - clearLeft;

    FillBgTilemapBufferRect(2, 0x01, clearLeft, RECIPEBOOK_POCKET_INDICATOR_Y, clearWidth, 1, 0);
}

static void RecipeBook_DrawPocketIndicatorSquares(void)
{
    u8 i;
    const u8 y = RECIPEBOOK_POCKET_INDICATOR_Y;

    RecipeBook_ClearPocketIndicatorRow();

    if (sRecipeBookPocketCount == 0)
    {
        CopyBgTilemapBufferToVram(2);
        return;
    }

    if (sRecipeBookPocketCount > 17)
    {
        CopyBgTilemapBufferToVram(2);
        return;
    }

    if ((sRecipeBookPocketCount & 1) != 0)
    {
        for (i = 0; i < sRecipeBookPocketCount; i++)
        {
            u16 tile = (i == sRecipeBookPocketIndex)
                ? RECIPEBOOK_INDICATOR_TILE_ACTIVE
                : RECIPEBOOK_INDICATOR_TILE_INACTIVE;
            u8 x = RecipeBook_GetOddPocketIndicatorX(i);

            FillBgTilemapBufferRect(2, tile, x, y, 1, 1, 0);
        }
    }
    else
    {
        u8 runLen = sRecipeBookPocketCount + 1;
        u8 startX = RECIPEBOOK_POCKET_INDICATOR_CENTER_X - (sRecipeBookPocketCount / 2);

        for (i = 0; i < runLen; i++)
        {
            u16 tile;

            if (i == 0)
            {
                tile = (sRecipeBookPocketIndex == 0)
                    ? RECIPEBOOK_INDICATOR_TILE_RIGHT_END_ACTIVE
                    : BG_TILE_H_FLIP(RECIPEBOOK_INDICATOR_TILE_LEFT_END_INACTIVE);
            }
            else if (i == runLen - 1)
            {
                tile = (sRecipeBookPocketIndex == sRecipeBookPocketCount - 1)
                    ? BG_TILE_H_FLIP(RECIPEBOOK_INDICATOR_TILE_RIGHT_END_ACTIVE)
                    : RECIPEBOOK_INDICATOR_TILE_LEFT_END_INACTIVE;
            }
            else
            {
                u8 leftPocket = i - 1;
                u8 rightPocket = i;

                if (sRecipeBookPocketIndex == leftPocket)
                    tile = RECIPEBOOK_INDICATOR_TILE_LINK_LEFT_ACTIVE;
                else if (sRecipeBookPocketIndex == rightPocket)
                    tile = BG_TILE_H_FLIP(RECIPEBOOK_INDICATOR_TILE_LINK_LEFT_ACTIVE);
                else
                    tile = RECIPEBOOK_INDICATOR_TILE_LINK_INACTIVE;
            }

            FillBgTilemapBufferRect(2, tile, startX + i, y, 1, 1, 0);
        }
    }

    CopyBgTilemapBufferToVram(2);
}

static void RecipeBook_CreatePocketSwitchArrows(void)
{
    if (sRecipeBookPocketArrowTask == TASK_NONE && sRecipeBookPocketCount > 1)
        sRecipeBookPocketArrowTask = AddScrollIndicatorArrowPair(&sRecipeBookPocketArrowTemplate, &sRecipeBookPocketArrowPos);
}

static void RecipeBook_DestroyPocketSwitchArrows(void)
{
    if (sRecipeBookPocketArrowTask != TASK_NONE)
    {
        RemoveScrollIndicatorArrowPair(sRecipeBookPocketArrowTask);
        sRecipeBookPocketArrowTask = TASK_NONE;
    }
}

static void RecipeBook_DestroyListScrollArrows(void)
{
    if (sRecipeBookListArrowTask != TASK_NONE)
    {
        RemoveScrollIndicatorArrowPair(sRecipeBookListArrowTask);
        sRecipeBookListArrowTask = TASK_NONE;
    }
}

static void RecipeBook_UpdateListScrollArrows(void)
{
    s32 maxScroll = sRecipeBookItemCount - sRecipeBookListMenuTemplate.maxShowed;
    struct ScrollArrowsTemplate template;

    if (maxScroll <= 0)
    {
        RecipeBook_DestroyListScrollArrows();
        return;
    }

    if (sRecipeBookListArrowTask != TASK_NONE)
        RecipeBook_DestroyListScrollArrows();

    template = sRecipeBookListScrollArrowTemplate;
    template.fullyUpThreshold = 0;
    template.fullyDownThreshold = maxScroll;
    sRecipeBookListArrowTask = AddScrollIndicatorArrowPair(&template, &sRecipeBookListScroll);
}

static void RecipeBook_DisableTimeOfDayBlend(void)
{
    if (sRecipeBookTimeBlendOverridden)
        return;

    gTimeBlend.startBlend = gTimeOfDayBlend[TIME_DAY];
    gTimeBlend.endBlend = gTimeOfDayBlend[TIME_DAY];
    gTimeBlend.weight = 256;
    gTimeBlend.altWeight = 256;
    sRecipeBookTimeBlendOverridden = TRUE;
}

static void RecipeBook_RestoreTimeOfDayBlend(void)
{
    if (!sRecipeBookTimeBlendOverridden)
        return;

    sRecipeBookTimeBlendOverridden = FALSE;
    UpdateTimeOfDay();
    ApplyWeatherColorMapIfIdle(gWeatherPtr->colorMapIndex);
}

static void RecipeBook_Init(void)
{
    u8 i;

    for (i = 0; i < RECIPEBOOK_WIN_COUNT; i++)
        sRecipeBookWindowIds[i] = WINDOW_NONE;
    for (i = 0; i < CRAFT_SLOT_COUNT; i++)
        sRecipeBookPatternSpriteIds[i] = SPRITE_NONE;
    sRecipeBookPocketIndex = 0;
    sRecipeBookListScroll = 0;
    sRecipeBookListCursor = 0;
    sRecipeBookListTaskId = TASK_NONE;
    sRecipeBookPocketArrowTask = TASK_NONE;
    sRecipeBookPocketArrowPos = 0;
    sRecipeBookListArrowTask = TASK_NONE;
    sRecipeBookPreviewItemId = ITEM_NONE;
    sRecipeBookPreviewVariantIndex = 0;
    sRecipeBookPendingItemId = ITEM_NONE;
    sRecipeBookPendingVariantIndex = 0;
    sRecipeBookPatternDelay = 0;
    sRecipeBookIngredientsItemId = ITEM_NONE;
    sRecipeBookIngredientsPage = 0;
    sRecipeBookIngredientsTotalPages = 0;
    sRecipeBookIngredientsRecipe = NULL;
    sRecipeBookVariantModeEnabled = FALSE;
    memset(sRecipeBookVariantIndex, 0, sizeof(sRecipeBookVariantIndex));
    sRecipeBookTimeBlendOverridden = FALSE;
    RecipeBook_DisableTimeOfDayBlend();

    RecipeBook_BuildPocketList();
    RecipeBook_InitWindows();
    if (sRecipeBookPocketCount == 0)
    {
        RecipeBook_PrintPocketNames(sText_RecipeBookNoRecipes, NULL);
    }
    else
    {
        RecipeBook_PrintPocketNames(gPocketNamesStringsTable[sRecipeBookPockets[sRecipeBookPocketIndex]], NULL);
    }
    RecipeBook_CopyPocketNameToWindow(0);
    RecipeBook_RefreshListMenu();
    RecipeBook_UpdatePatternPreview();
    RecipeBook_UpdateIngredientsWindow();
    RecipeBook_UpdateListScrollArrows();
    RecipeBook_DrawPocketIndicatorSquares();
    RecipeBook_CreatePocketSwitchArrows();
}

static void RecipeBook_Cleanup(void)
{
    RecipeBook_DestroyWindows();
    RecipeBook_DestroyPocketSwitchArrows();
    RecipeBook_DestroyListScrollArrows();
    RecipeBook_DestroyPatternIcons();
    RecipeBook_RestoreTimeOfDayBlend();
    UnsetBgTilemapBuffer(2);
}

static void CraftMenu_ReshowAfterBagMenu(void)
{
    sKeepSlots = TRUE;
    StartCraftMenu();
    FadeInFromBlack();
}

static void Task_RecipeBookMenu(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
#define tPocketSwitchState data[1]
#define tPocketSwitchTimer data[2]
#define tPocketSwitchDir data[3]
#define tPocketSwitchTarget data[4]

    switch (gTasks[taskId].data[0])
    {
    case 0:
        ShowBg(0);
        ShowBg(2);
        PlaySE(SE_DEX_PAGE);
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
        gTasks[taskId].data[0]++;
        break;
    case 1:
        if (!gPaletteFade.active)
            gTasks[taskId].data[0]++;
        break;
    case 2:
        if (JOY_NEW(B_BUTTON))
        {
            if (sRecipeBookVariantModeEnabled)
            {
                PlaySE(SE_SELECT);
                sRecipeBookVariantModeEnabled = FALSE;
                RecipeBook_CreatePocketSwitchArrows();
                RecipeBook_PrintPocketNames(gPocketNamesStringsTable[sRecipeBookPockets[sRecipeBookPocketIndex]], NULL);
                RecipeBook_CopyPocketNameToWindow(0);
                RecipeBook_RedrawVisibleListRows();
            }
            else
            {
                PlaySE(SE_SELECT);
                RecipeBook_SavePocketListState();
                BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
                gTasks[taskId].data[0]++;
            }
        }
        else
        {
            if (tPocketSwitchState != 0)
            {
                tPocketSwitchTimer++;
                if (!(tPocketSwitchTimer & 1))
                {
                    if (tPocketSwitchDir == MENU_CURSOR_DELTA_RIGHT)
                        RecipeBook_CopyPocketNameToWindow(tPocketSwitchTimer >> 1);
                    else
                        RecipeBook_CopyPocketNameToWindow(8 - (tPocketSwitchTimer >> 1));
                }
                if (tPocketSwitchTimer >= 16)
                {
                    sRecipeBookPocketIndex = tPocketSwitchTarget;
                    RecipeBook_RefreshListMenu();
                    RecipeBook_DrawPocketIndicatorSquares();
                    if (sRecipeBookVariantModeEnabled
                        && RecipeBook_GetUnlockedVariantCount(RecipeBook_GetSelectedItemId()) <= 1)
                    {
                        sRecipeBookVariantModeEnabled = FALSE;
                        RecipeBook_CreatePocketSwitchArrows();
                        RecipeBook_RedrawVisibleListRows();
                    }
                    sRecipeBookIngredientsPage = 0;
                    RecipeBook_SchedulePatternPreview();
                    RecipeBook_UpdateIngredientsWindow();
                    tPocketSwitchState = 0;
                }
            }
            else
            {
                const struct CraftRecipe *recipe;
                u8 ingredientCount;
                u8 totalPages;
                u16 currentItemId = RecipeBook_GetSelectedItemId();

                if (JOY_NEW(SELECT_BUTTON))
                {
                    u8 variantCount = RecipeBook_GetUnlockedVariantCount(currentItemId);

                    if (variantCount > 1)
                    {
                        PlaySE(SE_SELECT);
                        sRecipeBookVariantModeEnabled = !sRecipeBookVariantModeEnabled;
                        sRecipeBookIngredientsPage = 0;
                        RecipeBook_SchedulePatternPreview();
                        RecipeBook_UpdateIngredientsWindow();
                        RecipeBook_RedrawVisibleListRows();
                        RecipeBook_PrintPocketNames(gPocketNamesStringsTable[sRecipeBookPockets[sRecipeBookPocketIndex]], NULL);
                        RecipeBook_CopyPocketNameToWindow(0);

                        if (sRecipeBookVariantModeEnabled)
                            RecipeBook_DestroyPocketSwitchArrows();
                        else
                            RecipeBook_CreatePocketSwitchArrows();
                    }
                    else
                    {
                        PlaySE(SE_FAILURE);
                    }
                }

                if (sRecipeBookVariantModeEnabled)
                {
                    u8 variantCount = RecipeBook_GetUnlockedVariantCount(currentItemId);

                    if (variantCount > 1)
                    {
                        if (JOY_NEW(DPAD_LEFT))
                        {
                            PlaySE(SE_SELECT);
                            sRecipeBookVariantIndex[currentItemId] =
                                (sRecipeBookVariantIndex[currentItemId] + variantCount - 1) % variantCount;
                            sRecipeBookIngredientsPage = 0;
                            RecipeBook_SchedulePatternPreview();
                            RecipeBook_UpdateIngredientsWindow();
                            RecipeBook_RedrawListRow(sRecipeBookListScroll + sRecipeBookListCursor,
                                                     sRecipeBookListCursor);
                        }
                        else if (JOY_NEW(DPAD_RIGHT))
                        {
                            PlaySE(SE_SELECT);
                            sRecipeBookVariantIndex[currentItemId] =
                                (sRecipeBookVariantIndex[currentItemId] + 1) % variantCount;
                            sRecipeBookIngredientsPage = 0;
                            RecipeBook_SchedulePatternPreview();
                            RecipeBook_UpdateIngredientsWindow();
                            RecipeBook_RedrawListRow(sRecipeBookListScroll + sRecipeBookListCursor,
                                                     sRecipeBookListCursor);
                        }
                    }
                    else
                    {
                        sRecipeBookVariantModeEnabled = FALSE;
                        RecipeBook_CreatePocketSwitchArrows();
                        RecipeBook_PrintPocketNames(gPocketNamesStringsTable[sRecipeBookPockets[sRecipeBookPocketIndex]], NULL);
                        RecipeBook_CopyPocketNameToWindow(0);
                        RecipeBook_RedrawVisibleListRows();
                    }
                }
                else if (sRecipeBookPocketCount > 1)
                {
                    s8 direction = 0;

                    if (JOY_NEW(DPAD_LEFT))
                        direction = MENU_CURSOR_DELTA_LEFT;
                    else if (JOY_NEW(DPAD_RIGHT))
                        direction = MENU_CURSOR_DELTA_RIGHT;

                    if (direction != 0)
                    {
                        u8 newIndex = sRecipeBookPocketIndex;

                        PlaySE(SE_SELECT);
                        RecipeBook_SavePocketListState();
                        if (direction == MENU_CURSOR_DELTA_RIGHT)
                            newIndex = (newIndex + 1) % sRecipeBookPocketCount;
                        else
                            newIndex = (newIndex == 0) ? (sRecipeBookPocketCount - 1) : (newIndex - 1);

                        tPocketSwitchState = 1;
                        tPocketSwitchTimer = 0;
                        tPocketSwitchDir = direction;
                        tPocketSwitchTarget = newIndex;

                        if (direction == MENU_CURSOR_DELTA_RIGHT)
                            RecipeBook_PrintPocketNames(gPocketNamesStringsTable[sRecipeBookPockets[sRecipeBookPocketIndex]],
                                                        gPocketNamesStringsTable[sRecipeBookPockets[newIndex]]);
                        else
                            RecipeBook_PrintPocketNames(gPocketNamesStringsTable[sRecipeBookPockets[newIndex]],
                                                        gPocketNamesStringsTable[sRecipeBookPockets[sRecipeBookPocketIndex]]);

                        RecipeBook_CopyPocketNameToWindow(direction == MENU_CURSOR_DELTA_RIGHT ? 0 : 8);
                    }
                }

                recipe = RecipeBook_GetPreviewRecipe(currentItemId);
                ingredientCount = RecipeBook_GetUniqueIngredientCount(recipe);
                totalPages = (ingredientCount + RECIPEBOOK_INGREDIENTS_PER_PAGE - 1) / RECIPEBOOK_INGREDIENTS_PER_PAGE;
                if (totalPages > 1)
                {
                    if (JOY_NEW(L_BUTTON))
                    {
                        PlaySE(SE_SELECT);
                        sRecipeBookIngredientsPage = (sRecipeBookIngredientsPage + totalPages - 1) % totalPages;
                        RecipeBook_UpdateIngredientsWindow();
                    }
                    else if (JOY_NEW(R_BUTTON))
                    {
                        PlaySE(SE_SELECT);
                        sRecipeBookIngredientsPage = (sRecipeBookIngredientsPage + 1) % totalPages;
                        RecipeBook_UpdateIngredientsWindow();
                    }
                }
            }

            if (tPocketSwitchState == 0 && sRecipeBookListTaskId != TASK_NONE)
            {
                u16 prevItemId = RecipeBook_GetSelectedItemId();
                s32 listPosition = ListMenu_ProcessInput(sRecipeBookListTaskId);
                ListMenuGetScrollAndRow(sRecipeBookListTaskId, &sRecipeBookListScroll, &sRecipeBookListCursor);
                RecipeBook_SavePocketListState();
                    if (prevItemId != RecipeBook_GetSelectedItemId())
                    {
                        u16 newItemId = RecipeBook_GetSelectedItemId();
                        u8 variantCount = RecipeBook_GetUnlockedVariantCount(newItemId);

                        sRecipeBookIngredientsPage = 0;
                        if (sRecipeBookVariantModeEnabled && variantCount <= 1)
                        {
                            sRecipeBookVariantModeEnabled = FALSE;
                            RecipeBook_CreatePocketSwitchArrows();
                            RecipeBook_PrintPocketNames(gPocketNamesStringsTable[sRecipeBookPockets[sRecipeBookPocketIndex]], NULL);
                            RecipeBook_CopyPocketNameToWindow(0);
                            RecipeBook_RedrawVisibleListRows();
                        }
                        else if (variantCount > 1)
                        {
                            RecipeBook_GetVariantIndex(newItemId);
                        }
                    }

                RecipeBook_SchedulePatternPreview();
                RecipeBook_UpdateIngredientsWindow();

                if (listPosition == LIST_CANCEL)
                {
                    PlaySE(SE_SELECT);
                    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
                    gTasks[taskId].data[0]++;
                }
            }

            if (tPocketSwitchState == 0)
                RecipeBook_TickPatternPreview();
        }
        break;
    case 3:
        if (!gPaletteFade.active)
        {
            RecipeBook_Cleanup();
            DestroyTask(taskId);
            SetMainCallback2(CB2_ReturnToCraftMenu);
        }
        break;
    }

#undef tPocketSwitchState
#undef tPocketSwitchTimer
#undef tPocketSwitchDir
#undef tPocketSwitchTarget
}

static void Task_WaitFadeAndOpenBag(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        gCraftActiveSlot = CraftMenuUI_GetCursorPos();
        CraftMenuUI_Close();
        SetBagPreOpenCallback(BagPreOpen_SetCursorItem);
        SetMainCallback2(CB2_BagMenuFromCraftMenu);
        DestroyTask(taskId);
    }
}

static void Task_WaitFadeAndOpenRecipeBook(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        gCraftActiveSlot = CraftMenuUI_GetCursorPos();
        CraftMenuUI_Close();
        SetMainCallback2(CB2_CraftRecipeBookMenu);
        DestroyTask(taskId);
    }
}

static void Task_WaitFadeAndOpenDebugMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        gCraftActiveSlot = CraftMenuUI_GetCursorPos();
        CraftMenuUI_Close();
        SetMainCallback2(CB2_CraftDebugMenu);
        DestroyTask(taskId);
    }
}

static void OpenBagFromCraftMenu(void)
{
    FadeScreen(FADE_TO_BLACK, 0);
    CreateTask(Task_WaitFadeAndOpenBag, 0);
}

static void OpenRecipeBookFromCraftMenu(void)
{
    FadeScreen(FADE_TO_BLACK, 0);
    CreateTask(Task_WaitFadeAndOpenRecipeBook, 0);
}

//---------------------------------------------------------------------------
// Input handlers
//---------------------------------------------------------------------------

static bool8 HandleCraftMenuInput(void)
{
    if (gPaletteFade.active)
        return FALSE;
    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        if (CraftMenu_HasItemsOnTable())
        {
            gMenuCallback = NULL;
            CreateTask(Task_PackUpAsk, 0);
            return FALSE;
        }
        else
        {
            CloseCraftMenu();
            return TRUE;
        }
    }

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        {
            int row = CRAFT_SLOT_ROW(CraftMenuUI_GetCursorPos());
            int col = CRAFT_SLOT_COL(CraftMenuUI_GetCursorPos());
            if (gCraftSlots[row][col].itemId != ITEM_NONE)
            {
                CraftMenuUI_ShowActionMenu();
                gMenuCallback = HandleSlotActionInput;
                return FALSE;
            }
            else
            {
                OpenBagFromCraftMenu();
                return TRUE;
            }
        }
    }

    if (JOY_NEW(SELECT_BUTTON))
    {
        PlaySE(SE_SELECT);
        OpenRecipeBookFromCraftMenu();
        return TRUE;
    }

#if DEBUG_CRAFT_MENU
    if (JOY_NEW(R_BUTTON))
    {
        PlaySE(SE_SELECT);
        FadeScreen(FADE_TO_BLACK, 0);
        CreateTask(Task_WaitFadeAndOpenDebugMenu, 0);
        return TRUE;
    }
#endif

    if (JOY_NEW(START_BUTTON))
    {
        gMenuCallback = NULL;
        if (!CraftMenu_HasItemsOnTable())
        {
            PlaySE(SE_SELECT);
            CreateTask(Task_NoCraftItems, 0);
            return FALSE;
        }
        else
        {
            u16 itemId;
            const struct CraftRecipe *recipe = CraftLogic_GetMatchingRecipe(gCraftRecipes, gCraftRecipeCount, &itemId);
            if (recipe == NULL)
            {
                PlaySE(SE_FAILURE);
                CreateTask(Task_InvalidCraft, 0);
                return FALSE;
            }
            else
            {
                PlaySE(SE_SELECT);
                u16 qty = CraftLogic_GetCraftableQuantity(recipe);
                u8 taskId = CreateTask(Task_CraftAsk, 0);
                gTasks[taskId].data[0] = itemId;
                gTasks[taskId].data[1] = qty;
                return FALSE;
            }
        }
    }

    if (CraftMenuUI_HandleDpadInput())
    {
        PlaySE(SE_SELECT);
        CraftMenuUI_UpdateGrid();
    }

    return FALSE;
}

static void CloseCraftMenu(void)
{
    CraftMenuUI_Close();
    ScriptUnfreezeObjectEvents();
    UnlockPlayerFieldControls();
    ScriptContext_Enable();
}

static bool8 CraftMenu_HasItemsOnTable(void)
{
    int row, col;
    for (row = 0; row < CRAFT_ROWS; row++)
    {
        for (col = 0; col < CRAFT_COLS; col++)
        {
            if (gCraftSlots[row][col].itemId != ITEM_NONE)
                return TRUE;
        }
    }
    return FALSE;
}

static void PackUpYes(u8 taskId)
{
    int row, col;

    for (row = 0; row < CRAFT_ROWS; row++)
    {
        for (col = 0; col < CRAFT_COLS; col++)
        {
            struct ItemSlot *slot = &gCraftSlots[row][col];
            if (slot->itemId != ITEM_NONE)
            {
                AddBagItem(slot->itemId, slot->quantity);
                slot->itemId = ITEM_NONE;
                slot->quantity = 0;
            }
        }
    }

    CraftMenuUI_ClearPackUpMessage();
    CloseCraftMenu();

    u8 craftTask = FindTaskIdByFunc(Task_RunCraftMenu);
    if (craftTask != TASK_NONE)
        DestroyTask(craftTask);

    DestroyTask(taskId);
}

static void PackUpNo(u8 taskId)
{
    CraftMenuUI_ClearPackUpMessage();
    gMenuCallback = HandleCraftMenuInput;
    DestroyTask(taskId);
}

//---------------------------------------------------------------------------
// Slot and swap actions
//---------------------------------------------------------------------------

static void Action_SwapItem(void)
{
    OpenBagFromCraftMenu();
}

static void Action_RemoveItem(void)
{
    u8 slot = CraftMenuUI_GetCursorPos();
    struct ItemSlot *s = &gCraftSlots[CRAFT_SLOT_ROW(slot)][CRAFT_SLOT_COL(slot)];
    AddBagItem(s->itemId, s->quantity);
    s->itemId = ITEM_NONE;
    s->quantity = 0;
    CraftMenuUI_DrawIcons();
}

static void Action_StartSwapSlot(void)
{
    sSwapFromSlot = CraftMenuUI_GetCursorPos();
    CraftMenuUI_StartSwapMode();
    gMenuCallback = HandleSwapSlotInput;
}

static void Action_AdjustQuantity(void)
{
    CreateTask(Task_AdjustQuantity_Start, 0);
}

static bool8 HandleSlotActionInput(void)
{
    s8 selection = CraftMenuUI_ProcessActionMenuInput();
    if (selection == MENU_NOTHING_CHOSEN)
        return FALSE;


    switch (selection)
    {
    case SLOT_ACTION_SWAP_ITEM:
        CraftMenuUI_HideActionMenu();
        Action_SwapItem();
        return TRUE;
    case SLOT_ACTION_ADJUST_QTY:
        CraftMenuUI_HideActionMenu();
        Action_AdjustQuantity();
        gMenuCallback = NULL;
        return FALSE;
    case SLOT_ACTION_SWAP_SLOT:
        CraftMenuUI_HideActionMenu();
        Action_StartSwapSlot();
        return FALSE;
    case SLOT_ACTION_REMOVE_ITEM:
        CraftMenuUI_HideActionMenu();
        CraftMenuUI_RedrawInfo();
        Action_RemoveItem();
        break;
    case SLOT_ACTION_CANCEL:
    default:
        CraftMenuUI_HideActionMenu();
        CraftMenuUI_RedrawInfo();
        break;
    }

    gMenuCallback = HandleCraftMenuInput;
    return FALSE;
}

static bool8 HandleSwapSlotInput(void)
{
    if (CraftMenuUI_HandleDpadInput())
    {
        PlaySE(SE_SELECT);
        CraftMenuUI_UpdateGrid();
    }

    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        CraftMenuUI_EndSwapMode();
        CraftMenuUI_SetCursorPos(sSwapFromSlot);
        gMenuCallback = HandleCraftMenuInput;
        return FALSE;
    }

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        u8 to = CraftMenuUI_GetCursorPos();
        if (to != sSwapFromSlot)
            CraftLogic_SwapSlots(sSwapFromSlot, to);
        CraftMenuUI_EndSwapMode();
        CraftMenuUI_DrawIcons();
        gMenuCallback = HandleCraftMenuInput;
    }

    return FALSE;
}

static u16 sAdjustOldQty;
static u16 sAdjustMaxQty;
static u16 sAdjustItemId;

//---------------------------------------------------------------------------
// Quantity adjustment tasks
//---------------------------------------------------------------------------

static void Task_AdjustQuantity_Start(u8 taskId)
{
    int row = CRAFT_SLOT_ROW(CraftMenuUI_GetCursorPos());
    int col = CRAFT_SLOT_COL(CraftMenuUI_GetCursorPos());

    sAdjustItemId = gCraftSlots[row][col].itemId;
    sAdjustOldQty = gCraftSlots[row][col].quantity;
    sAdjustMaxQty = sAdjustOldQty + CountTotalItemQuantityInBag(sAdjustItemId);

    gTasks[taskId].data[0] = sAdjustOldQty;
    gMenuCallback = NULL;

    CraftMenuUI_DisplayAdjustQtyMessage(taskId, sAdjustItemId, Task_AdjustQuantity_Init);
}

static void Task_AdjustQuantity_Init(u8 taskId)
{
    CraftMenuUI_AddQuantityWindow();
    CraftMenuUI_PrintQuantity(gTasks[taskId].data[0]);
    gTasks[taskId].func = Task_AdjustQuantity_HandleInput;
}

static void Task_AdjustQuantity_HandleInput(u8 taskId)
{
    if (AdjustQuantityAccordingToDPadInput(&gTasks[taskId].data[0], sAdjustMaxQty))
        CraftMenuUI_PrintQuantity(gTasks[taskId].data[0]);

    if (JOY_NEW(A_BUTTON))
    {
        u16 newQty = gTasks[taskId].data[0];
        int row = CRAFT_SLOT_ROW(CraftMenuUI_GetCursorPos());
        int col = CRAFT_SLOT_COL(CraftMenuUI_GetCursorPos());

        if (newQty > sAdjustOldQty)
            RemoveBagItem(sAdjustItemId, newQty - sAdjustOldQty);
        else if (newQty < sAdjustOldQty)
            AddBagItem(sAdjustItemId, sAdjustOldQty - newQty);

        gCraftSlots[row][col].quantity = newQty;
        CraftMenuUI_RemoveQuantityWindow();
        CraftMenuUI_DrawIcons();
        CraftMenuUI_ClearAdjustQtyMessage();
        gMenuCallback = HandleCraftMenuInput;
        DestroyTask(taskId);
    }
    else if (JOY_NEW(B_BUTTON))
    {
        CraftMenuUI_RemoveQuantityWindow();
        CraftMenuUI_ClearAdjustQtyMessage();
        gMenuCallback = HandleCraftMenuInput;
        DestroyTask(taskId);
    }
}

static const struct YesNoFuncTable sPackUpYesNoFuncs = {
    .yesFunc = PackUpYes,
    .noFunc = PackUpNo,
};

//---------------------------------------------------------------------------
// Pack-up and confirmation tasks
//---------------------------------------------------------------------------

static void Task_ShowPackUpYesNo(u8 taskId)
{
    CraftMenuUI_ShowPackUpYesNo();
    DoYesNoFuncWithChoice(taskId, &sPackUpYesNoFuncs);
}

static void Task_PackUpAsk(u8 taskId)
{
    CraftMenuUI_DisplayPackUpMessage(taskId, Task_ShowPackUpYesNo);
}


static void Task_WaitForCraftMessageAck(u8 taskId)
{
    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        CraftMenuUI_ClearMessage();
        gMenuCallback = HandleCraftMenuInput;
        DestroyTask(taskId);
    }
}

static void Task_WaitForCraftResultAck(u8 taskId)
{
    if (!IsFanfareTaskInactive())
        return;

    if (gTasks[taskId].data[0] < 30)
    {
        gTasks[taskId].data[0]++;
        return;
    }

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        if (gTasks[taskId].data[1])
        {
            gTasks[taskId].data[1] = 0;
            SetCraftExcessVars(gTasks[taskId].data[2]);
            CraftMenuUI_DisplayMessage(taskId, sText_CraftExcess, Task_WaitForCraftMessageAck);
        }
        else
        {
            CraftMenuUI_ClearMessage();
            gMenuCallback = HandleCraftMenuInput;
            DestroyTask(taskId);
        }
    }
}

//---------------------------------------------------------------------------
// Crafting confirmation flow
//---------------------------------------------------------------------------

static void Task_NoCraftItems(u8 taskId)
{
    CraftMenuUI_DisplayNoItemsMessage(taskId, Task_WaitForCraftMessageAck);
}

static void Task_InvalidCraft(u8 taskId)
{
    CraftMenuUI_DisplayInvalidMessage(taskId, Task_WaitForCraftMessageAck);
}

static void CraftYes(u8 taskId)
{
    struct ItemSlot slotsBefore[CRAFT_ROWS][CRAFT_COLS];
    u16 crafted;
    u16 resultItemId = gTasks[taskId].data[0];
    bool8 hasRemaining;
    u16 usedPerIngredient;

    memcpy(slotsBefore, gCraftSlots, sizeof(slotsBefore));
    crafted = CraftLogic_Craft(gCraftRecipes, gCraftRecipeCount);
    if (crafted)
    {
        PlayFanfare(MUS_OBTAIN_ITEM);
        CraftMenuUI_DrawIcons();
    }
    usedPerIngredient = GetUsedPerIngredient(slotsBefore, &hasRemaining);
    SetCraftSuccessVars(resultItemId, crafted);
    gMenuCallback = NULL;
    gTasks[taskId].data[0] = 0;
    gTasks[taskId].data[1] = hasRemaining;
    gTasks[taskId].data[2] = usedPerIngredient;
    CraftMenuUI_DisplayMessage(taskId, sText_CraftSuccess, Task_WaitForCraftResultAck);
}

static void CraftNo(u8 taskId)
{
    CraftMenuUI_ClearMessage();
    gMenuCallback = HandleCraftMenuInput;
    DestroyTask(taskId);
}

static const struct YesNoFuncTable sCraftYesNoFuncs = {
    .yesFunc = CraftYes,
    .noFunc = CraftNo,
};

static void Task_ShowCraftYesNo(u8 taskId)
{
    CraftMenuUI_ShowYesNo();
    DoYesNoFuncWithChoice(taskId, &sCraftYesNoFuncs);
}

static void Task_CraftAsk(u8 taskId)
{
    u16 itemId = gTasks[taskId].data[0];
    u16 qty = gTasks[taskId].data[1];
    CraftMenuUI_DisplayCraftConfirmMessage(taskId, itemId, qty, Task_ShowCraftYesNo);
}
