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

// === Window IDs ===
EWRAM_DATA static u8 sCraftInfoWindowId = 0;
//EWRAM_DATA static u8 sCraftOptionsWindowId = 0;
//EWRAM_DATA static u8 sCraftQuantityWindowId = 0;
EWRAM_DATA static u8 sWorkbenchSpriteId;

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

// === Entry Point from Script ===
void StartCraftMenu(void)
{
    PlayerFreeze();
    StopPlayerAvatar();
    ScriptContext_Stop();
    LockPlayerFieldControls();
    CreateTask(Task_EnterCraftMenu, 0);
}

// === Initial Task: Setup Craft Menu ===
static void Task_EnterCraftMenu(u8 taskId)
{
    InitCraftMenu();
    gMenuCallback = HandleCraftMenuInput;
    gTasks[taskId].func = Task_RunCraftMenu;
}

// === Main Menu Task: Handles Input ===
static void Task_RunCraftMenu(u8 taskId)
{
    if (gMenuCallback && gMenuCallback() == TRUE)
    {
        DestroyTask(taskId);
    }
}

// === Setup Crafting Menu Internals ===
static void InitCraftMenu(void)
{
    sWorkbenchSpriteId = SPRITE_NONE;
    
    LoadCraftWindows();
    ShowInfoWindow();
    CreateWorkbenchSprite();
    UpdateCraftInfoWindow();
}

// === Window Setup ===
static void LoadCraftWindows(void)
{
    // Info Window
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

// === Workbench Sprite ===
enum { TAG_WORKBENCH = 0x4000 };

static const struct CompressedSpriteSheet sWorkbenchSheet = { gCraftMenuWorkbench_Gfx, 0x800, TAG_WORKBENCH };
static const struct SpritePalette sWorkbenchPalette = { gCraftMenuWorkbench_Pal, TAG_WORKBENCH };

static const struct OamData sOam_Workbench =
{
    .shape = SPRITE_SHAPE(64x64),
    .size = SPRITE_SIZE(64x64),
    .priority = 0,
};

static const union AnimCmd sAnim_Workbench[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END,
};

static const union AnimCmd *const sAnims_Workbench[] =
{
    sAnim_Workbench,
};

static const struct SpriteTemplate sWorkbenchTemplate =
{
    .tileTag = TAG_WORKBENCH,
    .paletteTag = TAG_WORKBENCH,
    .oam = &sOam_Workbench,
    .anims = sAnims_Workbench,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static void CreateWorkbenchSprite(void)
{
    if (sWorkbenchSpriteId == SPRITE_NONE)
    {
        LoadCompressedSpriteSheet(&sWorkbenchSheet);
        LoadSpritePalette(&sWorkbenchPalette);
        sWorkbenchSpriteId = CreateSprite(&sWorkbenchTemplate, 120, 60, 0);
    }
}

static void DestroyWorkbenchSprite(void)
{
    if (sWorkbenchSpriteId != SPRITE_NONE)
    {
        FreeSpriteTilesByTag(TAG_WORKBENCH);
        FreeSpritePaletteByTag(TAG_WORKBENCH);
        DestroySprite(&gSprites[sWorkbenchSpriteId]);
        sWorkbenchSpriteId = SPRITE_NONE;
    }
}

// === Input Handler ===
static bool8 HandleCraftMenuInput(void)
{
    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        CloseCraftMenu();
        return TRUE;
    }

    // Placeholder: add other input logic here

    return FALSE;
}

// === Cleanup Function ===
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
