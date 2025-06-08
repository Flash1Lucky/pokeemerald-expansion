// === craft_menu.c ===
#include "global.h"
#include "task.h"
#include "script.h"
#include "sound.h"
#include "menu.h"
#include "bg.h"
#include "gpu_regs.h"
#include "main.h"
#include "palette.h"
#include "graphics.h"
#include "window.h"
#include "text_window.h"
#include "text.h"
#include "field_screen_effect.h"
#include "constants/songs.h"
#include "field_player_avatar.h"
#include "start_menu.h"
#include "window.h"
#include "event_object_lock.h"
#include "malloc.h"
#include "sprite.h"
#include "decompress.h"
#include "strings.h"

// === Static Declarations ===
static void Task_EnterCraftMenu(u8 taskId);
static void Task_RunCraftMenu(u8 taskId);
static void InitCraftMenu(void);
static bool8 HandleCraftMenuInput(void);
static void CloseCraftMenu(void);
static void LoadCraftWindows(void);
static void ShowInfoWindow(void);
static void UpdateCraftInfoWindow(void);
static void CreateWorkbenchSprite(void);
static void DestroyWorkbenchSprite(void);

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

#define WB_CENTER_X 120
#define WB_CENTER_Y 60
#define WB_OFFSET   32

// === Window IDs ===
EWRAM_DATA static u8 sCraftInfoWindowId = 0;
EWRAM_DATA static u8 sWorkbenchSpriteIds[9];

// === Window Layout Enum ===
enum
{
    WINDOW_CRAFT_INFO,
    //WINDOW_CRAFT_OPTIONS,
    //WINDOW_QUANTITY,
    NUM_CRAFT_WINDOWS
};

// === Window Templates ===
static const struct WindowTemplate sCraftWindowTemplates[NUM_CRAFT_WINDOWS] =
{
    [WINDOW_CRAFT_INFO] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 26,
        .height = 3,
        .paletteNum = 15,
        .baseBlock = 1
    }/*,
    [WINDOW_CRAFT_OPTIONS] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 14,
        .width = 12,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 209
    },
    [WINDOW_QUANTITY] = {
        .bg = 0,
        .tilemapLeft = 19,
        .tilemapTop = 14,
        .width = 6,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 257
    }*/
};

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

static void InitCraftMenu(void)
{
    for (int i = 0; i < 9; i++)
        sWorkbenchSpriteIds[i] = SPRITE_NONE;

    LoadCraftWindows();
    ShowInfoWindow();
    CreateWorkbenchSprite();
    UpdateCraftInfoWindow();
}

static void LoadCraftWindows(void)
{
    sCraftInfoWindowId = AddWindow(&sCraftWindowTemplates[WINDOW_CRAFT_INFO]);
}

static void ShowInfoWindow(void)
{
    DrawStdFrameWithCustomTileAndPalette(sCraftInfoWindowId, TRUE, 0x214, 14);
    FillWindowPixelBuffer(sCraftInfoWindowId, PIXEL_FILL(1));
    CopyWindowToVram(sCraftInfoWindowId, COPYWIN_FULL);
}

static void UpdateCraftInfoWindow(void)
{
    AddTextPrinterParameterized(sCraftInfoWindowId, FONT_NORMAL, gText_Blank, 1, 1, 0, NULL);
}

static const struct CompressedSpriteSheet sWorkbenchSheets[] =
{
    { gCraftWorkbench_TopLeft_Gfx,    0x400, TAG_WB_TOPLEFT },
    { gCraftWorkbench_TopMid_Gfx,     0x400, TAG_WB_TOPMID },
    { gCraftWorkbench_TopRight_Gfx,   0x400, TAG_WB_TOPRIGHT },
    { gCraftWorkbench_MidLeft_Gfx,    0x400, TAG_WB_MIDLEFT },
    { gCraftWorkbench_MidMid_Gfx,     0x400, TAG_WB_MIDMID },
    { gCraftWorkbench_MidRight_Gfx,   0x400, TAG_WB_MIDRIGHT },
    { gCraftWorkbench_BotLeft_Gfx,    0x400, TAG_WB_BOTLEFT },
    { gCraftWorkbench_BotMid_Gfx,     0x400, TAG_WB_BOTMID },
    { gCraftWorkbench_BotRight_Gfx,   0x400, TAG_WB_BOTRIGHT },
};

static const struct SpritePalette sWorkbenchPalette = { gCraftWorkbench_Pal, TAG_WB_PALETTE };

static const struct OamData sOam_Workbench32 =
{
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 0,
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

static const struct SpriteTemplate sWorkbenchTemplates[9] =
{
    WB_TEMPLATE(TAG_WB_TOPLEFT),   // 0
    WB_TEMPLATE(TAG_WB_TOPMID),    // 1
    WB_TEMPLATE(TAG_WB_TOPRIGHT),  // 2
    WB_TEMPLATE(TAG_WB_MIDLEFT),   // 3
    WB_TEMPLATE(TAG_WB_MIDMID),    // 4
    WB_TEMPLATE(TAG_WB_MIDRIGHT),  // 5
    WB_TEMPLATE(TAG_WB_BOTLEFT),   // 6
    WB_TEMPLATE(TAG_WB_BOTMID),    // 7
    WB_TEMPLATE(TAG_WB_BOTRIGHT),  // 8
};

static void CreateWorkbenchSprite(void)
{
    for (int i = 0; i < 9; i++)
        LoadCompressedSpriteSheet(&sWorkbenchSheets[i]);
    LoadSpritePalette(&sWorkbenchPalette);

    const s16 xBase = WB_CENTER_X - 32;
    const s16 yBase = WB_CENTER_Y - 32;
    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 3; col++)
        {
            int i = row * 3 + col;
            sWorkbenchSpriteIds[i] = CreateSprite(&sWorkbenchTemplates[i], xBase + 32 * col, yBase + 32 * row, 0);
        }
    }
}

static void DestroyWorkbenchSprite(void)
{
    for (int i = 0; i < 9; i++)
    {
        if (sWorkbenchSpriteIds[i] != SPRITE_NONE)
        {
            DestroySprite(&gSprites[sWorkbenchSpriteIds[i]]);
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

static bool8 HandleCraftMenuInput(void)
{
    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        CloseCraftMenu();
        return TRUE;
    }
    return FALSE;
}

static void CloseCraftMenu(void)
{
    ClearStdWindowAndFrame(sCraftInfoWindowId, TRUE);
    if (sCraftInfoWindowId != WINDOW_NONE)
    {
        RemoveWindow(sCraftInfoWindowId);
        sCraftInfoWindowId = WINDOW_NONE;
    }
    DestroyWorkbenchSprite();
    ScriptUnfreezeObjectEvents();
    UnlockPlayerFieldControls();
    ScriptContext_Enable();
}
