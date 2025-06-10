#include "global.h"
#include "task.h"
#include "script.h"
#include "sound.h"
#include "main.h"
#include "constants/songs.h"
#include "start_menu.h"
#include "field_player_avatar.h"
#include "event_object_lock.h"
#include "craft_menu_ui.h"
#include "craft_logic.h"

static void Task_EnterCraftMenu(u8 taskId);
static void Task_RunCraftMenu(u8 taskId);
static void InitCraftMenu(void);
static bool8 HandleCraftMenuInput(void);
static void CloseCraftMenu(void);

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
    CraftLogic_InitSlots();
    CraftMenuUI_Init();
}

static void CB2_ReturnToCraftMenu(void)
{
    SetMainCallback2(CB2_OpenCraftMenu);
}

static void OpenBagFromCraftMenu(void)
{
    gCraftActiveSlot = sCraftCursorPos;
    CraftMenuUI_Close();
    SetBagPreOpenCallback(BagPreOpen_SetCursorItem);
    GoToBagMenu(ITEMMENULOCATION_CRAFTING, POCKETS_COUNT, CB2_ReturnToCraftMenu);
}

static bool8 HandleCraftMenuInput(void)
{
    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        CloseCraftMenu();
        return TRUE;
    }

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        OpenBagFromCraftMenu();
        return TRUE;
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

void CB2_OpenCraftMenu(void)
{
    StartCraftMenu();
}
