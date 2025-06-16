#include "global.h"
#include "task.h"
#include "script.h"
#include "sound.h"
#include "main.h"
#include "constants/songs.h"
#include "start_menu.h"
#include "menu.h"
#include "menu_helpers.h"
#include "field_player_avatar.h"
#include "event_object_lock.h"
#include "craft_menu_ui.h"
#include "craft_logic.h"
#include "item.h"
#include "item_menu.h"
#include "strings.h"
#include "craft_menu.h"
#include "craft_debug.h"
#include "config/crafting.h"
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
static void Task_AdjustQuantity(u8 taskId);
static void CraftMenu_ReshowAfterBagMenu(void);
void CB2_ReturnToCraftMenu(void);

static const u8 sText_CraftPlaceHowManyVar1[] = _("Place how many {STR_VAR_1}?");

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

static bool8 sKeepSlots = FALSE;

enum {
    SLOT_ACTION_SWAP_ITEM,
    SLOT_ACTION_ADJUST_QTY,
    SLOT_ACTION_SWAP_SLOT,
    SLOT_ACTION_REMOVE_ITEM,
    SLOT_ACTION_CANCEL,
};

static u8 sSwapFromSlot = 0;

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
            int row = CraftMenuUI_GetCursorPos() / CRAFT_COLS;
            int col = CraftMenuUI_GetCursorPos() % CRAFT_COLS;
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

#if DEBUG_CRAFT_MENU
    if (JOY_NEW(R_BUTTON))
    {
        PlaySE(SE_SELECT);
        FadeScreen(FADE_TO_BLACK, 0);
        CreateTask(Task_WaitFadeAndOpenDebugMenu, 0);
        return TRUE;
    }
#endif

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

static void Action_SwapItem(void)
{
    OpenBagFromCraftMenu();
}

static void Action_RemoveItem(void)
{
    u8 slot = CraftMenuUI_GetCursorPos();
    struct ItemSlot *s = &gCraftSlots[slot / CRAFT_COLS][slot % CRAFT_COLS];
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
    CreateTask(Task_AdjustQuantity, 0);
}

static bool8 HandleSlotActionInput(void)
{
    s8 selection = CraftMenuUI_ProcessActionMenuInput();
    if (selection == MENU_NOTHING_CHOSEN)
        return FALSE;

    CraftMenuUI_HideActionMenu();
    switch (selection)
    {
    case SLOT_ACTION_SWAP_ITEM:
        Action_SwapItem();
        return TRUE;
    case SLOT_ACTION_ADJUST_QTY:
        Action_AdjustQuantity();
        gMenuCallback = NULL;
        return FALSE;
    case SLOT_ACTION_SWAP_SLOT:
        Action_StartSwapSlot();
        return FALSE;
    case SLOT_ACTION_REMOVE_ITEM:
        Action_RemoveItem();
        break;
    case SLOT_ACTION_CANCEL:
    default:
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

static void Task_AdjustQuantity(u8 taskId)
{
    static u16 sOldQty;
    static u16 sMaxQty;
    static u16 sItemId;
    switch (gTasks[taskId].data[0])
    {
    case 0:
        {
            int row = CraftMenuUI_GetCursorPos() / CRAFT_COLS;
            int col = CraftMenuUI_GetCursorPos() % CRAFT_COLS;
            sItemId = gCraftSlots[row][col].itemId;
            sOldQty = gCraftSlots[row][col].quantity;
        }
        sMaxQty = sOldQty + CountTotalItemQuantityInBag(sItemId);
        gTasks[taskId].data[1] = sOldQty;
        gMenuCallback = NULL;
        CopyItemNameHandlePlural(sItemId, gStringVar1, 2);
        StringExpandPlaceholders(gStringVar4, sText_CraftPlaceHowManyVar1);
        CraftMenuUI_PrintInfo(gStringVar4, 2, 7);
        CraftMenuUI_AddQuantityWindow();
        CraftMenuUI_PrintQuantity(sOldQty);
        gTasks[taskId].data[0] = 1;
        break;
    case 1:
        if (AdjustQuantityAccordingToDPadInput(&gTasks[taskId].data[1], sMaxQty))
        {
            CraftMenuUI_PrintQuantity(gTasks[taskId].data[1]);
        }

        if (JOY_NEW(A_BUTTON))
        {
            u16 newQty = gTasks[taskId].data[1];
            int row = CraftMenuUI_GetCursorPos() / CRAFT_COLS;
            int col = CraftMenuUI_GetCursorPos() % CRAFT_COLS;
            if (newQty > sOldQty)
                RemoveBagItem(sItemId, newQty - sOldQty);
            else if (newQty < sOldQty)
                AddBagItem(sItemId, sOldQty - newQty);
            gCraftSlots[row][col].quantity = newQty;
            CraftMenuUI_DrawIcons();
            gTasks[taskId].data[0] = 2;
        }
        else if (JOY_NEW(B_BUTTON))
        {
            gTasks[taskId].data[0] = 2;
        }
        break;
    case 2:
        CraftMenuUI_RemoveQuantityWindow();
        CraftMenuUI_RedrawInfo();
        gMenuCallback = HandleCraftMenuInput;
        DestroyTask(taskId);
        break;
    }
}

static const struct YesNoFuncTable sPackUpYesNoFuncs = {
    .yesFunc = PackUpYes,
    .noFunc = PackUpNo,
};

static void Task_ShowPackUpYesNo(u8 taskId)
{
    CraftMenuUI_ShowPackUpYesNo();
    DoYesNoFuncWithChoice(taskId, &sPackUpYesNoFuncs);
}

static void Task_PackUpAsk(u8 taskId)
{
    CraftMenuUI_DisplayPackUpMessage(taskId, Task_ShowPackUpYesNo);
}

void CB2_OpenCraftMenu(void)
{
    StartCraftMenu();
    SetMainCallback2(CB2_Overworld);
}

static void CraftMenu_ReshowAfterBagMenu(void)
{
    sKeepSlots = TRUE;
    StartCraftMenu();
    FadeInFromBlack();
}
