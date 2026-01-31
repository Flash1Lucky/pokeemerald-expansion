#ifndef GUARD_BATTLE_INFO_H
#define GUARD_BATTLE_INFO_H

#include "battle.h"

void CB2_BattleInfoMenu(void);

void BattleInfo_RecordSwitchIn(u32 battler);
void BattleInfo_RecordFaint(u32 battler);
void BattleInfo_RecordHp(u32 battler);
void BattleInfo_RecordStatus(u32 battler);
void BattleInfo_RecordKnownMove(u32 battler, u16 move, u8 moveIndex);
void BattleInfo_RecordMoveUsed(u32 battler, u16 move, u8 moveIndex);
void BattleInfo_RecordItem(u32 battler, u16 itemId);
void BattleInfo_RecordFieldCondition(enum BattleInfoFieldCondition condition, u32 battler);
void BattleInfo_RecordSideCondition(u32 side, enum BattleInfoSideCondition condition, u32 battler);

#endif // GUARD_BATTLE_INFO_H
