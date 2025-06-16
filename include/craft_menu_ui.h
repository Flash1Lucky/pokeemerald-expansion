#ifndef GUARD_CRAFT_MENU_UI_H
#define GUARD_CRAFT_MENU_UI_H

#include "global.h"
#include "task.h"

void CraftMenuUI_Init(void);
void CraftMenuUI_UpdateGrid(void);
void CraftMenuUI_DrawIcons(void);
bool8 CraftMenuUI_HandleDpadInput(void);
void CraftMenuUI_Close(void);
u8 CraftMenuUI_GetCursorPos(void);
void CraftMenuUI_SetCursorPos(u8 pos);

void CraftMenuUI_DisplayPackUpMessage(u8 taskId, TaskFunc nextTask);
void CraftMenuUI_ShowPackUpYesNo(void);
void CraftMenuUI_ClearPackUpMessage(void);

// Additional UI features
void CraftMenuUI_ShowActionMenu(void);
void CraftMenuUI_HideActionMenu(void);
s8 CraftMenuUI_ProcessActionMenuInput(void);
void CraftMenuUI_StartSwapMode(void);
void CraftMenuUI_EndSwapMode(void);
bool8 CraftMenuUI_InSwapMode(void);
void CraftMenuUI_RedrawInfo(void);
void CraftMenuUI_PrintInfo(const u8 *text, u8 x, u8 y);

#endif // GUARD_CRAFT_MENU_UI_H
