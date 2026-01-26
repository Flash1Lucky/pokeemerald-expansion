#ifndef GUARD_BATTLE_INFO_H
#define GUARD_BATTLE_INFO_H

void CB2_BattleInfoMenu(void);

void BattleInfo_RecordSwitchIn(u32 battler);
void BattleInfo_RecordFaint(u32 battler);
void BattleInfo_RecordHp(u32 battler);
void BattleInfo_RecordStatus(u32 battler);
void BattleInfo_RecordKnownMove(u32 battler, u16 move, u8 moveIndex);
void BattleInfo_RecordMoveUsed(u32 battler, u16 move, u8 moveIndex);
void BattleInfo_RecordItem(u32 battler, u16 itemId);

#endif // GUARD_BATTLE_INFO_H
