#include "global.h"
#include "bg.h"
#include "gpu_regs.h"
#include "palette.h"
#include "graphics.h"
#include "window.h"
#include "text_window.h"
#include "text.h"
#include "menu.h"
#include "main.h"
#include "sprite.h"
#include "decompress.h"
#include "strings.h"
#include "string_util.h"
#include "item_icon.h"
#include "item_menu.h"
#include "craft_menu.h"
#include "constants/songs.h"
#include "constants/items.h"
#include "craft_logic.h"
#include "craft_menu_ui.h"

#define TAG_WB_TOPLEFT     0x4001
#define TAG_WB_TOPMID      0x4002
#define TAG_WB_TOPRIGHT    0x4003
#define TAG_WB_MIDLEFT     0x4004
#define TAG_WB_MIDMID      0x4005
#define TAG_WB_MIDRIGHT    0x4006
#define TAG_WB_BOTLEFT     0x4007
#define TAG_WB_BOTMID      0x4008
#define TAG_WB_BOTRIGHT    0x4009
#define TAG_WB_PALETTE     0x4010
#define TAG_CRAFT_ICON_BASE 0x5200

#define WB_CENTER_X 120
#define WB_CENTER_Y 60

enum
{
    WINDOW_CRAFT_GRID,
    WINDOW_CRAFT_INFO,
    WINDOW_CRAFT_YESNO,
    NUM_CRAFT_WINDOWS
};

EWRAM_DATA static u8 sCraftGridWindowId = 0;
EWRAM_DATA static u8 sCraftInfoWindowId = 0;
EWRAM_DATA static u8 sPackUpMessageWindowId = 0;
EWRAM_DATA static u8 sWorkbenchSpriteIds[CRAFT_SLOT_COUNT];
EWRAM_DATA static u8 sCraftCursorPos = 0;
EWRAM_DATA static u8 sCraftSlotSpriteIds[CRAFT_SLOT_COUNT];

static const u8 sText_CraftingUi_AButton[] = _("{A_BUTTON}");
static const u8 sText_CraftingUi_AddItem[] = _("Add\nItem");
static const u8 sText_CraftingUi_BButtonExit[] = _("{B_BUTTON} Exit");
static const u8 sText_CraftingUi_StartButtonCraft[] = _("{START_BUTTON} Craft");
static const u8 sText_CraftingUi_SelectButton[] = _("{SELECT_BUTTON}");
static const u8 sText_CraftingUi_RecipeBook[] = _("Recipe\nBook");
const u8 gText_PackUpQuestion[] = _("Would you like to pack up?");

static const u8 sGridTextColor[] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_LIGHT_GRAY, TEXT_COLOR_DARK_GRAY};
static const u8 sInputTextColor[] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};
static const u8 sHintTextColor[] = {TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_LIGHT_GREEN};

struct GridSlotPos
{
    u8 x;
    u8 y;
};

static const struct GridSlotPos sWorkbenchSlotPositions[CRAFT_SLOT_COUNT] =
{
    {18,  1},  {45,  1},  {72,  1},
    {18, 28},  {45, 28},  {72, 28},
    {18, 55},  {45, 55},  {72, 55},
};

static const struct WindowTemplate sCraftWindowTemplates[NUM_CRAFT_WINDOWS] =
{
    [WINDOW_CRAFT_GRID] = {
        .bg = 0,
        .tilemapLeft = 9,
        .tilemapTop = 3,
        .width = 11,
        .height = 10,
        .paletteNum = 15,
        .baseBlock = 1
    },
    [WINDOW_CRAFT_INFO] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 26,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 145
    },
    [WINDOW_CRAFT_YESNO] = {
        .bg = 0,
        .tilemapLeft = 23,
        .tilemapTop = 9,
        .width = 5,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 300,
    }
};

static const struct CompressedSpriteSheet sWorkbenchSheets[] =
{
    { gCraftWorkbench_TopLeft_Gfx,  0x400, TAG_WB_TOPLEFT },
    { gCraftWorkbench_TopMid_Gfx,   0x400, TAG_WB_TOPMID },
    { gCraftWorkbench_TopRight_Gfx, 0x400, TAG_WB_TOPRIGHT },
    { gCraftWorkbench_MidLeft_Gfx,  0x400, TAG_WB_MIDLEFT },
    { gCraftWorkbench_MidMid_Gfx,   0x400, TAG_WB_MIDMID },
    { gCraftWorkbench_MidRight_Gfx, 0x400, TAG_WB_MIDRIGHT },
    { gCraftWorkbench_BotLeft_Gfx,  0x400, TAG_WB_BOTLEFT },
    { gCraftWorkbench_BotMid_Gfx,   0x400, TAG_WB_BOTMID },
    { gCraftWorkbench_BotRight_Gfx, 0x400, TAG_WB_BOTRIGHT },
};

static const struct SpritePalette sWorkbenchPalette = { gCraftWorkbench_Pal, TAG_WB_PALETTE };

static const struct OamData sOam_Workbench32 =
{
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 1,
};

#define WB_TEMPLATE(tag) {            \
    .tileTag = tag,                   \
    .paletteTag = TAG_WB_PALETTE,     \
    .oam = &sOam_Workbench32,         \
    .anims = gDummySpriteAnimTable,   \
    .images = NULL,                   \
    .affineAnims = gDummySpriteAffineAnimTable, \
    .callback = SpriteCallbackDummy   \
}

static const struct SpriteTemplate sWorkbenchTemplates[CRAFT_SLOT_COUNT] =
{
    WB_TEMPLATE(TAG_WB_TOPLEFT),
    WB_TEMPLATE(TAG_WB_TOPMID),
    WB_TEMPLATE(TAG_WB_TOPRIGHT),
    WB_TEMPLATE(TAG_WB_MIDLEFT),
    WB_TEMPLATE(TAG_WB_MIDMID),
    WB_TEMPLATE(TAG_WB_MIDRIGHT),
    WB_TEMPLATE(TAG_WB_BOTLEFT),
    WB_TEMPLATE(TAG_WB_BOTMID),
    WB_TEMPLATE(TAG_WB_BOTRIGHT),
};

static void LoadCraftWindows(void)
{
    sCraftGridWindowId = AddWindow(&sCraftWindowTemplates[WINDOW_CRAFT_GRID]);
    sCraftInfoWindowId = AddWindow(&sCraftWindowTemplates[WINDOW_CRAFT_INFO]);
}

static void ShowGridWindow(void)
{
    FillWindowPixelBuffer(sCraftGridWindowId, PIXEL_FILL(0));
    PutWindowTilemap(sCraftGridWindowId);
    CopyWindowToVram(sCraftGridWindowId, COPYWIN_FULL);
}

static void ShowInfoWindow(void)
{
    DrawStdFrameWithCustomTileAndPalette(sCraftInfoWindowId, TRUE, 0x214, 14);
    FillWindowPixelBuffer(sCraftInfoWindowId, PIXEL_FILL(1));
    CopyWindowToVram(sCraftInfoWindowId, COPYWIN_FULL);
}

static void UpdateCraftInfoWindow(void)
{
    AddTextPrinterParameterized3(sCraftInfoWindowId, FONT_NORMAL, 2, 7, sInputTextColor, 0, sText_CraftingUi_AButton);
    AddTextPrinterParameterized3(sCraftInfoWindowId, FONT_SMALL, 13, 2, sInputTextColor, 0, sText_CraftingUi_AddItem);
    AddTextPrinterParameterized3(sCraftInfoWindowId, FONT_NORMAL, 45, 7, sInputTextColor, 0, sText_CraftingUi_BButtonExit);
    AddTextPrinterParameterized3(sCraftInfoWindowId, FONT_NORMAL, 85, 7, sHintTextColor, 0, sText_CraftingUi_StartButtonCraft);
    AddTextPrinterParameterized3(sCraftInfoWindowId, FONT_NORMAL, 148, 7, sInputTextColor, 0, sText_CraftingUi_SelectButton);
    AddTextPrinterParameterized3(sCraftInfoWindowId, FONT_SMALL, 175, 2, sInputTextColor, 0, sText_CraftingUi_RecipeBook);
}


static void CreateWorkbenchSprite(void)
{
    int row, col, i;
    for (i = 0; i < CRAFT_SLOT_COUNT; i++)
        LoadCompressedSpriteSheet(&sWorkbenchSheets[i]);
    LoadSpritePalette(&sWorkbenchPalette);

    const s16 xBase = WB_CENTER_X - 32;
    const s16 yBase = WB_CENTER_Y - 32;
    for (row = 0; row < 3; row++)
    {
        for (col = 0; col < 3; col++)
        {
            i = row * 3 + col;
            sWorkbenchSpriteIds[i] = CreateSprite(&sWorkbenchTemplates[i], xBase + 32 * col, yBase + 32 * row, 0);
        }
    }
}

static void DestroyWorkbenchSprite(void)
{
    int i;
    for (i = 0; i < CRAFT_SLOT_COUNT; i++)
    {
        if (sWorkbenchSpriteIds[i] != SPRITE_NONE)
        {
            DestroySpriteAndFreeResources(&gSprites[sWorkbenchSpriteIds[i]]);
            sWorkbenchSpriteIds[i] = SPRITE_NONE;
        }
    }
    FreeSpriteTilesByTag(TAG_WB_TOPLEFT);
    FreeSpriteTilesByTag(TAG_WB_TOPMID);
    FreeSpriteTilesByTag(TAG_WB_TOPRIGHT);
    FreeSpriteTilesByTag(TAG_WB_MIDLEFT);
    FreeSpriteTilesByTag(TAG_WB_MIDMID);
    FreeSpriteTilesByTag(TAG_WB_MIDRIGHT);
    FreeSpriteTilesByTag(TAG_WB_BOTLEFT);
    FreeSpriteTilesByTag(TAG_WB_BOTMID);
    FreeSpriteTilesByTag(TAG_WB_BOTRIGHT);
    FreeSpritePaletteByTag(TAG_WB_PALETTE);
}

void CraftMenuUI_DrawIcons(void)
{
    int i;
    for (i = 0; i < CRAFT_SLOT_COUNT; i++)
    {
        if (sCraftSlotSpriteIds[i] != SPRITE_NONE)
        {
            DestroySpriteAndFreeResources(&gSprites[sCraftSlotSpriteIds[i]]);
            sCraftSlotSpriteIds[i] = SPRITE_NONE;
        }

        if (gCraftSlots[i].itemId != ITEM_NONE)
        {
            u16 tag = TAG_CRAFT_ICON_BASE + i;
            u8 spriteId = AddItemIconSprite(tag, tag, gCraftSlots[i].itemId);
            if (spriteId != MAX_SPRITES)
            {
                sCraftSlotSpriteIds[i] = spriteId;
                gSprites[spriteId].x = sWorkbenchSlotPositions[i].x + 79;
                gSprites[spriteId].y = sWorkbenchSlotPositions[i].y + 28;
                gSprites[spriteId].oam.priority = 0;
            }
        }
    }
}

void CraftMenuUI_UpdateGrid(void)
{
    int i;
    FillWindowPixelBuffer(sCraftGridWindowId, PIXEL_FILL(0));
    for (i = 0; i < CRAFT_SLOT_COUNT; i++)
    {
        const struct GridSlotPos *pos = &sWorkbenchSlotPositions[i];
        if (i == sCraftCursorPos)
        {
            AddTextPrinterParameterized3(sCraftGridWindowId, FONT_NORMAL, pos->x - 14, pos->y, sGridTextColor, 0, gText_SelectorArrow2);
        }
    }
    CopyWindowToVram(sCraftGridWindowId, COPYWIN_FULL);
}

bool8 CraftMenuUI_HandleDpadInput(void)
{
    u8 oldPos = sCraftCursorPos;

    if (JOY_NEW(DPAD_UP) && sCraftCursorPos >= 3)
        sCraftCursorPos -= 3;
    else if (JOY_NEW(DPAD_DOWN) && sCraftCursorPos < 6)
        sCraftCursorPos += 3;
    else if (JOY_NEW(DPAD_LEFT) && sCraftCursorPos % 3 != 0)
        sCraftCursorPos--;
    else if (JOY_NEW(DPAD_RIGHT) && sCraftCursorPos % 3 != 2)
        sCraftCursorPos++;

    return (sCraftCursorPos != oldPos);
}

void CraftMenuUI_Init(void)
{
    int i;
    for (i = 0; i < CRAFT_SLOT_COUNT; i++)
    {
        sWorkbenchSpriteIds[i] = SPRITE_NONE;
        sCraftSlotSpriteIds[i] = SPRITE_NONE;
    }
    sCraftCursorPos = 0;

    LoadCraftWindows();
    ShowGridWindow();
    ShowInfoWindow();
    CreateWorkbenchSprite();
    CraftMenuUI_DrawIcons();
    UpdateCraftInfoWindow();
    CraftMenuUI_UpdateGrid();
}

void CraftMenuUI_Close(void)
{
    int i;
    gCraftActiveSlot = sCraftCursorPos;
    ClearStdWindowAndFrame(sCraftGridWindowId, TRUE);
    if (sCraftGridWindowId != WINDOW_NONE)
    {
        RemoveWindow(sCraftGridWindowId);
        sCraftGridWindowId = WINDOW_NONE;
    }
    ClearStdWindowAndFrame(sCraftInfoWindowId, TRUE);
    if (sCraftInfoWindowId != WINDOW_NONE)
    {
        RemoveWindow(sCraftInfoWindowId);
        sCraftInfoWindowId = WINDOW_NONE;
    }

    for (i = 0; i < CRAFT_SLOT_COUNT; i++)
    {
        if (sCraftSlotSpriteIds[i] != SPRITE_NONE)
        {
            DestroySpriteAndFreeResources(&gSprites[sCraftSlotSpriteIds[i]]);
            sCraftSlotSpriteIds[i] = SPRITE_NONE;
        }
    }

    DestroyWorkbenchSprite();
}

u8 CraftMenuUI_GetCursorPos(void)
{
    return sCraftCursorPos;
}

void CraftMenuUI_SetCursorPos(u8 pos)
{
    if (pos < CRAFT_SLOT_COUNT)
        sCraftCursorPos = pos;
    else
        sCraftCursorPos = 0;
    CraftMenuUI_UpdateGrid();
}

void CraftMenuUI_DisplayPackUpMessage(u8 taskId, TaskFunc nextTask)
{
    if (sCraftInfoWindowId != WINDOW_NONE)
    {
        ClearStdWindowAndFrame(sCraftInfoWindowId, TRUE);
        RemoveWindow(sCraftInfoWindowId);
        sCraftInfoWindowId = WINDOW_NONE;
    }

    sPackUpMessageWindowId = AddWindow(&sCraftWindowTemplates[WINDOW_CRAFT_INFO]);
    LoadMessageBoxAndBorderGfx();
    StringExpandPlaceholders(gStringVar4, gText_PackUpQuestion);
    DisplayMessageAndContinueTask(taskId, sPackUpMessageWindowId, DLG_WINDOW_BASE_TILE_NUM,
                                  DLG_WINDOW_PALETTE_NUM, FONT_NORMAL, GetPlayerTextSpeedDelay(),
                                  gStringVar4, nextTask);
    CopyWindowToVram(sPackUpMessageWindowId, COPYWIN_FULL);
}

void CraftMenuUI_ShowPackUpYesNo(void)
{
    CreateYesNoMenu(&sCraftWindowTemplates[WINDOW_CRAFT_YESNO], STD_WINDOW_BASE_TILE_NUM, STD_WINDOW_PALETTE_NUM, 0);
}

void CraftMenuUI_ClearPackUpMessage(void)
{
    if (sPackUpMessageWindowId != WINDOW_NONE)
    {
        ClearDialogWindowAndFrame(sPackUpMessageWindowId, TRUE);
        RemoveWindow(sPackUpMessageWindowId);
        sPackUpMessageWindowId = WINDOW_NONE;
    }

    if (sCraftInfoWindowId == WINDOW_NONE)
    {
        sCraftInfoWindowId = AddWindow(&sCraftWindowTemplates[WINDOW_CRAFT_INFO]);
        ShowInfoWindow();
        UpdateCraftInfoWindow();
    }
}


