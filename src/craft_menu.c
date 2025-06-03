#include "global.h"
#include "task.h"
#include "script.h"
#include "sound.h"
#include "menu.h"
#include "bg.h"
#include "gpu_regs.h"
#include "main.h"
#include "palette.h"
#include "window.h"
#include "text_window.h"
#include "text.h"
#include "field_screen_effect.h"
#include "constants/songs.h"
#include "field_player_avatar.h"
#include "start_menu.h"
#include "window.h"
#include "event_object_lock.h"

// === Static Declarations ===
static void Task_EnterCraftMenu(u8 taskId);
static void Task_RunCraftMenu(u8 taskId);
static void InitCraftMenu(void);
static bool8 HandleCraftMenuInput(void);
static void CloseCraftMenu(void);
static void LoadCraftWindows(void);
static void ShowCraftTableAndInfoWindows(void);
static void PrintCraftTableItems(void);
static void UpdateCraftInfoWindow(void);

// === Window IDs ===
EWRAM_DATA static u8 sCraftTableWindowId = 0;
EWRAM_DATA static u8 sCraftInfoWindowId = 0;
EWRAM_DATA static u8 sCraftOptionsWindowId = 0;
EWRAM_DATA static u8 sCraftQuantityWindowId = 0;

// === Window Layout Enum ===
enum
{
    WINDOW_CRAFT_TABLE,
    WINDOW_CRAFT_INFO,
    WINDOW_CRAFT_OPTIONS,
    WINDOW_QUANTITY,
    NUM_CRAFT_WINDOWS
};

// === Window Templates ===
static const struct WindowTemplate sCraftWindowTemplates[NUM_CRAFT_WINDOWS] =
{
    [WINDOW_CRAFT_TABLE] = {
        .bg = 0,
        .tilemapLeft = 6,
        .tilemapTop = 2,
        .width = 20,
        .height = 10,
        .paletteNum = 15,
        .baseBlock = 1
    },
    [WINDOW_CRAFT_INFO] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 16,
        .width = 30,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 145
    },
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
    }
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
    LoadCraftWindows();
    ShowCraftTableAndInfoWindows();
    PrintCraftTableItems();
    UpdateCraftInfoWindow();
}

// === Window Setup ===
static void LoadCraftWindows(void)
{
    // Craft Table Window
    sCraftTableWindowId = AddWindow(&sCraftWindowTemplates[WINDOW_CRAFT_TABLE]);

    // Info Window
    sCraftInfoWindowId = AddWindow(&sCraftWindowTemplates[WINDOW_CRAFT_INFO]);
}

static void ShowCraftTableAndInfoWindows(void)
{
    // Table Window (give this a green fill)
    DrawStdFrameWithCustomTileAndPalette(sCraftTableWindowId, TRUE, 0x214, 14);
    FillWindowPixelBuffer(sCraftTableWindowId, PIXEL_FILL(3));  // ← Use a distinct color
    CopyWindowToVram(sCraftTableWindowId, COPYWIN_FULL);

    // Info Window (use a different fill color)
    DrawStdFrameWithCustomTileAndPalette(sCraftInfoWindowId, TRUE, 0x214, 14);
    FillWindowPixelBuffer(sCraftInfoWindowId, PIXEL_FILL(1));  // ← Try black or gray
    CopyWindowToVram(sCraftInfoWindowId, COPYWIN_FULL);
}

static void PrintCraftTableItems(void)
{
    AddTextPrinterParameterized(sCraftTableWindowId, FONT_NORMAL, (const u8*)"[WIP] Craft Grid", 2, 2, 0, NULL);
}

static void UpdateCraftInfoWindow(void)
{
    AddTextPrinterParameterized(sCraftInfoWindowId, FONT_NORMAL, (const u8*)"[B] Cancel\n[A] Add Item", 1, 1, 0, NULL);
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
    ClearStdWindowAndFrame(sCraftTableWindowId, TRUE);
    if (sCraftTableWindowId != WINDOW_NONE)
    {
        RemoveWindow(sCraftTableWindowId);
        sCraftTableWindowId = WINDOW_NONE;
    }

    ClearStdWindowAndFrame(sCraftInfoWindowId, TRUE);
    if (sCraftInfoWindowId != WINDOW_NONE)
    {
        RemoveWindow(sCraftInfoWindowId);
        sCraftInfoWindowId = WINDOW_NONE;
    }

    ScriptUnfreezeObjectEvents();
    UnlockPlayerFieldControls();
    ScriptContext_Enable();
}
