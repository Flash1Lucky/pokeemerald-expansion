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

#endif // GUARD_CRAFT_MENU_UI_H
