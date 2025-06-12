#include "global.h"
#include "bg.h"
#include "gpu_regs.h"
#include "palette.h"
#include "window.h"
#include "text_window.h"
#include "text.h"
#include "string_util.h"
#include "main.h"
#include "menu.h"
#include "craft_logic.h"
#include "craft_menu.h"
#include "craft_debug_menu.h"
#include "constants/rgb.h"

static const u8 sDebugTextColor[] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};

static const u16 sBgColor[] = { RGB_WHITE };

static const struct BgTemplate sBgTemplate[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    }
};

#define WIN_DEBUG 0

static const struct WindowTemplate sDebugWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 2,
    .width = 18,
    .height = 12,
    .paletteNum = 15,
    .baseBlock = 0x1A0
};

static void Task_CraftDebugMenu(u8 taskId);
#define tState      data[0]
#define tWindowId   data[1]

static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void CB2_CraftDebugMenu(void)
{
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplate, ARRAY_COUNT(sBgTemplate));
        ResetAllBgsCoordinates();
        FreeAllWindowBuffers();
        InitWindows(&sDebugWindowTemplate);
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        gMain.state++;
        break;
    case 1:
        LoadMessageBoxAndBorderGfx();
        ResetPaletteFade();
        LoadPalette(sBgColor, 0, 2);
        LoadPalette(GetOverworldTextboxPalettePtr(), 0xf0, 16);
        gMain.state++;
        break;
    case 2:
        CreateTask(Task_CraftDebugMenu, 0);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        gMain.state++;
        break;
    case 3:
        break;
    }
}

static void PrintSlots(u8 windowId)
{
    u8 i;
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    for (i = 0; i < CRAFT_SLOT_COUNT; i++)
    {
        u8 y = i * 10;
        ConvertIntToDecimalStringN(gStringVar3, i + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
        StringCopy(gStringVar4, gStringVar3);
        StringAppend(gStringVar4, (const u8*)": ");
        if (gCraftSlots[i].itemId != ITEM_NONE)
        {
            CopyItemName(gCraftSlots[i].itemId, gStringVar2);
            StringAppend(gStringVar4, gStringVar2);
            ConvertIntToDecimalStringN(gStringVar1, gCraftSlots[i].quantity, STR_CONV_MODE_LEFT_ALIGN, 3);
            StringExpandPlaceholders(gStringVar2, gText_xVar1);
            StringAppend(gStringVar4, gStringVar2);
        }
        else
        {
            StringAppend(gStringVar4, gText_None);
        }
        AddTextPrinterParameterized3(windowId, FONT_SMALL, 1, y, sDebugTextColor, 0, gStringVar4);
    }
    CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void Task_CraftDebugMenu(u8 taskId)
{
    switch (gTasks[taskId].tState)
    {
    case 0:
        gTasks[taskId].tWindowId = AddWindow(&sDebugWindowTemplate);
        DrawStdFrameWithCustomTileAndPalette(gTasks[taskId].tWindowId, TRUE, 0x214, 14);
        PrintSlots(gTasks[taskId].tWindowId);
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_WHITE);
        gTasks[taskId].tState++;
        break;
    case 1:
        if (!gPaletteFade.active)
            gTasks[taskId].tState++;
        break;
    case 2:
        if (JOY_NEW(B_BUTTON | A_BUTTON))
        {
            BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_WHITE);
            gTasks[taskId].tState++;
        }
        break;
    case 3:
        if (!gPaletteFade.active)
        {
            ClearStdWindowAndFrame(gTasks[taskId].tWindowId, TRUE);
            RemoveWindow(gTasks[taskId].tWindowId);
            DestroyTask(taskId);
            SetMainCallback2(CB2_ReturnToCraftMenu);
        }
        break;
    }
}

#undef tState
#undef tWindowId
