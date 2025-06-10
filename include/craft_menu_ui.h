#ifndef GUARD_CRAFT_MENU_UI_H
#define GUARD_CRAFT_MENU_UI_H

#include "global.h"

void CraftMenuUI_Init(void);
void CraftMenuUI_UpdateGrid(void);
bool8 CraftMenuUI_HandleDpadInput(void);
void CraftMenuUI_Close(void);
u8 CraftMenuUI_GetCursorPos(void);

#endif // GUARD_CRAFT_MENU_UI_H
