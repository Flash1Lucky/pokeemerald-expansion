#include "global.h"
#include "battle.h"
#include "battle_info.h"
#include "battle_interface.h"
#include "battle_util.h"
#include "bg.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "item.h"
#include "main.h"
#include "malloc.h"
#include "move.h"
#include "menu.h"
#include "menu_helpers.h"
#include "palette.h"
#include "pokemon.h"
#include "pokemon_icon.h"
#include "pokemon_summary_screen.h"
#include "pokeball.h"
#include "graphics.h"
#include "reshow_battle_screen.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "international_string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trainer_pokemon_sprites.h"
#include "window.h"
#include "constants/abilities.h"
#include "constants/battle.h"
#include "constants/characters.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/party_menu.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define BINFO_HP_BAR_SCALE 48
#define BINFO_HP_BAR_SEGMENTS_SUMMARY 10
#define BINFO_HP_BAR_SEGMENTS_FIELD 6
#define BINFO_HP_BAR_LABEL_PIXELS 16
#define BINFO_HP_BAR_TOTAL_WIDTH (BINFO_HP_BAR_LABEL_PIXELS + BINFO_HP_BAR_SCALE)
#define BINFO_ICON_START_X 15
#define BINFO_ICON_SPACING_X 41
#define BINFO_ICON_Y 28
#define BINFO_NAME_LIMIT 10
#define BINFO_FADEIN_TIMEOUT 32
#define BINFO_BALL_TILE_TAG 0xD8F0
#define BINFO_BALL_PAL_TAG 0xD8F1
#define BINFO_HP_BAR_TILE_TAG 0xD8F2
#define BINFO_HP_BAR_PAL_TAG 0xD8F3
#define BINFO_STATUS_ICON_TILE_TAG 0xD8F4
#define BINFO_STATUS_ICON_PAL_TAG 0xD8F5
#define BINFO_STAT_STAGE_TILE_TAG_BASE 0xD8F6
#define BINFO_STAT_STAGE_TILE_TAG_COUNT 12
#define BINFO_STAT_ICON_TILE_TAG_BASE (BINFO_STAT_STAGE_TILE_TAG_BASE + BINFO_STAT_STAGE_TILE_TAG_COUNT)
#define BINFO_STAT_ICON_TILE_TAG_COUNT 7
#define BINFO_STAT_INDICATOR_PLUS_PAL_TAG (BINFO_STAT_ICON_TILE_TAG_BASE + BINFO_STAT_ICON_TILE_TAG_COUNT)
#define BINFO_STAT_INDICATOR_MINUS_PAL_TAG (BINFO_STAT_INDICATOR_PLUS_PAL_TAG + 1)
#define BINFO_SUMMARY_SPRITE_X 32
#define BINFO_SUMMARY_SPRITE_Y 72
#define BINFO_SUMMARY_RIGHT_X 72
#define BINFO_SUMMARY_RIGHT_Y 56
#define BINFO_SUMMARY_NAME_Y 96
#define BINFO_SUMMARY_SPRITE_WIDTH 64
#define BINFO_SUMMARY_NAME_OFFSET_Y 34
#define BINFO_SUMMARY_LEVEL_OFFSET_Y 16
#define BINFO_SUMMARY_LEVEL_CENTER_OFFSET 16
#define BINFO_SUMMARY_GENDER_CENTER_OFFSET 16
#define BINFO_SUMMARY_SPRITE_LOAD_DELAY 2
#define BINFO_TYPE_ICON_COUNT 2
#define BINFO_TYPE_ICON_WIDTH 32
#define BINFO_TYPE_ICON_GAP 4
#define BINFO_TYPE_LABEL_GAP 4
#define BINFO_TYPE_LINE_OFFSET 24
#define BINFO_STATUS_INDICATOR_GAP 8
#define BINFO_STATUS_INDICATOR_Y_OFFSET 4
#define BINFO_FIELD_COLUMN_WIDTH 112
#define BINFO_FIELD_COLUMN_LEFT_X 4
#define BINFO_FIELD_COLUMN_RIGHT_X (BINFO_FIELD_COLUMN_LEFT_X + BINFO_FIELD_COLUMN_WIDTH)
#define BINFO_FIELD_SECTION_HEIGHT 64
#define BINFO_FIELD_ICON_WIDTH 32
#define BINFO_FIELD_ICON_X_OFFSET 12
#define BINFO_FIELD_ICON_Y_OFFSET 4
#define BINFO_FIELD_NAME_X_OFFSET 34
#define BINFO_FIELD_NAME_Y_OFFSET -2
#define BINFO_FIELD_HP_X_OFFSET 30
#define BINFO_FIELD_HP_Y_OFFSET 10
#define BINFO_FIELD_MAX_SLOTS 2
#define BINFO_FIELD_RESERVED_Y_OFFSET (BINFO_FIELD_HP_Y_OFFSET + 16)
#define BINFO_FIELD_STATUS_X_OFFSET (BINFO_FIELD_ICON_X_OFFSET - (BINFO_FIELD_ICON_WIDTH / 2) + 2)
#define BINFO_FIELD_STATUS_Y_OFFSET BINFO_FIELD_RESERVED_Y_OFFSET
#define BINFO_FIELD_STATUS_LINE_HEIGHT 9
#define BINFO_FIELD_FLOW_ROWS 4
#define BINFO_FIELD_DETAIL_SLOTS_PER_ROW 4
#define BINFO_FIELD_SLOT_WIDTH 24
#define BINFO_FIELD_SLOT_GAP 2
#define BINFO_FIELD_CELL_WIDTH (BINFO_FIELD_SLOT_WIDTH + BINFO_FIELD_SLOT_GAP)
#define BINFO_FIELD_STATUS_ICON_WIDTH 20
#define BINFO_FIELD_STATUS_ICON_SPRITE_WIDTH 32
#define BINFO_FIELD_STATUS_ICON_X_ADJUST -6
#define BINFO_FIELD_STATUS_TURN_GAP 2
#define BINFO_FIELD_FLOW_TEXT_Y_OFFSET -8
#define BINFO_FIELD_TURNS_TEXT_SINGULAR " turn)"
#define BINFO_FIELD_TURNS_TEXT_PLURAL " turns)"
#define BINFO_FIELD_TURNS_LEFT_SINGULAR " turn left)"
#define BINFO_FIELD_TURNS_LEFT_PLURAL " turns left)"
#define BINFO_FIELD_TURNS_PASSED_SINGULAR " turn passed)"
#define BINFO_FIELD_TURNS_PASSED_PLURAL " turns passed)"
#define BINFO_FIELD_STAT_STAGE_WIDTH 8
#define BINFO_FIELD_STAT_ICON_X_OFFSET BINFO_FIELD_STAT_STAGE_WIDTH
#define BINFO_FIELD_STAT_ICON_WIDTH 16
#define BINFO_FIELD_STAT_INDICATOR_WIDTH (BINFO_FIELD_STAT_ICON_X_OFFSET + BINFO_FIELD_STAT_ICON_WIDTH)
#define BINFO_FIELD_STAT_INDICATORS_MAX 7
#define BINFO_FIELD_STAT_SPRITES_PER_INDICATOR 2
#define BINFO_FIELD_STAT_SPRITE_COUNT (BINFO_FIELD_STAT_INDICATORS_MAX * BINFO_FIELD_STAT_SPRITES_PER_INDICATOR)
#define BINFO_FIELD_STAT_INDICATOR_HEIGHT 8
#define BINFO_FIELD_STAT_INDICATOR_GAP_X 2
#define BINFO_FIELD_STAT_INDICATOR_GAP_Y 1
#define BINFO_FIELD_EFFECT_LINE_HEIGHT 10
#define BINFO_FIELD_EFFECT_LINE_LEN 48
#define BINFO_FIELD_EFFECT_MAX_LINES 6
#define BINFO_FIELD_PAGE_MAX_LINES 12
#define BINFO_FIELD_EFFECT_SECTION_GAP 4

#define BINFO_HP_BAR_TILES 8
#define BINFO_HP_BAR_PIXELS_PER_TILE 8

// Matches gHealthboxElementsGfxTable indices in battle_interface.c
#define BINFO_HEALTHBOX_GFX_HP_BAR_H 1
#define BINFO_HEALTHBOX_GFX_HP_BAR_P 2
#define BINFO_HEALTHBOX_GFX_HP_BAR_GREEN 3
#define BINFO_HEALTHBOX_GFX_HP_BAR_YELLOW 50
#define BINFO_HEALTHBOX_GFX_HP_BAR_RED 59
#define BINFO_HEALTHBOX_GFX_FRAME_END_BAR 129

enum
{
    BINFO_TAB_ENEMY,
    BINFO_TAB_FIELD,
};

enum
{
    ENEMY_VIEW_SUMMARY,
    ENEMY_VIEW_DETAILS,
};

enum
{
    DETAILS_PAGE_INFO,
    DETAILS_PAGE_MOVES,
};

enum
{
    ABILITY_MODE_NONE,
    ABILITY_MODE_LIST,
    ABILITY_MODE_POPUP,
};

enum
{
    FIELD_PAGE_ACTIVE,
    FIELD_PAGE_EFFECTS,
    FIELD_PAGE_COUNT,
};

enum
{
    BINFO_GENDER_MALE,
    BINFO_GENDER_FEMALE,
    BINFO_GENDERLESS,
};

enum
{
    BINFO_EFFECT_COLOR_DEFAULT,
    BINFO_EFFECT_COLOR_HELPFUL,
    BINFO_EFFECT_COLOR_HAZARD,
};

struct BattleInfoMenu
{
    u8 tabsWindowId;
    u8 contentWindowId;
    u8 abilityWindowId;
    u8 summarySpriteId;
    u8 hpBarSpriteId;
    u8 statusIconSpriteId;
    u8 summarySpriteSlot;
    u8 summarySpriteLoadDelay;
    u8 enemySlotSpriteIds[PARTY_SIZE];
    bool8 enemySlotIsBall[PARTY_SIZE];
    s16 enemySlotBaseY[PARTY_SIZE];
    bool8 iconsCreated;
    bool8 ballGfxLoaded;
    u8 typeIconSpriteIds[BINFO_TYPE_ICON_COUNT];
    bool8 typeIconsCreated;
    bool8 typeIconsGfxLoaded;
    u8 view;
    u8 detailPage;
    u8 abilityMode;
    u8 abilityIndex;
    u8 detailSlot;
    u8 fieldIconSpriteIds[MAX_BATTLERS_COUNT];
    u8 fieldHpBarSpriteIds[MAX_BATTLERS_COUNT];
    u8 fieldStatusIconSpriteIds[MAX_BATTLERS_COUNT];
    u8 fieldStatIndicatorSpriteIds[MAX_BATTLERS_COUNT][BINFO_FIELD_STAT_SPRITE_COUNT];
    bool8 fieldIconsCreated;
    bool8 fieldHpBarsCreated;
    bool8 fieldStatusIconsCreated;
    bool8 fieldStatIndicatorsCreated;
};

static struct BattleInfoMenu *GetStructPtr(u8 taskId);
static void UNUSED SetStructPtr(u8 taskId, void *ptr);
static void CB2_BattleInfoMenuMain(void);
static void CB2_BattleInfoMenuExit(void);
static void VBlankCB(void);
static void BattleInfo_InitTabsOnly(struct BattleInfoMenu *data);
static void BattleInfo_DestroyTabsOnly(struct BattleInfoMenu *data);
static void UNUSED Task_BattleInfoFadeIn(u8 taskId);
static void Task_BattleInfoProcessInput(u8 taskId);
static void Task_BattleInfoFadeOut(u8 taskId);
static void UNUSED BattleInfo_InitMenu(struct BattleInfoMenu *data);
static void BattleInfo_DestroyMenu(struct BattleInfoMenu *data);
static void BattleInfo_DrawTabs(struct BattleInfoMenu *data);
static void BattleInfo_ShowCurrentTab(struct BattleInfoMenu *data);
static void BattleInfo_DrawEnemySummary(struct BattleInfoMenu *data);
static void BattleInfo_DrawEnemyDetails(struct BattleInfoMenu *data);
static void BattleInfo_DrawFieldInfo(struct BattleInfoMenu *data);
static void BattleInfo_PrintText(u8 windowId, u8 fontId, const u8 *str, u8 x, u8 y, u8 speed, void (*callback)(struct TextPrinterTemplate *, u16));
static void BattleInfo_PrintTextColored(u8 windowId, u8 fontId, const u8 *str, u8 x, u8 y, const u8 *colors);
static void BattleInfo_EnsureEnemyIcons(struct BattleInfoMenu *data);
static void BattleInfo_EnsureHpBarSprite(struct BattleInfoMenu *data);
static void BattleInfo_EnsureStatusIconSprite(struct BattleInfoMenu *data);
static void BattleInfo_EnsureTypeIcons(struct BattleInfoMenu *data);
static void BattleInfo_SetEnemyIconsVisible(struct BattleInfoMenu *data, bool8 visible);
static void BattleInfo_SetHpBarVisible(struct BattleInfoMenu *data, bool8 visible);
static void BattleInfo_SetStatusIconVisible(struct BattleInfoMenu *data, bool8 visible);
static void BattleInfo_SetTypeIconsInvisible(struct BattleInfoMenu *data);
static void BattleInfo_UpdateEnemySelectionSprite(struct BattleInfoMenu *data);
static void BattleInfo_DestroySummarySprite(struct BattleInfoMenu *data);
static void BattleInfo_DestroyHpBarSprite(struct BattleInfoMenu *data);
static void BattleInfo_DestroyStatusIconSprite(struct BattleInfoMenu *data);
static void BattleInfo_DestroyTypeIcons(struct BattleInfoMenu *data);
static void BattleInfo_DestroyFieldIcons(struct BattleInfoMenu *data);
static void BattleInfo_DestroyFieldHpBars(struct BattleInfoMenu *data);
static void BattleInfo_DestroyFieldStatusIcons(struct BattleInfoMenu *data);
static void BattleInfo_DestroyFieldStatIndicators(struct BattleInfoMenu *data);
static void BattleInfo_FreeStatIndicatorGfx(void);
static void BattleInfo_SyncActiveOpponents(void);
static void BattleInfo_OpenAbilityPopup(struct BattleInfoMenu *data, enum Ability ability);
static void BattleInfo_CloseAbilityPopup(struct BattleInfoMenu *data);
static void BattleInfo_MoveEnemySlot(s8 delta, u8 maxCount);
static u8 BattleInfo_GetEnemyPartyCount(void);
static u8 BattleInfo_GetEnemyPartySlotByDisplaySlot(u8 displaySlot);
static bool32 BattleInfo_IsEnemySlotSeen(u8 slot);
static u8 BattleInfo_GetGenderDisplay(u8 battlerGender);
static void BattleInfo_FormatBarOnly(u8 *dst, u8 hpFraction, u8 segments);
static void BattleInfo_FormatStatus(u8 *dst, u32 status1);
static u8 BattleInfo_GetAbilityList(u16 species, enum Ability *abilities);
static enum Ability BattleInfo_GetConfirmedAbility(u8 slot, const enum Ability *abilities, u8 abilityCount);
static void BattleInfo_PrintEnemyHeader(struct BattleInfoMenu *data, u8 *y, u8 slot);
static u8 BattleInfo_GetAilmentFromStatus(const struct BattleInfoMon *info);
static u8 BattleInfo_GetAilmentFromStatus1(u32 status1);
static void BattleInfo_DrawStatusIconSprite(struct BattleInfoMenu *data, const struct BattleInfoMon *info, s16 x, s16 y);
static void UNUSED BattleInfo_PrintActiveBattler(u8 windowId, u8 *y, u32 battler);
static void BattleInfo_DrawHpBarSprite(struct BattleInfoMenu *data, s16 x, s16 y, u8 hpFraction);
static void BattleInfo_UpdateHpBarTiles(u8 spriteId, u8 hpFraction);
static void BattleInfo_DrawTypeLineAndIcons(struct BattleInfoMenu *data, u16 species, s16 x, s16 y);
static void BattleInfo_DrawTypeLineAndIconsUnknown(struct BattleInfoMenu *data, s16 x, s16 y);
static void BattleInfo_FormatStageLine(u8 *dst, u32 battler);
static void BattleInfo_FormatVolatileLine(u8 *dst, u32 battler);
static u8 BattleInfo_CollectFieldEffects(u8 lines[BINFO_FIELD_EFFECT_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN], u32 battler);
static void BattleInfo_DrawFieldActive(struct BattleInfoMenu *data);
static void BattleInfo_DrawFieldBattlerSection(struct BattleInfoMenu *data, u32 battler, s16 x, s16 y);
static void BattleInfo_DrawFieldEffects(struct BattleInfoMenu *data);
static bool8 BattleInfo_IsWeatherActive(void);
static bool8 BattleInfo_IsTerrainActive(void);
static u8 BattleInfo_CollectGlobalEffects(u8 lines[BINFO_FIELD_PAGE_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN]);
static u8 BattleInfo_CollectHazardLines(u8 side, u8 lines[BINFO_FIELD_PAGE_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN]);
static u8 BattleInfo_CollectSideConditions(u8 side, u8 lines[BINFO_FIELD_PAGE_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN], u8 colors[BINFO_FIELD_PAGE_MAX_LINES]);
static void BattleInfo_PrintEffectLines(struct BattleInfoMenu *data,
                                        u8 lines[BINFO_FIELD_PAGE_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN],
                                        u8 count, s16 x, s16 yStart);
static void BattleInfo_PrintEffectLinesColored(struct BattleInfoMenu *data,
                                               u8 lines[BINFO_FIELD_PAGE_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN],
                                               u8 colors[BINFO_FIELD_PAGE_MAX_LINES],
                                               u8 count, s16 x, s16 yStart);
static void BattleInfo_AppendTurnCount(u8 *dst, u16 turns, bool8 showLeft);
static void BattleInfo_AppendFieldConditionTurns(u8 *dst, enum BattleInfoFieldCondition condition, u16 timer, bool8 variableDuration);
static void BattleInfo_AppendSideConditionTurns(u8 *dst, u8 side, enum BattleInfoSideCondition condition, u16 timer, bool8 variableDuration);
static u16 BattleInfo_GetCurrentTurn(void);
static u16 BattleInfo_GetTurnsPassed(u16 startTurn);
static u16 BattleInfo_GetFieldTurnsPassed(enum BattleInfoFieldCondition condition);
static u16 BattleInfo_GetSideTurnsPassed(u8 side, enum BattleInfoSideCondition condition);
static u8 BattleInfo_GetFieldConditionBattler(enum BattleInfoFieldCondition condition);
static u8 BattleInfo_GetSideConditionBattler(u8 side, enum BattleInfoSideCondition condition);
static bool8 BattleInfo_IsBattlerItemKnown(u32 battler);
static bool8 BattleInfo_IsFieldDurationKnown(enum BattleInfoFieldCondition condition, bool8 variableDuration);
static bool8 BattleInfo_IsSideDurationKnown(u8 side, enum BattleInfoSideCondition condition, bool8 variableDuration);
static void BattleInfo_BuildWeatherLine(u8 *dst);
static void BattleInfo_BuildTerrainLine(u8 *dst);
static const u8 *BattleInfo_GetGenderText(u8 gender);
static u16 BattleInfo_GetKeys(void);
static u8 BattleInfo_DrawFieldTitle(struct BattleInfoMenu *data, const u8 *title);
static u8 BattleInfo_CreateHpBarSprite(void);
static u8 BattleInfo_CreateFieldStatusIconSprite(void);
static void BattleInfo_EnsureStatIndicatorGfx(void);
static u8 BattleInfo_CreateStatStageSprite(u8 stageIndex, u16 paletteTag);
static u8 BattleInfo_CreateStatIconSprite(u8 statIndex, u16 paletteTag);

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0,
    },
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0,
    },
};

static const u16 sBgColor[] = {RGB_WHITE};

#define BINFO_TABS_BASE_BLOCK 1
#define BINFO_CONTENT_BASE_BLOCK (BINFO_TABS_BASE_BLOCK + 28 * 3)
#define BINFO_ABILITY_BASE_BLOCK (BINFO_CONTENT_BASE_BLOCK + 28 * 18)

static const struct WindowTemplate sTabsWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 0,
    .width = 28,
    .height = 2,
    .paletteNum = 0xF,
    .baseBlock = BINFO_TABS_BASE_BLOCK,
};

static const struct WindowTemplate sContentWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 2,
    .width = 28,
    .height = 15,
    .paletteNum = 0xF,
    .baseBlock = BINFO_CONTENT_BASE_BLOCK,
};

static const struct WindowTemplate sAbilityWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 8,
    .width = 28,
    .height = 6,
    .paletteNum = 0xF,
    .baseBlock = BINFO_ABILITY_BASE_BLOCK,
};

static const u8 sText_EnemyInfo[] = _("Enemy Info");
static const u8 sText_FieldInfo[] = _("Field Info");
static const u8 sText_UnknownPokemon[] = _("Unknown Pokemon");
static const u8 sText_UnknownItem[] = _("Unknown");
static const u8 sText_StatusOK[] = _("OK");
static const u8 sText_StatusSLP[] = _("SLP");
static const u8 sText_StatusPSN[] = _("PSN");
static const u8 sText_StatusBRN[] = _("BRN");
static const u8 sText_StatusFRZ[] = _("FRZ");
static const u8 sText_StatusPAR[] = _("PAR");
static const u8 sText_StatusTOX[] = _("TOX");
static const u8 sText_StatusFRB[] = _("FRB");
static const u8 sText_AbilityHeader[] = _("Abilities:");
static const u8 sText_ItemHeader[] = _("Item: ");
static const u8 sText_AbilityLine[] = _("Ability: ");
static const u8 sText_SpeciesLine[] = _("Species: ");
static const u8 sText_TypeLine[] = _("Type:");
static const u8 sText_NotRevealedYet[] = _("Not revealed yet");
static const u8 sBattleInfoTextColors_Black[] = {TEXT_COLOR_TRANSPARENT, 1, 3};
static const u8 sBattleInfoTextColors_Male[] = {TEXT_COLOR_TRANSPARENT, 5, 6};
static const u8 sBattleInfoTextColors_Female[] = {TEXT_COLOR_TRANSPARENT, 7, 8};
static const u8 sBattleInfoTextColors_Green[] = {TEXT_COLOR_TRANSPARENT, 9, 10};
static const u8 sBattleInfoTextColors_Red[] = {TEXT_COLOR_TRANSPARENT, 11, 12};
static const u8 sText_PageInfo[] = _("Page: Info");
static const u8 sText_PageMoves[] = _("Page: Moves");
static const u8 sText_KnownMoves[] = _("Known Moves");
static const u8 sText_A_Details[] = _("A: Details");
static const u8 sText_FieldActiveHeader[] = _("Active Battlers");
static const u8 sText_FieldEffectsHeader[] = _("Field Effects");
static const u8 sText_FieldConditionsHeader[] = _("Field Conditions");
static const u8 sText_SideConditionsHeader[] = _("Side Conditions");
static const u8 sText_AllySideHeader[] = _("Ally Side");
static const u8 sText_OpponentSideHeader[] = _("Opponent Side");
static const u8 sText_FieldNoEffects[] = _("No field or side conditions");
static const u8 sText_EffectConfused[] = _("Confused");
static const u8 sText_EffectInfatuated[] = _("Infatuated");
static const u8 sText_EffectTypePrefix[] = _("Type: ");
static const u8 sText_EffectTypeAddPrefix[] = _("Type+: ");
static const u8 sText_EffectLeechSeed[] = _("Leech Seed");
static const u8 sText_EffectPerish[] = _("Perish: ");
static const u8 sText_StagesPrefix[] = _("Stg: ");
static const u8 sText_VolatilePrefix[] = _("Vol: ");
static const u8 sText_None[] = _("-");
static const u8 sText_WeatherPrefix[] = _("Weather: ");
static const u8 sText_TerrainPrefix[] = _("Terrain: ");
static const u8 sText_RoomTrickRoom[] = _("Trick Room:");
static const u8 sText_RoomGravity[] = _("Gravity:");
static const u8 sText_RoomMagicRoom[] = _("Magic Room:");
static const u8 sText_RoomWonderRoom[] = _("Wonder Room:");
static const u8 sText_RoomMudSport[] = _("Mud Sport:");
static const u8 sText_RoomWaterSport[] = _("Water Sport:");
static const u8 sText_RoomFairyLock[] = _("Fairy Lock:");
static const u8 sText_HazardNone[] = _("None");
static const u8 sText_HazardStealthRock[] = _("Stealth Rock");
static const u8 sText_HazardSpikesPrefix[] = _("Spikes: ");
static const u8 sText_HazardToxicSpikesPrefix[] = _("Toxic Spikes: ");
static const u8 sText_HazardStickyWeb[] = _("Sticky Web");
static const u8 sText_HazardSteelSurge[] = _("Steelsurge");
static const u8 sText_SideReflect[] = _("Reflect");
static const u8 sText_SideLightScreen[] = _("Light Screen");
static const u8 sText_SideAuroraVeil[] = _("Aurora Veil");
static const u8 sText_SideTailwind[] = _("Tailwind");
static const u8 sText_SideSafeguard[] = _("Safeguard");
static const u8 sText_SideMist[] = _("Mist");
static const u8 sText_SideLuckyChant[] = _("Lucky Chant");
static const u8 sText_MoveSlotUnknown[] = _("---");
static const u8 sText_P1[] = _("P1");
static const u8 sText_P2[] = _("P2");
static const u8 sText_O1[] = _("O1");
static const u8 sText_O2[] = _("O2");
static const u8 sText_StageAtk[] = _("A");
static const u8 sText_StageDef[] = _("D");
static const u8 sText_StageSpAtk[] = _("SA");
static const u8 sText_StageSpDef[] = _("SD");
static const u8 sText_StageSpe[] = _("S");
static const u8 sText_StageAcc[] = _("Ac");
static const u8 sText_StageEva[] = _("Ev");
static const u8 sText_WeatherNone[] = _("None");
static const u8 sText_WeatherRain[] = _("Rain");
static const u8 sText_WeatherSun[] = _("Sun");
static const u8 sText_WeatherSand[] = _("Sand");
static const u8 sText_WeatherHail[] = _("Hail");
static const u8 sText_WeatherSnow[] = _("Snow");
static const u8 sText_WeatherFog[] = _("Fog");
static const u8 sText_WeatherHarshSun[] = _("Harsh Sun");
static const u8 sText_WeatherHeavyRain[] = _("Heavy Rain");
static const u8 sText_WeatherStrongWinds[] = _("Strong Winds");
static const u8 sText_TerrainNone[] = _("None");
static const u8 sText_TerrainGrassy[] = _("Grassy");
static const u8 sText_TerrainMisty[] = _("Misty");
static const u8 sText_TerrainElectric[] = _("Electric");
static const u8 sText_TerrainPsychic[] = _("Psychic");
static const u8 sText_Conf[] = _("Conf");
static const u8 sText_Leech[] = _("Leech");
static const u8 sText_Per[] = _("Per");

static const union AnimCmd sBattleInfoBallAnimSeq[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END,
};

static const union AnimCmd *const sBattleInfoBallAnimTable[] =
{
    sBattleInfoBallAnimSeq,
};

static const struct CompressedSpriteSheet sBattleInfoBallSpriteSheet =
{
    gBallGfx_Poke,
    0x180,
    BINFO_BALL_TILE_TAG,
};

static const struct SpritePalette sBattleInfoBallSpritePalette =
{
    gBallPal_Poke,
    BINFO_BALL_PAL_TAG,
};

static const struct OamData sBattleInfoBallOamData =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct SpriteTemplate sBattleInfoBallSpriteTemplate =
{
    .tileTag = BINFO_BALL_TILE_TAG,
    .paletteTag = BINFO_BALL_PAL_TAG,
    .oam = &sBattleInfoBallOamData,
    .anims = sBattleInfoBallAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const u8 sBattleInfoHpBarGfx[TILE_SIZE_4BPP * BINFO_HP_BAR_TILES] = {0};

static const struct SpriteSheet sBattleInfoHpBarSpriteSheet =
{
    sBattleInfoHpBarGfx,
    sizeof(sBattleInfoHpBarGfx),
    BINFO_HP_BAR_TILE_TAG,
};

static const struct SpritePalette sBattleInfoHpBarSpritePalette =
{
    gBattleInterface_BallDisplayPal,
    BINFO_HP_BAR_PAL_TAG,
};

static const struct OamData sBattleInfoHpBarOamData =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct Subsprite sBattleInfoHpBarSubsprites[] =
{
    {
        .x = -16,
        .y = 0,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 0,
        .priority = 0,
    },
    {
        .x = 16,
        .y = 0,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 4,
        .priority = 0,
    },
};

static const struct SubspriteTable sBattleInfoHpBarSubspriteTable[] =
{
    {
        .subspriteCount = ARRAY_COUNT(sBattleInfoHpBarSubsprites),
        .subsprites = sBattleInfoHpBarSubsprites,
    },
};

static const struct SpriteTemplate sBattleInfoHpBarSpriteTemplate =
{
    .tileTag = BINFO_HP_BAR_TILE_TAG,
    .paletteTag = BINFO_HP_BAR_PAL_TAG,
    .oam = &sBattleInfoHpBarOamData,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct OamData sBattleInfoStatusIconOamData =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sBattleInfoStatusAnim_Poison[] = {
    ANIMCMD_FRAME(0, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sBattleInfoStatusAnim_Paralyzed[] = {
    ANIMCMD_FRAME(4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sBattleInfoStatusAnim_Sleep[] = {
    ANIMCMD_FRAME(8, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sBattleInfoStatusAnim_Frozen[] = {
    ANIMCMD_FRAME(12, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sBattleInfoStatusAnim_Burn[] = {
    ANIMCMD_FRAME(16, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sBattleInfoStatusAnim_Pokerus[] = {
    ANIMCMD_FRAME(20, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sBattleInfoStatusAnim_Faint[] = {
    ANIMCMD_FRAME(24, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sBattleInfoStatusAnim_Frostbite[] = {
    ANIMCMD_FRAME(28, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd *const sBattleInfoStatusAnimTable[] = {
    sBattleInfoStatusAnim_Poison,
    sBattleInfoStatusAnim_Paralyzed,
    sBattleInfoStatusAnim_Sleep,
    sBattleInfoStatusAnim_Frozen,
    sBattleInfoStatusAnim_Burn,
    sBattleInfoStatusAnim_Pokerus,
    sBattleInfoStatusAnim_Faint,
    sBattleInfoStatusAnim_Frostbite,
};

static const struct CompressedSpriteSheet sBattleInfoStatusIconsSpriteSheet =
{
    .data = gStatusGfx_Icons,
    .size = 0x400,
    .tag = BINFO_STATUS_ICON_TILE_TAG,
};

static const struct SpritePalette sBattleInfoStatusIconsSpritePalette =
{
    .data = gStatusPal_Icons,
    .tag = BINFO_STATUS_ICON_PAL_TAG,
};

static const struct SpriteTemplate sBattleInfoStatusIconSpriteTemplate =
{
    .tileTag = BINFO_STATUS_ICON_TILE_TAG,
    .paletteTag = BINFO_STATUS_ICON_PAL_TAG,
    .oam = &sBattleInfoStatusIconOamData,
    .anims = sBattleInfoStatusAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const u8 sBattleInfoStatStagePlus1Gfx[] = INCBIN_U8("graphics/battle_interface/plus_one_indicator.4bpp");
static const u8 sBattleInfoStatStagePlus2Gfx[] = INCBIN_U8("graphics/battle_interface/plus_two_indicator.4bpp");
static const u8 sBattleInfoStatStagePlus3Gfx[] = INCBIN_U8("graphics/battle_interface/plus_three_indicator.4bpp");
static const u8 sBattleInfoStatStagePlus4Gfx[] = INCBIN_U8("graphics/battle_interface/plus_four_indicator.4bpp");
static const u8 sBattleInfoStatStagePlus5Gfx[] = INCBIN_U8("graphics/battle_interface/plus_five_indicator.4bpp");
static const u8 sBattleInfoStatStagePlus6Gfx[] = INCBIN_U8("graphics/battle_interface/plus_six_indicator.4bpp");
static const u8 sBattleInfoStatStageMinus1Gfx[] = INCBIN_U8("graphics/battle_interface/minus_one_indicator.4bpp");
static const u8 sBattleInfoStatStageMinus2Gfx[] = INCBIN_U8("graphics/battle_interface/minus_two_indicator.4bpp");
static const u8 sBattleInfoStatStageMinus3Gfx[] = INCBIN_U8("graphics/battle_interface/minus_three_indicator.4bpp");
static const u8 sBattleInfoStatStageMinus4Gfx[] = INCBIN_U8("graphics/battle_interface/minus_four_indicator.4bpp");
static const u8 sBattleInfoStatStageMinus5Gfx[] = INCBIN_U8("graphics/battle_interface/minus_five_indicator.4bpp");
static const u8 sBattleInfoStatStageMinus6Gfx[] = INCBIN_U8("graphics/battle_interface/minus_six_indicator.4bpp");

static const struct SpriteSheet sBattleInfoStatStageSpriteSheets[] =
{
    {sBattleInfoStatStagePlus1Gfx, sizeof(sBattleInfoStatStagePlus1Gfx), BINFO_STAT_STAGE_TILE_TAG_BASE + 0},
    {sBattleInfoStatStagePlus2Gfx, sizeof(sBattleInfoStatStagePlus2Gfx), BINFO_STAT_STAGE_TILE_TAG_BASE + 1},
    {sBattleInfoStatStagePlus3Gfx, sizeof(sBattleInfoStatStagePlus3Gfx), BINFO_STAT_STAGE_TILE_TAG_BASE + 2},
    {sBattleInfoStatStagePlus4Gfx, sizeof(sBattleInfoStatStagePlus4Gfx), BINFO_STAT_STAGE_TILE_TAG_BASE + 3},
    {sBattleInfoStatStagePlus5Gfx, sizeof(sBattleInfoStatStagePlus5Gfx), BINFO_STAT_STAGE_TILE_TAG_BASE + 4},
    {sBattleInfoStatStagePlus6Gfx, sizeof(sBattleInfoStatStagePlus6Gfx), BINFO_STAT_STAGE_TILE_TAG_BASE + 5},
    {sBattleInfoStatStageMinus1Gfx, sizeof(sBattleInfoStatStageMinus1Gfx), BINFO_STAT_STAGE_TILE_TAG_BASE + 6},
    {sBattleInfoStatStageMinus2Gfx, sizeof(sBattleInfoStatStageMinus2Gfx), BINFO_STAT_STAGE_TILE_TAG_BASE + 7},
    {sBattleInfoStatStageMinus3Gfx, sizeof(sBattleInfoStatStageMinus3Gfx), BINFO_STAT_STAGE_TILE_TAG_BASE + 8},
    {sBattleInfoStatStageMinus4Gfx, sizeof(sBattleInfoStatStageMinus4Gfx), BINFO_STAT_STAGE_TILE_TAG_BASE + 9},
    {sBattleInfoStatStageMinus5Gfx, sizeof(sBattleInfoStatStageMinus5Gfx), BINFO_STAT_STAGE_TILE_TAG_BASE + 10},
    {sBattleInfoStatStageMinus6Gfx, sizeof(sBattleInfoStatStageMinus6Gfx), BINFO_STAT_STAGE_TILE_TAG_BASE + 11},
    {},
};

static const u8 sBattleInfoStatIconAtkGfx[] = INCBIN_U8("graphics/battle_interface/stat_atk_indicator.4bpp");
static const u8 sBattleInfoStatIconDefGfx[] = INCBIN_U8("graphics/battle_interface/stat_def_indicator.4bpp");
static const u8 sBattleInfoStatIconSpAGfx[] = INCBIN_U8("graphics/battle_interface/stat_spa_indicator.4bpp");
static const u8 sBattleInfoStatIconSpDGfx[] = INCBIN_U8("graphics/battle_interface/stat_spd_indicator.4bpp");
static const u8 sBattleInfoStatIconSpeGfx[] = INCBIN_U8("graphics/battle_interface/stat_spe_indicator.4bpp");
static const u8 sBattleInfoStatIconAccGfx[] = INCBIN_U8("graphics/battle_interface/stat_acc_indicator.4bpp");
static const u8 sBattleInfoStatIconEvaGfx[] = INCBIN_U8("graphics/battle_interface/stat_eva_indicator.4bpp");

static const struct SpriteSheet sBattleInfoStatIconSpriteSheets[] =
{
    {sBattleInfoStatIconAtkGfx, sizeof(sBattleInfoStatIconAtkGfx), BINFO_STAT_ICON_TILE_TAG_BASE + 0},
    {sBattleInfoStatIconDefGfx, sizeof(sBattleInfoStatIconDefGfx), BINFO_STAT_ICON_TILE_TAG_BASE + 1},
    {sBattleInfoStatIconSpAGfx, sizeof(sBattleInfoStatIconSpAGfx), BINFO_STAT_ICON_TILE_TAG_BASE + 2},
    {sBattleInfoStatIconSpDGfx, sizeof(sBattleInfoStatIconSpDGfx), BINFO_STAT_ICON_TILE_TAG_BASE + 3},
    {sBattleInfoStatIconSpeGfx, sizeof(sBattleInfoStatIconSpeGfx), BINFO_STAT_ICON_TILE_TAG_BASE + 4},
    {sBattleInfoStatIconAccGfx, sizeof(sBattleInfoStatIconAccGfx), BINFO_STAT_ICON_TILE_TAG_BASE + 5},
    {sBattleInfoStatIconEvaGfx, sizeof(sBattleInfoStatIconEvaGfx), BINFO_STAT_ICON_TILE_TAG_BASE + 6},
    {},
};

static const u16 sBattleInfoStatIndicatorPlusPal[] = INCBIN_U16("graphics/battle_interface/stat_plus_indicator.gbapal");
static const u16 sBattleInfoStatIndicatorMinusPal[] = INCBIN_U16("graphics/battle_interface/stat_minus_indicator.gbapal");

static const struct SpritePalette sBattleInfoStatIndicatorPlusPalette =
{
    .data = sBattleInfoStatIndicatorPlusPal,
    .tag = BINFO_STAT_INDICATOR_PLUS_PAL_TAG,
};

static const struct SpritePalette sBattleInfoStatIndicatorMinusPalette =
{
    .data = sBattleInfoStatIndicatorMinusPal,
    .tag = BINFO_STAT_INDICATOR_MINUS_PAL_TAG,
};

static const struct OamData sBattleInfoStatStageOamData =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(8x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct OamData sBattleInfoStatIconOamData =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct SpriteTemplate sBattleInfoStatStageSpriteTemplate =
{
    .tileTag = BINFO_STAT_STAGE_TILE_TAG_BASE,
    .paletteTag = BINFO_STAT_INDICATOR_PLUS_PAL_TAG,
    .oam = &sBattleInfoStatStageOamData,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sBattleInfoStatIconSpriteTemplate =
{
    .tileTag = BINFO_STAT_ICON_TILE_TAG_BASE,
    .paletteTag = BINFO_STAT_INDICATOR_PLUS_PAL_TAG,
    .oam = &sBattleInfoStatIconOamData,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};


static EWRAM_DATA struct BattleInfoMenu *sBattleInfoMenu = NULL;

static struct BattleInfoMenu *GetStructPtr(u8 taskId)
{
    u8 *taskDataPtr = (u8 *)(&gTasks[taskId].data[0]);

    return (struct BattleInfoMenu *)(T1_READ_PTR(taskDataPtr));
}

static void UNUSED SetStructPtr(u8 taskId, void *ptr)
{
    u32 structPtr = (u32)(ptr);
    u8 *taskDataPtr = (u8 *)(&gTasks[taskId].data[0]);

    taskDataPtr[0] = structPtr >> 0;
    taskDataPtr[1] = structPtr >> 8;
    taskDataPtr[2] = structPtr >> 16;
    taskDataPtr[3] = structPtr >> 24;
}

static u16 BattleInfo_GetKeys(void)
{
    if (gMain.newKeys != 0)
        return gMain.newKeys;
    return gMain.newAndRepeatedKeys;
}

static void BattleInfo_PrintText(u8 windowId, u8 fontId, const u8 *str, u8 x, u8 y, u8 speed, void (*callback)(struct TextPrinterTemplate *, u16))
{
    AddTextPrinterParameterized4(windowId, fontId, x, y, 0, 0, sBattleInfoTextColors_Black, speed, str);
    (void)callback;
}

static void BattleInfo_PrintTextColored(u8 windowId, u8 fontId, const u8 *str, u8 x, u8 y, const u8 *colors)
{
    AddTextPrinterParameterized4(windowId, fontId, x, y, 0, 0, colors, 0, str);
}

static void CB2_BattleInfoMenuMain(void)
{
    u16 keys = BattleInfo_GetKeys();

    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();

    if (!gPaletteFade.active && sBattleInfoMenu != NULL
        && gBattleStruct->battleInfo.tab == BINFO_TAB_ENEMY
        && sBattleInfoMenu->view == ENEMY_VIEW_SUMMARY
        && sBattleInfoMenu->summarySpriteLoadDelay != 0)
    {
        sBattleInfoMenu->summarySpriteLoadDelay--;
        if (sBattleInfoMenu->summarySpriteLoadDelay == 0)
            BattleInfo_DrawEnemySummary(sBattleInfoMenu);
    }

    if (!gPaletteFade.active && sBattleInfoMenu != NULL && (keys & (L_BUTTON | R_BUTTON)))
    {
        gBattleStruct->battleInfo.tab ^= 1;
        PlaySE(SE_SELECT);
        BattleInfo_DrawTabs(sBattleInfoMenu);
        BattleInfo_ShowCurrentTab(sBattleInfoMenu);
    }

    if (!gPaletteFade.active && sBattleInfoMenu != NULL && gBattleStruct->battleInfo.tab == BINFO_TAB_ENEMY)
    {
        u8 count = BattleInfo_GetEnemyPartyCount();

        if (keys & DPAD_LEFT)
        {
            BattleInfo_MoveEnemySlot(-1, count);
            BattleInfo_UpdateEnemySelectionSprite(sBattleInfoMenu);
            if (sBattleInfoMenu->contentWindowId != WINDOW_NONE)
                BattleInfo_DrawEnemySummary(sBattleInfoMenu);
        }
        else if (keys & DPAD_RIGHT)
        {
            BattleInfo_MoveEnemySlot(1, count);
            BattleInfo_UpdateEnemySelectionSprite(sBattleInfoMenu);
            if (sBattleInfoMenu->contentWindowId != WINDOW_NONE)
                BattleInfo_DrawEnemySummary(sBattleInfoMenu);
        }
    }

    if (!gPaletteFade.active && sBattleInfoMenu != NULL && gBattleStruct->battleInfo.tab == BINFO_TAB_FIELD)
    {
        if (keys & DPAD_LEFT)
        {
            if (gBattleStruct->battleInfo.fieldPage == 0)
                gBattleStruct->battleInfo.fieldPage = FIELD_PAGE_COUNT - 1;
            else
                gBattleStruct->battleInfo.fieldPage--;
            BattleInfo_DrawFieldInfo(sBattleInfoMenu);
        }
        else if (keys & DPAD_RIGHT)
        {
            gBattleStruct->battleInfo.fieldPage = (gBattleStruct->battleInfo.fieldPage + 1) % FIELD_PAGE_COUNT;
            BattleInfo_DrawFieldInfo(sBattleInfoMenu);
        }
    }

    if (!gPaletteFade.active && (keys & B_BUTTON))
    {
        BeginNormalPaletteFade(-1, 0, 0, 0x10, 0);
        SetMainCallback2(CB2_BattleInfoMenuExit);
    }
}

static void CB2_BattleInfoMenuExit(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();

    if (!gPaletteFade.active)
    {
        if (sBattleInfoMenu != NULL)
        {
            BattleInfo_DestroyTabsOnly(sBattleInfoMenu);
            Free(sBattleInfoMenu);
            sBattleInfoMenu = NULL;
        }
        FreeAllWindowBuffers();
        SetMainCallback2(ReshowBattleScreenAfterMenu);
    }
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void CB2_BattleInfoMenu(void)
{
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        gMain.state++;
        break;
    case 1:
        ResetVramOamAndBgCntRegs();
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        ResetAllBgsCoordinates();
        FreeAllWindowBuffers();
        FillBgTilemapBufferRect(0, 0, 0, 0, 32, 32, 0);
        FillBgTilemapBufferRect(1, 0, 0, 0, 32, 32, 0);
        CopyBgTilemapBufferToVram(0);
        CopyBgTilemapBufferToVram(1);
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        HideBg(1);
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetAllPicSprites();
        gMain.state++;
        break;
    case 3:
        LoadPalette(sBgColor, 0, 2);
        LoadPalette(gBattleInfoTextPal, 0xf0, PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 4:
        sBattleInfoMenu = AllocZeroed(sizeof(*sBattleInfoMenu));
        BattleInfo_InitTabsOnly(sBattleInfoMenu);
        BeginNormalPaletteFade(-1, 0, 0x10, 0, 0);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(CB2_BattleInfoMenuMain);
        return;
    }
}

static void UNUSED BattleInfo_InitMenu(struct BattleInfoMenu *data)
{
    data->tabsWindowId = AddWindow(&sTabsWindowTemplate);
    data->contentWindowId = AddWindow(&sContentWindowTemplate);
    data->abilityWindowId = WINDOW_NONE;
    data->summarySpriteId = 0xFF;
    data->hpBarSpriteId = MAX_SPRITES;
    data->statusIconSpriteId = MAX_SPRITES;
    data->summarySpriteSlot = 0xFF;
    data->summarySpriteLoadDelay = 0;
    data->view = ENEMY_VIEW_SUMMARY;
    data->detailPage = DETAILS_PAGE_INFO;
    data->abilityMode = ABILITY_MODE_NONE;
    data->abilityIndex = 0;
    data->iconsCreated = FALSE;
    data->ballGfxLoaded = FALSE;
    data->typeIconsCreated = FALSE;
    data->typeIconsGfxLoaded = FALSE;
    data->typeIconSpriteIds[0] = MAX_SPRITES;
    data->typeIconSpriteIds[1] = MAX_SPRITES;
    data->fieldIconsCreated = FALSE;
    data->fieldHpBarsCreated = FALSE;
    data->fieldStatusIconsCreated = FALSE;
    data->fieldStatIndicatorsCreated = FALSE;
    for (u32 i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        data->fieldIconSpriteIds[i] = MAX_SPRITES;
        data->fieldHpBarSpriteIds[i] = MAX_SPRITES;
        data->fieldStatusIconSpriteIds[i] = MAX_SPRITES;
        for (u32 j = 0; j < BINFO_FIELD_STAT_SPRITE_COUNT; j++)
            data->fieldStatIndicatorSpriteIds[i][j] = MAX_SPRITES;
    }
    if (gBattleStruct->battleInfo.tab >= BINFO_TAB_FIELD)
        gBattleStruct->battleInfo.tab = BINFO_TAB_ENEMY;
    if (gBattleStruct->battleInfo.fieldPage >= FIELD_PAGE_COUNT)
        gBattleStruct->battleInfo.fieldPage = FIELD_PAGE_ACTIVE;
    if (gBattleStruct->battleInfo.enemySlot >= BattleInfo_GetEnemyPartyCount())
        gBattleStruct->battleInfo.enemySlot = 0;

    PutWindowTilemap(data->tabsWindowId);
    PutWindowTilemap(data->contentWindowId);
    BattleInfo_DrawTabs(data);
    BattleInfo_ShowCurrentTab(data);
}

static void BattleInfo_InitTabsOnly(struct BattleInfoMenu *data)
{
    if (gBattleStruct->battleInfo.tab >= BINFO_TAB_FIELD)
        gBattleStruct->battleInfo.tab = BINFO_TAB_ENEMY;
    if (gBattleStruct->battleInfo.fieldPage >= FIELD_PAGE_COUNT)
        gBattleStruct->battleInfo.fieldPage = FIELD_PAGE_ACTIVE;
    if (gBattleStruct->battleInfo.enemySlot >= BattleInfo_GetEnemyPartyCount())
        gBattleStruct->battleInfo.enemySlot = 0;

    data->tabsWindowId = AddWindow(&sTabsWindowTemplate);
    data->contentWindowId = AddWindow(&sContentWindowTemplate);
    data->abilityWindowId = WINDOW_NONE;
    data->summarySpriteId = 0xFF;
    data->hpBarSpriteId = MAX_SPRITES;
    data->statusIconSpriteId = MAX_SPRITES;
    data->summarySpriteSlot = 0xFF;
    data->summarySpriteLoadDelay = 0;
    data->iconsCreated = FALSE;
    data->ballGfxLoaded = FALSE;
    data->typeIconsCreated = FALSE;
    data->typeIconsGfxLoaded = FALSE;
    data->typeIconSpriteIds[0] = MAX_SPRITES;
    data->typeIconSpriteIds[1] = MAX_SPRITES;
    data->fieldIconsCreated = FALSE;
    data->fieldHpBarsCreated = FALSE;
    data->fieldStatusIconsCreated = FALSE;
    data->fieldStatIndicatorsCreated = FALSE;
    for (u32 i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        data->fieldIconSpriteIds[i] = MAX_SPRITES;
        data->fieldHpBarSpriteIds[i] = MAX_SPRITES;
        data->fieldStatusIconSpriteIds[i] = MAX_SPRITES;
        for (u32 j = 0; j < BINFO_FIELD_STAT_SPRITE_COUNT; j++)
            data->fieldStatIndicatorSpriteIds[i][j] = MAX_SPRITES;
    }
    PutWindowTilemap(data->tabsWindowId);
    PutWindowTilemap(data->contentWindowId);
    BattleInfo_DrawTabs(data);
    BattleInfo_ShowCurrentTab(data);
}

static void BattleInfo_DestroyMenu(struct BattleInfoMenu *data)
{
    u32 i;
    bool8 freeMonIconPalettes = data->iconsCreated || data->fieldIconsCreated;

    BattleInfo_CloseAbilityPopup(data);
    BattleInfo_DestroySummarySprite(data);
    BattleInfo_DestroyFieldStatIndicators(data);
    BattleInfo_DestroyFieldStatusIcons(data);
    BattleInfo_DestroyFieldHpBars(data);
    BattleInfo_DestroyHpBarSprite(data);
    BattleInfo_DestroyStatusIconSprite(data);
    BattleInfo_DestroyTypeIcons(data);
    BattleInfo_DestroyFieldIcons(data);
    BattleInfo_FreeStatIndicatorGfx();

    if (data->iconsCreated)
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (data->enemySlotSpriteIds[i] == 0xFF)
                continue;
            if (data->enemySlotIsBall[i])
                DestroySprite(&gSprites[data->enemySlotSpriteIds[i]]);
            else
                FreeAndDestroyMonIconSprite(&gSprites[data->enemySlotSpriteIds[i]]);
        }
        if (data->ballGfxLoaded)
        {
            FreeSpriteTilesByTag(BINFO_BALL_TILE_TAG);
            FreeSpritePaletteByTag(BINFO_BALL_PAL_TAG);
        }
    }
    if (freeMonIconPalettes)
        FreeMonIconPalettes();

    ClearStdWindowAndFrameToTransparent(data->tabsWindowId, TRUE);
    RemoveWindow(data->tabsWindowId);
    ClearStdWindowAndFrameToTransparent(data->contentWindowId, TRUE);
    RemoveWindow(data->contentWindowId);
}

static void BattleInfo_DestroyTabsOnly(struct BattleInfoMenu *data)
{
    u32 i;
    bool8 freeMonIconPalettes = data->iconsCreated || data->fieldIconsCreated;

    BattleInfo_CloseAbilityPopup(data);
    BattleInfo_DestroySummarySprite(data);
    BattleInfo_DestroyFieldStatIndicators(data);
    BattleInfo_DestroyFieldStatusIcons(data);
    BattleInfo_DestroyFieldHpBars(data);
    BattleInfo_DestroyHpBarSprite(data);
    BattleInfo_DestroyStatusIconSprite(data);
    BattleInfo_DestroyTypeIcons(data);
    BattleInfo_DestroyFieldIcons(data);
    BattleInfo_FreeStatIndicatorGfx();

    if (data->iconsCreated)
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (data->enemySlotSpriteIds[i] == 0xFF)
                continue;
            if (data->enemySlotIsBall[i])
                DestroySprite(&gSprites[data->enemySlotSpriteIds[i]]);
            else
                FreeAndDestroyMonIconSprite(&gSprites[data->enemySlotSpriteIds[i]]);
        }
        if (data->ballGfxLoaded)
        {
            FreeSpriteTilesByTag(BINFO_BALL_TILE_TAG);
            FreeSpritePaletteByTag(BINFO_BALL_PAL_TAG);
        }
    }
    if (freeMonIconPalettes)
        FreeMonIconPalettes();

    ClearStdWindowAndFrameToTransparent(data->tabsWindowId, TRUE);
    RemoveWindow(data->tabsWindowId);
    if (data->contentWindowId != WINDOW_NONE)
    {
        ClearStdWindowAndFrameToTransparent(data->contentWindowId, TRUE);
        RemoveWindow(data->contentWindowId);
    }
}

static void UNUSED Task_BattleInfoFadeIn(u8 taskId)
{
    if (!gPaletteFade.active || gTasks[taskId].data[2]++ >= BINFO_FADEIN_TIMEOUT)
        gTasks[taskId].func = Task_BattleInfoProcessInput;
}

static void Task_BattleInfoProcessInput(u8 taskId)
{
    struct BattleInfoMenu *data = GetStructPtr(taskId);
    u16 keys = BattleInfo_GetKeys();

    if (keys & B_BUTTON)
    {
        BeginNormalPaletteFade(-1, 0, 0, 0x10, 0);
        gTasks[taskId].func = Task_BattleInfoFadeOut;
        return;
    }

    if ((keys & L_BUTTON) || (keys & R_BUTTON))
    {
        gBattleStruct->battleInfo.tab ^= 1;
        data->view = ENEMY_VIEW_SUMMARY;
        data->detailPage = DETAILS_PAGE_INFO;
        data->abilityMode = ABILITY_MODE_NONE;
        BattleInfo_CloseAbilityPopup(data);
        BattleInfo_DrawTabs(data);
        BattleInfo_ShowCurrentTab(data);
        return;
    }

    if (gBattleStruct->battleInfo.tab == BINFO_TAB_ENEMY)
    {
        u8 count = BattleInfo_GetEnemyPartyCount();

        if (data->view == ENEMY_VIEW_SUMMARY)
        {
            if (keys & DPAD_LEFT)
            {
                BattleInfo_MoveEnemySlot(-1, count);
                BattleInfo_DrawEnemySummary(data);
            }
            else if (keys & DPAD_RIGHT)
            {
                BattleInfo_MoveEnemySlot(1, count);
                BattleInfo_DrawEnemySummary(data);
            }
            else if ((keys & A_BUTTON) && BattleInfo_IsEnemySlotSeen(BattleInfo_GetEnemyPartySlotByDisplaySlot(gBattleStruct->battleInfo.enemySlot)))
            {
                PlaySE(SE_SELECT);
                data->detailSlot = BattleInfo_GetEnemyPartySlotByDisplaySlot(gBattleStruct->battleInfo.enemySlot);
                data->view = ENEMY_VIEW_DETAILS;
                data->detailPage = DETAILS_PAGE_INFO;
                data->abilityMode = ABILITY_MODE_NONE;
                BattleInfo_DrawEnemyDetails(data);
            }
        }
        else
        {
            if (data->abilityMode == ABILITY_MODE_POPUP)
            {
                if (keys & B_BUTTON)
                {
                    BattleInfo_CloseAbilityPopup(data);
                    data->abilityMode = ABILITY_MODE_LIST;
                    BattleInfo_DrawEnemyDetails(data);
                }
                return;
            }
            if (data->abilityMode == ABILITY_MODE_LIST)
            {
                enum Ability abilities[NUM_ABILITY_SLOTS];
                u8 abilityCount = BattleInfo_GetAbilityList(gBattleStruct->battleInfo.mon[B_SIDE_OPPONENT][data->detailSlot].species, abilities);

                if (keys & B_BUTTON)
                {
                    data->abilityMode = ABILITY_MODE_NONE;
                    BattleInfo_DrawEnemyDetails(data);
                }
                else if ((keys & DPAD_UP) && abilityCount != 0)
                {
                    data->abilityIndex = (data->abilityIndex == 0) ? (abilityCount - 1) : (data->abilityIndex - 1);
                    BattleInfo_DrawEnemyDetails(data);
                }
                else if ((keys & DPAD_DOWN) && abilityCount != 0)
                {
                    data->abilityIndex = (data->abilityIndex + 1) % abilityCount;
                    BattleInfo_DrawEnemyDetails(data);
                }
                else if ((keys & A_BUTTON) && abilityCount != 0)
                {
                    BattleInfo_OpenAbilityPopup(data, abilities[data->abilityIndex]);
                }
                return;
            }

            if (keys & B_BUTTON)
            {
                data->view = ENEMY_VIEW_SUMMARY;
                data->detailPage = DETAILS_PAGE_INFO;
                BattleInfo_DrawEnemySummary(data);
            }
            else if (keys & DPAD_LEFT)
            {
                data->detailPage ^= 1;
                BattleInfo_DrawEnemyDetails(data);
            }
            else if (keys & DPAD_RIGHT)
            {
                data->detailPage ^= 1;
                BattleInfo_DrawEnemyDetails(data);
            }
            else if (keys & DPAD_UP)
            {
                BattleInfo_MoveEnemySlot(-1, count);
                data->detailSlot = BattleInfo_GetEnemyPartySlotByDisplaySlot(gBattleStruct->battleInfo.enemySlot);
                BattleInfo_DrawEnemyDetails(data);
            }
            else if (keys & DPAD_DOWN)
            {
                BattleInfo_MoveEnemySlot(1, count);
                data->detailSlot = BattleInfo_GetEnemyPartySlotByDisplaySlot(gBattleStruct->battleInfo.enemySlot);
                BattleInfo_DrawEnemyDetails(data);
            }
            else if ((keys & A_BUTTON) && BattleInfo_IsEnemySlotSeen(data->detailSlot))
            {
                data->abilityMode = ABILITY_MODE_LIST;
                data->abilityIndex = 0;
                BattleInfo_DrawEnemyDetails(data);
            }
        }
    }
    else
    {
        if (keys & DPAD_LEFT)
        {
            if (gBattleStruct->battleInfo.fieldPage == 0)
                gBattleStruct->battleInfo.fieldPage = FIELD_PAGE_COUNT - 1;
            else
                gBattleStruct->battleInfo.fieldPage--;
            BattleInfo_DrawFieldInfo(data);
        }
        else if (keys & DPAD_RIGHT)
        {
            gBattleStruct->battleInfo.fieldPage = (gBattleStruct->battleInfo.fieldPage + 1) % FIELD_PAGE_COUNT;
            BattleInfo_DrawFieldInfo(data);
        }
    }
}

static void Task_BattleInfoFadeOut(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        struct BattleInfoMenu *data = GetStructPtr(taskId);
        BattleInfo_DestroyMenu(data);
        FreeAllWindowBuffers();
        Free(data);
        DestroyTask(taskId);
        SetMainCallback2(ReshowBattleScreenAfterMenu);
    }
}

static void BattleInfo_DrawTabs(struct BattleInfoMenu *data)
{
    FillWindowPixelBuffer(data->tabsWindowId, PIXEL_FILL(0));

    if (gBattleStruct->battleInfo.tab == BINFO_TAB_ENEMY)
    {
        BattleInfo_PrintText(data->tabsWindowId, FONT_NORMAL, COMPOUND_STRING("> Enemy Info"), 1, 1, 0, NULL);
        BattleInfo_PrintText(data->tabsWindowId, FONT_NORMAL, COMPOUND_STRING("  Field Info"), 120, 1, 0, NULL);
    }
    else
    {
        BattleInfo_PrintText(data->tabsWindowId, FONT_NORMAL, COMPOUND_STRING("  Enemy Info"), 1, 1, 0, NULL);
        BattleInfo_PrintText(data->tabsWindowId, FONT_NORMAL, COMPOUND_STRING("> Field Info"), 120, 1, 0, NULL);
    }

    CopyWindowToVram(data->tabsWindowId, COPYWIN_FULL);
}

static void BattleInfo_ShowCurrentTab(struct BattleInfoMenu *data)
{
    if (gBattleStruct->battleInfo.tab == BINFO_TAB_ENEMY)
    {
        BattleInfo_DestroyFieldIcons(data);
        BattleInfo_DestroyFieldHpBars(data);
        BattleInfo_DestroyFieldStatusIcons(data);
        BattleInfo_DestroyFieldStatIndicators(data);
        data->view = ENEMY_VIEW_SUMMARY;
        BattleInfo_EnsureEnemyIcons(data);
        BattleInfo_SetEnemyIconsVisible(data, TRUE);
        BattleInfo_DrawEnemySummary(data);
    }
    else
    {
        BattleInfo_SetEnemyIconsVisible(data, FALSE);
        BattleInfo_DestroySummarySprite(data);
        BattleInfo_SetHpBarVisible(data, FALSE);
        BattleInfo_SetStatusIconVisible(data, FALSE);
        BattleInfo_SetTypeIconsInvisible(data);
        BattleInfo_DrawFieldInfo(data);
    }
}

static void BattleInfo_EnsureEnemyIcons(struct BattleInfoMenu *data)
{
    u32 i;
    u8 count;

    if (data->iconsCreated)
        return;

    BattleInfo_SyncActiveOpponents();
    count = BattleInfo_GetEnemyPartyCount();
    if (count <= 1)
        return;
    LoadMonIconPalettes();
    LoadCompressedSpriteSheetUsingHeap(&sBattleInfoBallSpriteSheet);
    LoadSpritePalette(&sBattleInfoBallSpritePalette);
    data->ballGfxLoaded = TRUE;

    for (i = 0; i < PARTY_SIZE; i++)
        data->enemySlotSpriteIds[i] = 0xFF;

    for (i = 0; i < count; i++)
    {
        s16 x = BINFO_ICON_START_X + (i * BINFO_ICON_SPACING_X);
        s16 y = BINFO_ICON_Y;
        u8 partySlot = BattleInfo_GetEnemyPartySlotByDisplaySlot(i);
        struct BattleInfoMon *info = &gBattleStruct->battleInfo.mon[B_SIDE_OPPONENT][partySlot];
        u8 spriteId;

        if (info->seen)
        {
            spriteId = CreateMonIcon(info->species, SpriteCallbackDummy, x, y, 0, info->personality);
            data->enemySlotIsBall[i] = FALSE;
        }
        else
        {
            spriteId = CreateSprite(&sBattleInfoBallSpriteTemplate, x, y + 40, 0);
            data->enemySlotIsBall[i] = TRUE;
        }
        if (spriteId != MAX_SPRITES)
        {
            data->enemySlotSpriteIds[i] = spriteId;
            data->enemySlotBaseY[i] = y;
            gSprites[spriteId].oam.priority = 0;
        }
    }

    data->iconsCreated = TRUE;
    BattleInfo_UpdateEnemySelectionSprite(data);
}

static void BattleInfo_SyncActiveOpponents(void)
{
    u32 battler;

    if (gBattleStruct == NULL)
        return;

    for (battler = 0; battler < gBattlersCount; battler++)
    {
        if (GetBattlerSide(battler) != B_SIDE_OPPONENT)
            continue;
        if (gAbsentBattlerFlags & (1u << battler))
            continue;
        if (gBattleMons[battler].species == SPECIES_NONE)
            continue;
        BattleInfo_RecordSwitchIn(battler);
    }
}

static u8 BattleInfo_CreateHpBarSprite(void)
{
    u8 spriteId;

    if (GetSpriteTileStartByTag(BINFO_HP_BAR_TILE_TAG) == 0xFFFF)
        LoadSpriteSheet(&sBattleInfoHpBarSpriteSheet);
    if (IndexOfSpritePaletteTag(BINFO_HP_BAR_PAL_TAG) == 0xFF)
        LoadSpritePalette(&sBattleInfoHpBarSpritePalette);

    spriteId = CreateSprite(&sBattleInfoHpBarSpriteTemplate, 0, 0, 0);
    if (spriteId != MAX_SPRITES)
    {
        SetSubspriteTables(&gSprites[spriteId], sBattleInfoHpBarSubspriteTable);
        gSprites[spriteId].subspriteMode = SUBSPRITES_IGNORE_PRIORITY;
        gSprites[spriteId].invisible = TRUE;
    }

    return spriteId;
}

static u8 BattleInfo_CreateFieldStatusIconSprite(void)
{
    u8 spriteId;

    if (GetSpriteTileStartByTag(BINFO_STATUS_ICON_TILE_TAG) == 0xFFFF)
        LoadCompressedSpriteSheet(&sBattleInfoStatusIconsSpriteSheet);
    if (IndexOfSpritePaletteTag(BINFO_STATUS_ICON_PAL_TAG) == 0xFF)
        LoadSpritePalette(&sBattleInfoStatusIconsSpritePalette);

    spriteId = CreateSprite(&sBattleInfoStatusIconSpriteTemplate, 0, 0, 0);
    if (spriteId != MAX_SPRITES)
    {
        gSprites[spriteId].oam.priority = 0;
        gSprites[spriteId].invisible = TRUE;
    }

    return spriteId;
}

static void BattleInfo_EnsureStatIndicatorGfx(void)
{
    u32 i;

    for (i = 0; i < BINFO_STAT_STAGE_TILE_TAG_COUNT; i++)
    {
        u16 tag = BINFO_STAT_STAGE_TILE_TAG_BASE + i;
        if (GetSpriteTileStartByTag(tag) == 0xFFFF)
            LoadSpriteSheet(&sBattleInfoStatStageSpriteSheets[i]);
    }
    for (i = 0; i < BINFO_STAT_ICON_TILE_TAG_COUNT; i++)
    {
        u16 tag = BINFO_STAT_ICON_TILE_TAG_BASE + i;
        if (GetSpriteTileStartByTag(tag) == 0xFFFF)
            LoadSpriteSheet(&sBattleInfoStatIconSpriteSheets[i]);
    }
    if (IndexOfSpritePaletteTag(BINFO_STAT_INDICATOR_PLUS_PAL_TAG) == 0xFF)
        LoadSpritePalette(&sBattleInfoStatIndicatorPlusPalette);
    if (IndexOfSpritePaletteTag(BINFO_STAT_INDICATOR_MINUS_PAL_TAG) == 0xFF)
        LoadSpritePalette(&sBattleInfoStatIndicatorMinusPalette);
}

static u8 BattleInfo_CreateStatStageSprite(u8 stageIndex, u16 paletteTag)
{
    struct SpriteTemplate template = sBattleInfoStatStageSpriteTemplate;
    u8 spriteId;

    if (stageIndex >= BINFO_STAT_STAGE_TILE_TAG_COUNT)
        return MAX_SPRITES;

    BattleInfo_EnsureStatIndicatorGfx();
    template.tileTag = BINFO_STAT_STAGE_TILE_TAG_BASE + stageIndex;
    template.paletteTag = paletteTag;
    spriteId = CreateSprite(&template, 0, 0, 0);
    if (spriteId != MAX_SPRITES)
    {
        gSprites[spriteId].oam.priority = 0;
        gSprites[spriteId].invisible = TRUE;
    }

    return spriteId;
}

static u8 BattleInfo_CreateStatIconSprite(u8 statIndex, u16 paletteTag)
{
    struct SpriteTemplate template = sBattleInfoStatIconSpriteTemplate;
    u8 spriteId;

    if (statIndex >= BINFO_STAT_ICON_TILE_TAG_COUNT)
        return MAX_SPRITES;

    BattleInfo_EnsureStatIndicatorGfx();
    template.tileTag = BINFO_STAT_ICON_TILE_TAG_BASE + statIndex;
    template.paletteTag = paletteTag;
    spriteId = CreateSprite(&template, 0, 0, 0);
    if (spriteId != MAX_SPRITES)
    {
        gSprites[spriteId].oam.priority = 0;
        gSprites[spriteId].invisible = TRUE;
    }

    return spriteId;
}

static void BattleInfo_EnsureHpBarSprite(struct BattleInfoMenu *data)
{
    if (data->hpBarSpriteId != MAX_SPRITES)
        return;

    data->hpBarSpriteId = BattleInfo_CreateHpBarSprite();
}

static void BattleInfo_EnsureStatusIconSprite(struct BattleInfoMenu *data)
{
    if (data->statusIconSpriteId != MAX_SPRITES)
        return;

    if (GetSpriteTileStartByTag(BINFO_STATUS_ICON_TILE_TAG) == 0xFFFF)
        LoadCompressedSpriteSheet(&sBattleInfoStatusIconsSpriteSheet);
    if (IndexOfSpritePaletteTag(BINFO_STATUS_ICON_PAL_TAG) == 0xFF)
        LoadSpritePalette(&sBattleInfoStatusIconsSpritePalette);

    data->statusIconSpriteId = CreateSprite(&sBattleInfoStatusIconSpriteTemplate, 0, 0, 0);
    if (data->statusIconSpriteId != MAX_SPRITES)
    {
        gSprites[data->statusIconSpriteId].oam.priority = 0;
        gSprites[data->statusIconSpriteId].invisible = TRUE;
    }
}

static void BattleInfo_EnsureTypeIcons(struct BattleInfoMenu *data)
{
    u32 i;

    if (data->typeIconsCreated)
        return;

    if (GetSpriteTileStartByTag(gSpriteSheet_MoveTypes.tag) == 0xFFFF)
        LoadCompressedSpriteSheet(&gSpriteSheet_MoveTypes);
    if (!data->typeIconsGfxLoaded)
    {
        LoadPalette(gMoveTypes_Pal, OBJ_PLTT_ID(13), 3 * PLTT_SIZE_4BPP);
        data->typeIconsGfxLoaded = TRUE;
    }

    for (i = 0; i < BINFO_TYPE_ICON_COUNT; i++)
    {
        data->typeIconSpriteIds[i] = CreateSprite(&gSpriteTemplate_MoveTypes, 0, 0, 0);
        if (data->typeIconSpriteIds[i] != MAX_SPRITES)
        {
            gSprites[data->typeIconSpriteIds[i]].oam.priority = 0;
            gSprites[data->typeIconSpriteIds[i]].invisible = TRUE;
        }
    }

    data->typeIconsCreated = TRUE;
}

static void BattleInfo_SetHpBarVisible(struct BattleInfoMenu *data, bool8 visible)
{
    if (data->hpBarSpriteId == MAX_SPRITES)
        return;

    gSprites[data->hpBarSpriteId].invisible = !visible;
}

static void BattleInfo_SetStatusIconVisible(struct BattleInfoMenu *data, bool8 visible)
{
    if (data->statusIconSpriteId == MAX_SPRITES)
        return;

    gSprites[data->statusIconSpriteId].invisible = !visible;
}

static void BattleInfo_SetTypeIconsInvisible(struct BattleInfoMenu *data)
{
    u32 i;

    if (!data->typeIconsCreated)
        return;

    for (i = 0; i < BINFO_TYPE_ICON_COUNT; i++)
    {
        if (data->typeIconSpriteIds[i] == MAX_SPRITES)
            continue;
        gSprites[data->typeIconSpriteIds[i]].invisible = TRUE;
    }
}

static void BattleInfo_SetEnemyIconsVisible(struct BattleInfoMenu *data, bool8 visible)
{
    u32 i;

    if (!data->iconsCreated)
        return;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (data->enemySlotSpriteIds[i] == 0xFF)
            continue;
        gSprites[data->enemySlotSpriteIds[i]].invisible = !visible;
    }
}

static void BattleInfo_UpdateEnemySelectionSprite(struct BattleInfoMenu *data)
{
    u32 i;
    u8 count = BattleInfo_GetEnemyPartyCount();
    u8 selected = gBattleStruct->battleInfo.enemySlot;

    if (!data->iconsCreated)
        return;

    for (i = 0; i < count; i++)
    {
        u8 spriteId = data->enemySlotSpriteIds[i];
        if (spriteId == 0xFF)
            continue;
        gSprites[spriteId].y = data->enemySlotBaseY[i] + ((i == selected) ? -2 : 0);
    }
}

static void BattleInfo_DestroySummarySprite(struct BattleInfoMenu *data)
{
    if (data->summarySpriteId != 0xFF)
    {
        FreeAndDestroyMonPicSprite(data->summarySpriteId);
        data->summarySpriteId = 0xFF;
    }
}

static void BattleInfo_DestroyHpBarSprite(struct BattleInfoMenu *data)
{
    if (data->hpBarSpriteId != MAX_SPRITES)
    {
        DestroySprite(&gSprites[data->hpBarSpriteId]);
        data->hpBarSpriteId = MAX_SPRITES;
    }

    FreeSpriteTilesByTag(BINFO_HP_BAR_TILE_TAG);
    FreeSpritePaletteByTag(BINFO_HP_BAR_PAL_TAG);
}

static void BattleInfo_DestroyStatusIconSprite(struct BattleInfoMenu *data)
{
    if (data->statusIconSpriteId != MAX_SPRITES)
    {
        DestroySprite(&gSprites[data->statusIconSpriteId]);
        data->statusIconSpriteId = MAX_SPRITES;
    }

    FreeSpriteTilesByTag(BINFO_STATUS_ICON_TILE_TAG);
    FreeSpritePaletteByTag(BINFO_STATUS_ICON_PAL_TAG);
}

static void BattleInfo_DestroyTypeIcons(struct BattleInfoMenu *data)
{
    u32 i;

    if (data->typeIconsCreated)
    {
        for (i = 0; i < BINFO_TYPE_ICON_COUNT; i++)
        {
            if (data->typeIconSpriteIds[i] == MAX_SPRITES)
                continue;
            DestroySprite(&gSprites[data->typeIconSpriteIds[i]]);
            data->typeIconSpriteIds[i] = MAX_SPRITES;
        }
        data->typeIconsCreated = FALSE;
    }

    if (data->typeIconsGfxLoaded)
    {
        FreeSpriteTilesByTag(gSpriteSheet_MoveTypes.tag);
        data->typeIconsGfxLoaded = FALSE;
    }
}

static void BattleInfo_DestroyFieldIcons(struct BattleInfoMenu *data)
{
    u32 i;

    if (!data->fieldIconsCreated)
        return;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        if (data->fieldIconSpriteIds[i] == MAX_SPRITES)
            continue;
        FreeAndDestroyMonIconSprite(&gSprites[data->fieldIconSpriteIds[i]]);
        data->fieldIconSpriteIds[i] = MAX_SPRITES;
    }
}

static void BattleInfo_DestroyFieldHpBars(struct BattleInfoMenu *data)
{
    u32 i;

    if (!data->fieldHpBarsCreated)
        return;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        if (data->fieldHpBarSpriteIds[i] == MAX_SPRITES)
            continue;
        DestroySprite(&gSprites[data->fieldHpBarSpriteIds[i]]);
        data->fieldHpBarSpriteIds[i] = MAX_SPRITES;
    }

    data->fieldHpBarsCreated = FALSE;
}

static void BattleInfo_DestroyFieldStatusIcons(struct BattleInfoMenu *data)
{
    u32 i;

    if (!data->fieldStatusIconsCreated)
        return;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        if (data->fieldStatusIconSpriteIds[i] == MAX_SPRITES)
            continue;
        DestroySprite(&gSprites[data->fieldStatusIconSpriteIds[i]]);
        data->fieldStatusIconSpriteIds[i] = MAX_SPRITES;
    }

    data->fieldStatusIconsCreated = FALSE;
}

static void BattleInfo_DestroyFieldStatIndicators(struct BattleInfoMenu *data)
{
    u32 i;
    u32 j;

    if (!data->fieldStatIndicatorsCreated)
        return;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        for (j = 0; j < BINFO_FIELD_STAT_SPRITE_COUNT; j++)
        {
            if (data->fieldStatIndicatorSpriteIds[i][j] == MAX_SPRITES)
                continue;
            DestroySprite(&gSprites[data->fieldStatIndicatorSpriteIds[i][j]]);
            data->fieldStatIndicatorSpriteIds[i][j] = MAX_SPRITES;
        }
    }

    data->fieldStatIndicatorsCreated = FALSE;
}

static void BattleInfo_FreeStatIndicatorGfx(void)
{
    u32 i;

    for (i = 0; i < BINFO_STAT_STAGE_TILE_TAG_COUNT; i++)
        FreeSpriteTilesByTag(BINFO_STAT_STAGE_TILE_TAG_BASE + i);
    for (i = 0; i < BINFO_STAT_ICON_TILE_TAG_COUNT; i++)
        FreeSpriteTilesByTag(BINFO_STAT_ICON_TILE_TAG_BASE + i);
    FreeSpritePaletteByTag(BINFO_STAT_INDICATOR_PLUS_PAL_TAG);
    FreeSpritePaletteByTag(BINFO_STAT_INDICATOR_MINUS_PAL_TAG);
}

static void BattleInfo_DrawEnemySummary(struct BattleInfoMenu *data)
{
    u8 displaySlot = gBattleStruct->battleInfo.enemySlot;
    u8 slot = BattleInfo_GetEnemyPartySlotByDisplaySlot(displaySlot);
    u8 rightY = BINFO_SUMMARY_RIGHT_Y;
    u8 text[64];
    struct BattleInfoMon *info = &gBattleStruct->battleInfo.mon[B_SIDE_OPPONENT][slot];

    BattleInfo_UpdateEnemySelectionSprite(data);
    if (data->summarySpriteSlot != slot)
    {
        data->summarySpriteSlot = slot;
        if (info->seen && data->summarySpriteId != 0xFF)
            data->summarySpriteLoadDelay = BINFO_SUMMARY_SPRITE_LOAD_DELAY;
        else
            data->summarySpriteLoadDelay = 0;
        BattleInfo_DestroySummarySprite(data);
    }
    FillWindowPixelBuffer(data->contentWindowId, PIXEL_FILL(0));
    BattleInfo_SetTypeIconsInvisible(data);

    if (!info->seen)
    {
        if (data->summarySpriteId == 0xFF && data->summarySpriteLoadDelay == 0)
        {
            data->summarySpriteId = CreateMonPicSprite(SPECIES_NONE, FALSE, 0, TRUE,
                                                       BINFO_SUMMARY_SPRITE_X, BINFO_SUMMARY_SPRITE_Y, 0, SPECIES_NONE);
            if (data->summarySpriteId != 0xFF)
                gSprites[data->summarySpriteId].oam.priority = 0;
        }

        {
            s16 tileLeft = GetWindowAttribute(data->contentWindowId, WINDOW_TILEMAP_LEFT);
            s16 tileTop = GetWindowAttribute(data->contentWindowId, WINDOW_TILEMAP_TOP);
            s16 windowLeft = tileLeft * 8;
            s16 windowTop = tileTop * 8;
            s16 spriteX = BINFO_SUMMARY_SPRITE_X;
            s16 spriteY = BINFO_SUMMARY_SPRITE_Y;
            s16 spriteLeft;
            s16 nameX;
            s16 nameY;
            u32 nameWidth;

            if (data->summarySpriteId != 0xFF)
            {
                spriteX = gSprites[data->summarySpriteId].x + gSprites[data->summarySpriteId].x2;
                spriteY = gSprites[data->summarySpriteId].y + gSprites[data->summarySpriteId].y2;
            }

            spriteLeft = spriteX - (BINFO_SUMMARY_SPRITE_WIDTH / 2);
            nameY = spriteY + BINFO_SUMMARY_NAME_OFFSET_Y - windowTop;

            StringCopy(text, sText_UnknownItem);
            nameWidth = GetStringWidth(FONT_NORMAL, text, 0);
            nameX = spriteLeft - windowLeft;
            if (nameWidth < BINFO_SUMMARY_SPRITE_WIDTH)
                nameX += (BINFO_SUMMARY_SPRITE_WIDTH - nameWidth) / 2;
            if (nameX < 0)
                nameX = 0;

            AddTextPrinterParameterized4(data->contentWindowId, FONT_NORMAL, nameX, nameY, 0, 0, sBattleInfoTextColors_Black, 0, text);
        }

        {
            s16 typeY = (s16)rightY - BINFO_TYPE_LINE_OFFSET;
            if (typeY < 0)
                typeY = 0;
            BattleInfo_DrawTypeLineAndIconsUnknown(data, BINFO_SUMMARY_RIGHT_X, typeY);
        }

        BattleInfo_DrawHpBarSprite(data, BINFO_SUMMARY_RIGHT_X, rightY, BINFO_HP_BAR_SCALE);
        BattleInfo_SetStatusIconVisible(data, FALSE);
        rightY += 16;
        BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, sText_NotRevealedYet, BINFO_SUMMARY_RIGHT_X, rightY, 0, NULL);
        CopyWindowToVram(data->contentWindowId, COPYWIN_FULL);
        return;
    }

    if (data->summarySpriteId == 0xFF && data->summarySpriteLoadDelay == 0)
    {
        data->summarySpriteId = CreateMonPicSprite(info->species, info->shiny, info->personality, TRUE,
                                                   BINFO_SUMMARY_SPRITE_X, BINFO_SUMMARY_SPRITE_Y, 0, info->species);
        if (data->summarySpriteId != 0xFF)
            gSprites[data->summarySpriteId].oam.priority = 0;
    }

    {
        s16 tileLeft = GetWindowAttribute(data->contentWindowId, WINDOW_TILEMAP_LEFT);
        s16 tileTop = GetWindowAttribute(data->contentWindowId, WINDOW_TILEMAP_TOP);
        s16 windowLeft = tileLeft * 8;
        s16 windowTop = tileTop * 8;
        s16 spriteX = BINFO_SUMMARY_SPRITE_X;
        s16 spriteY = BINFO_SUMMARY_SPRITE_Y;
        s16 spriteLeft;
        s16 spriteCenterX;
        s16 nameX;
        s16 nameY;
        s16 levelX;
        s16 levelY;
        u8 levelText[8];
        u32 nameWidth;
        u8 level;
        const u8 *genderText = NULL;
        const u8 *genderColors = NULL;

        if (data->summarySpriteId != 0xFF)
        {
            spriteX = gSprites[data->summarySpriteId].x + gSprites[data->summarySpriteId].x2;
            spriteY = gSprites[data->summarySpriteId].y + gSprites[data->summarySpriteId].y2;
        }

        spriteLeft = spriteX - (BINFO_SUMMARY_SPRITE_WIDTH / 2);
        spriteCenterX = spriteX - windowLeft;
        nameY = spriteY + BINFO_SUMMARY_NAME_OFFSET_Y - windowTop;

        StringCopy(text, GetSpeciesName(info->species));
        nameWidth = GetStringWidth(FONT_NORMAL, text, 0);
        nameX = spriteLeft - windowLeft;
        if (nameWidth < BINFO_SUMMARY_SPRITE_WIDTH)
            nameX += (BINFO_SUMMARY_SPRITE_WIDTH - nameWidth) / 2;
        if (nameX < 0)
            nameX = 0;

        AddTextPrinterParameterized4(data->contentWindowId, FONT_NORMAL, nameX, nameY, 0, 0, sBattleInfoTextColors_Black, 0, text);

        if (info->species != SPECIES_NIDORAN_M && info->species != SPECIES_NIDORAN_F)
        {
            if (info->gender == BINFO_GENDER_MALE)
            {
                genderText = gText_MaleSymbol;
                genderColors = sBattleInfoTextColors_Male;
            }
            else if (info->gender == BINFO_GENDER_FEMALE)
            {
                genderText = gText_FemaleSymbol;
                genderColors = sBattleInfoTextColors_Female;
            }
        }

        level = GetMonData(&gEnemyParty[slot], MON_DATA_LEVEL);
        StringCopy(levelText, gText_LevelSymbol);
        ConvertIntToDecimalStringN(levelText + StringLength(levelText), level, STR_CONV_MODE_LEFT_ALIGN, 3);
        levelX = spriteCenterX - BINFO_SUMMARY_LEVEL_CENTER_OFFSET;
        if (levelX < 0)
            levelX = 0;

        levelY = nameY + BINFO_SUMMARY_LEVEL_OFFSET_Y;
        AddTextPrinterParameterized4(data->contentWindowId, FONT_NORMAL, levelX, levelY, 0, 0, sBattleInfoTextColors_Black, 0, levelText);

        if (genderText != NULL && genderColors != NULL)
        {
            s16 genderX = spriteCenterX + BINFO_SUMMARY_GENDER_CENTER_OFFSET;

            if (genderX < 0)
                genderX = 0;
            AddTextPrinterParameterized4(data->contentWindowId, FONT_NORMAL, genderX, levelY, 0, 0, genderColors, 0, genderText);
        }
    }

    {
        s16 typeY = (s16)rightY - BINFO_TYPE_LINE_OFFSET;
        if (typeY < 0)
            typeY = 0;
        BattleInfo_DrawTypeLineAndIcons(data, info->species, BINFO_SUMMARY_RIGHT_X, typeY);
    }

    BattleInfo_DrawHpBarSprite(data, BINFO_SUMMARY_RIGHT_X, rightY, info->hpFraction);
    BattleInfo_DrawStatusIconSprite(data, info, BINFO_SUMMARY_RIGHT_X, rightY);
    rightY += 16;

    {
        enum Ability abilities[NUM_ABILITY_SLOTS];
        enum Ability confirmed;
        u8 abilityCount = BattleInfo_GetAbilityList(info->species, abilities);
        confirmed = BattleInfo_GetConfirmedAbility(slot, abilities, abilityCount);
        StringCopy(text, sText_AbilityLine);
        if (confirmed != ABILITY_NONE)
        {
            StringAppend(text, gAbilitiesInfo[confirmed].name);
        }
        else
        {
            u8 countText[4];
            ConvertIntToDecimalStringN(countText, abilityCount, STR_CONV_MODE_LEFT_ALIGN, 1);
            StringAppend(text, countText);
            StringAppend(text, COMPOUND_STRING(" Possibilities"));
        }
        BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, text, BINFO_SUMMARY_RIGHT_X, rightY, 0, NULL);
        rightY += 16;
    }

    StringCopy(text, sText_ItemHeader);
    if (info->itemId != ITEM_NONE)
    {
        CopyItemName(info->itemId, text + StringLength(text));
    }
    else
    {
        StringAppend(text, sText_UnknownItem);
    }
    BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, text, BINFO_SUMMARY_RIGHT_X, rightY, 0, NULL);
    rightY += 16;

    BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, sText_A_Details, BINFO_SUMMARY_RIGHT_X, rightY, 0, NULL);

    CopyWindowToVram(data->contentWindowId, COPYWIN_FULL);
}

static void BattleInfo_DrawEnemyDetails(struct BattleInfoMenu *data)
{
    u8 slot = data->detailSlot;
    u8 y = 1;
    struct BattleInfoMon *info = &gBattleStruct->battleInfo.mon[B_SIDE_OPPONENT][slot];

    BattleInfo_UpdateEnemySelectionSprite(data);
    FillWindowPixelBuffer(data->contentWindowId, PIXEL_FILL(0));
    BattleInfo_SetTypeIconsInvisible(data);

    BattleInfo_PrintEnemyHeader(data, &y, slot);

    if (!info->seen)
    {
        BattleInfo_SetHpBarVisible(data, FALSE);
        BattleInfo_SetStatusIconVisible(data, FALSE);
        BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, sText_UnknownPokemon, 0, y, 0, NULL);
        CopyWindowToVram(data->contentWindowId, COPYWIN_FULL);
        return;
    }

    if (data->detailPage == DETAILS_PAGE_INFO)
    {
        u8 text[64];
        enum Ability abilities[NUM_ABILITY_SLOTS];
        u8 abilityCount = BattleInfo_GetAbilityList(info->species, abilities);
        enum Ability confirmed = BattleInfo_GetConfirmedAbility(slot, abilities, abilityCount);
        u32 i;

        BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, sText_PageInfo, 150, 1, 0, NULL);

        BattleInfo_DrawHpBarSprite(data, 0, y, info->hpFraction);
        BattleInfo_DrawStatusIconSprite(data, info, 0, y);
        y += 16;

        StringCopy(text, sText_ItemHeader);
        if (info->itemId != ITEM_NONE)
            CopyItemName(info->itemId, text + StringLength(text));
        else
            StringAppend(text, sText_UnknownItem);
        BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, text, 0, y, 0, NULL);
        y += 16;

        BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, sText_AbilityHeader, 0, y, 0, NULL);
        y += 16;

        if (abilityCount == 0)
        {
            BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, sText_None, 0, y, 0, NULL);
        }
        else
        {
            if (data->abilityIndex >= abilityCount)
                data->abilityIndex = 0;
            for (i = 0; i < abilityCount; i++)
            {
                u8 *ptr = text;
                if (data->abilityMode == ABILITY_MODE_LIST && data->abilityIndex == i)
                    *ptr++ = CHAR_RIGHT_ARROW;
                else
                    *ptr++ = CHAR_SPACE;
                *ptr++ = CHAR_SPACE;
                if (confirmed != ABILITY_NONE && abilities[i] != confirmed)
                {
                    *ptr++ = CHAR_X;
                    *ptr++ = CHAR_SPACE;
                }
                ptr = StringCopy(ptr, gAbilitiesInfo[abilities[i]].name);
                *ptr = EOS;
                BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, text, 0, y, 0, NULL);
                y += 16;
            }
        }
    }
    else
    {
        BattleInfo_SetHpBarVisible(data, FALSE);
        BattleInfo_SetStatusIconVisible(data, FALSE);
        u8 text[64];
        u32 i;
        BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, sText_PageMoves, 150, 1, 0, NULL);
        BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, sText_KnownMoves, 0, y, 0, NULL);
        y += 16;

        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            u16 move = gBattleStruct->battleInfo.mon[B_SIDE_OPPONENT][slot].knownMoves[i];
            u8 *ptr = text;
            ptr = ConvertIntToDecimalStringN(ptr, i + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
            *ptr++ = CHAR_COLON;
            *ptr++ = CHAR_SPACE;
            if (move == MOVE_NONE)
            {
                StringCopy(ptr, sText_MoveSlotUnknown);
            }
            else
            {
                u8 ppText[8];
                ptr = StringCopy(ptr, GetMoveName(move));
                StringAppend(ptr, COMPOUND_STRING("  PP used: "));
                ConvertIntToDecimalStringN(ppText, gBattleStruct->battleInfo.mon[B_SIDE_OPPONENT][slot].ppUsed[i], STR_CONV_MODE_LEFT_ALIGN, 2);
                StringAppend(ptr, ppText);
            }
            BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, text, 0, y, 0, NULL);
            y += 16;
        }
    }

    CopyWindowToVram(data->contentWindowId, COPYWIN_FULL);
}

static void BattleInfo_DrawFieldInfo(struct BattleInfoMenu *data)
{
    BattleInfo_SetTypeIconsInvisible(data);
    BattleInfo_SetStatusIconVisible(data, FALSE);
    BattleInfo_DestroyFieldIcons(data);
    BattleInfo_DestroyFieldHpBars(data);
    BattleInfo_DestroyFieldStatusIcons(data);
    BattleInfo_DestroyFieldStatIndicators(data);

    switch (gBattleStruct->battleInfo.fieldPage)
    {
    case FIELD_PAGE_ACTIVE:
        BattleInfo_DrawFieldActive(data);
        break;
    case FIELD_PAGE_EFFECTS:
        BattleInfo_DrawFieldEffects(data);
        break;
    }
}

static u8 BattleInfo_DrawFieldTitle(struct BattleInfoMenu *data, const u8 *title)
{
    u16 windowWidth = GetWindowAttribute(data->contentWindowId, WINDOW_WIDTH) * 8;
    u8 x = GetStringCenterAlignXOffset(FONT_NORMAL, title, windowWidth);

    BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, title, x, 1, 0, NULL);
    return 13;
}

static void BattleInfo_DrawFieldActive(struct BattleInfoMenu *data)
{
    u8 y;
    u8 playerSlot = 0;
    u8 opponentSlot = 0;
    u32 battler;

    FillWindowPixelBuffer(data->contentWindowId, PIXEL_FILL(0));
    y = BattleInfo_DrawFieldTitle(data, sText_FieldActiveHeader) + 8;

    LoadMonIconPalettes();
    data->fieldIconsCreated = TRUE;

    for (battler = 0; battler < gBattlersCount; battler++)
    {
        if (gAbsentBattlerFlags & (1u << battler))
            continue;
        if (gBattleMons[battler].species == SPECIES_NONE)
            continue;

        if (GetBattlerSide(battler) == B_SIDE_PLAYER)
        {
            if (playerSlot >= BINFO_FIELD_MAX_SLOTS)
                continue;
            BattleInfo_DrawFieldBattlerSection(data, battler, BINFO_FIELD_COLUMN_LEFT_X,
                                               y + (playerSlot * BINFO_FIELD_SECTION_HEIGHT));
            playerSlot++;
        }
        else
        {
            if (opponentSlot >= BINFO_FIELD_MAX_SLOTS)
                continue;
            BattleInfo_DrawFieldBattlerSection(data, battler, BINFO_FIELD_COLUMN_RIGHT_X,
                                               y + (opponentSlot * BINFO_FIELD_SECTION_HEIGHT));
            opponentSlot++;
        }
    }

    CopyWindowToVram(data->contentWindowId, COPYWIN_FULL);
}

static void BattleInfo_DrawFieldBattlerSection(struct BattleInfoMenu *data, u32 battler, s16 x, s16 y)
{
    u8 name[POKEMON_NAME_LENGTH + 1];
    u8 hpFraction;
    u8 iconSpriteId;
    u8 hpBarSpriteId;
    s16 tileLeft = GetWindowAttribute(data->contentWindowId, WINDOW_TILEMAP_LEFT);
    s16 tileTop = GetWindowAttribute(data->contentWindowId, WINDOW_TILEMAP_TOP);
    s16 windowLeft = tileLeft * 8;
    s16 windowTop = tileTop * 8;
    s16 iconX = windowLeft + x + BINFO_FIELD_ICON_X_OFFSET;
    s16 iconY = windowTop + y + BINFO_FIELD_ICON_Y_OFFSET;

    iconSpriteId = CreateMonIcon(gBattleMons[battler].species, SpriteCallbackDummy, iconX, iconY, 0,
                                 gBattleMons[battler].personality);
    if (iconSpriteId != MAX_SPRITES)
    {
        data->fieldIconSpriteIds[battler] = iconSpriteId;
        data->fieldIconsCreated = TRUE;
    }

    StringCopyN(name, GetSpeciesName(gBattleMons[battler].species), POKEMON_NAME_LENGTH);
    name[POKEMON_NAME_LENGTH] = EOS;
    {
        s16 nameY = y + BINFO_FIELD_NAME_Y_OFFSET;
        if (nameY < 0)
            nameY = 0;
        BattleInfo_PrintText(data->contentWindowId, FONT_SMALL, name, x + BINFO_FIELD_NAME_X_OFFSET,
                             nameY, 0, NULL);
    }

    hpFraction = GetScaledHPFraction(gBattleMons[battler].hp, gBattleMons[battler].maxHP, BINFO_HP_BAR_SCALE);
    hpBarSpriteId = data->fieldHpBarSpriteIds[battler];
    if (hpBarSpriteId == MAX_SPRITES)
    {
        hpBarSpriteId = BattleInfo_CreateHpBarSprite();
        data->fieldHpBarSpriteIds[battler] = hpBarSpriteId;
        if (hpBarSpriteId != MAX_SPRITES)
            data->fieldHpBarsCreated = TRUE;
    }

    if (hpBarSpriteId != MAX_SPRITES)
    {
        BattleInfo_UpdateHpBarTiles(hpBarSpriteId, hpFraction);
        gSprites[hpBarSpriteId].x = windowLeft + x + BINFO_FIELD_HP_X_OFFSET + BINFO_HP_BAR_LABEL_PIXELS;
        gSprites[hpBarSpriteId].y = windowTop + y + BINFO_FIELD_HP_Y_OFFSET;
        gSprites[hpBarSpriteId].invisible = FALSE;
    }

    {
        u32 status1 = gBattleMons[battler].status1;
        u8 ailment = BattleInfo_GetAilmentFromStatus1(status1);
        s16 reservedLeft = x + BINFO_FIELD_STATUS_X_OFFSET;
        s16 rowY[BINFO_FIELD_FLOW_ROWS];
        u8 rowIndex = 0;
        u8 slotIndex = 0;
        bool8 outOfSpace = FALSE;
        u32 row;

        for (row = 0; row < BINFO_FIELD_FLOW_ROWS; row++)
            rowY[row] = y + BINFO_FIELD_STATUS_Y_OFFSET + (row * BINFO_FIELD_STATUS_LINE_HEIGHT);

        if (ailment != AILMENT_NONE && !outOfSpace)
        {
            u8 countText[24];
            bool8 hasCounter = FALSE;
            u8 slotsNeeded = 1;

            if (status1 & STATUS1_SLEEP)
            {
                u8 turns = status1 & STATUS1_SLEEP;

                StringCopy(countText, COMPOUND_STRING("("));
                ConvertIntToDecimalStringN(countText + StringLength(countText), turns, STR_CONV_MODE_LEFT_ALIGN, 2);
                if (turns == 1)
                    StringAppend(countText, COMPOUND_STRING(BINFO_FIELD_TURNS_TEXT_SINGULAR));
                else
                    StringAppend(countText, COMPOUND_STRING(BINFO_FIELD_TURNS_TEXT_PLURAL));
                hasCounter = TRUE;
                slotsNeeded = 3;
            }
            else if (status1 & STATUS1_TOXIC_POISON)
            {
                u8 turns = (status1 & STATUS1_TOXIC_COUNTER) >> 8;

                StringCopy(countText, COMPOUND_STRING("("));
                ConvertIntToDecimalStringN(countText + StringLength(countText), turns, STR_CONV_MODE_LEFT_ALIGN, 2);
                if (turns == 1)
                    StringAppend(countText, COMPOUND_STRING(BINFO_FIELD_TURNS_TEXT_SINGULAR));
                else
                    StringAppend(countText, COMPOUND_STRING(BINFO_FIELD_TURNS_TEXT_PLURAL));
                hasCounter = TRUE;
                slotsNeeded = 3;
            }

            if (slotIndex + slotsNeeded > BINFO_FIELD_DETAIL_SLOTS_PER_ROW)
            {
                rowIndex++;
                slotIndex = 0;
            }

            if (rowIndex >= BINFO_FIELD_FLOW_ROWS)
            {
                outOfSpace = TRUE;
            }
            else
            {
                u8 statusSpriteId = data->fieldStatusIconSpriteIds[battler];
                s16 slotX = reservedLeft + (slotIndex * BINFO_FIELD_CELL_WIDTH);
                s16 drawY = rowY[rowIndex];

                if (statusSpriteId == MAX_SPRITES)
                {
                    statusSpriteId = BattleInfo_CreateFieldStatusIconSprite();
                    data->fieldStatusIconSpriteIds[battler] = statusSpriteId;
                    if (statusSpriteId != MAX_SPRITES)
                        data->fieldStatusIconsCreated = TRUE;
                }

                if (statusSpriteId != MAX_SPRITES)
                {
                    gSprites[statusSpriteId].x = windowLeft + slotX + (BINFO_FIELD_STATUS_ICON_SPRITE_WIDTH / 2) + BINFO_FIELD_STATUS_ICON_X_ADJUST;
                    gSprites[statusSpriteId].y = windowTop + drawY;
                    StartSpriteAnim(&gSprites[statusSpriteId], ailment - 1);
                    gSprites[statusSpriteId].invisible = FALSE;
                }

                if (hasCounter)
                {
                    BattleInfo_PrintText(data->contentWindowId, FONT_SMALL, countText,
                                         slotX + BINFO_FIELD_STATUS_ICON_WIDTH + BINFO_FIELD_STATUS_TURN_GAP,
                                         drawY + BINFO_FIELD_FLOW_TEXT_Y_OFFSET, 0, NULL);
                }

                slotIndex += slotsNeeded;
            }
        }

        if (!outOfSpace)
        {
            struct
            {
                enum Stat stat;
                u8 animIndex;
            } stats[] =
            {
                {STAT_ATK, 0},
                {STAT_DEF, 1},
                {STAT_SPATK, 2},
                {STAT_SPDEF, 3},
                {STAT_SPEED, 4},
                {STAT_ACC, 5},
                {STAT_EVASION, 6},
            };
            u8 indicatorCount = 0;
            u32 i;

            for (i = 0; i < ARRAY_COUNT(stats); i++)
            {
                s8 stage = gBattleMons[battler].statStages[stats[i].stat] - DEFAULT_STAT_STAGE;
                u8 magnitude;
                u8 stageAnim;
                u8 stageSpriteId;
                u8 statSpriteId;
                u16 paletteTag;
                s16 slotX;
                s16 drawY;

                if (stage == 0)
                    continue;
                if (indicatorCount >= BINFO_FIELD_STAT_INDICATORS_MAX)
                    break;

                if (slotIndex >= BINFO_FIELD_DETAIL_SLOTS_PER_ROW)
                {
                    rowIndex++;
                    slotIndex = 0;
                }

                if (rowIndex >= BINFO_FIELD_FLOW_ROWS)
                {
                    outOfSpace = TRUE;
                    break;
                }

                magnitude = (stage > 0) ? stage : -stage;
                if (magnitude > 6)
                    magnitude = 6;
                stageAnim = (stage > 0) ? (magnitude - 1) : (6 + magnitude - 1);
                paletteTag = (stage > 0) ? BINFO_STAT_INDICATOR_PLUS_PAL_TAG : BINFO_STAT_INDICATOR_MINUS_PAL_TAG;

                slotX = reservedLeft + (slotIndex * BINFO_FIELD_CELL_WIDTH);
                drawY = rowY[rowIndex];

                stageSpriteId = BattleInfo_CreateStatStageSprite(stageAnim, paletteTag);
                if (stageSpriteId != MAX_SPRITES)
                {
                    data->fieldStatIndicatorSpriteIds[battler][indicatorCount * 2] = stageSpriteId;
                    data->fieldStatIndicatorsCreated = TRUE;
                    gSprites[stageSpriteId].x = windowLeft + slotX + (BINFO_FIELD_STAT_STAGE_WIDTH / 2);
                    gSprites[stageSpriteId].y = windowTop + drawY;
                    gSprites[stageSpriteId].invisible = FALSE;
                }

                statSpriteId = BattleInfo_CreateStatIconSprite(stats[i].animIndex, paletteTag);
                if (statSpriteId != MAX_SPRITES)
                {
                    data->fieldStatIndicatorSpriteIds[battler][indicatorCount * 2 + 1] = statSpriteId;
                    data->fieldStatIndicatorsCreated = TRUE;
                    gSprites[statSpriteId].x = windowLeft + slotX + BINFO_FIELD_STAT_ICON_X_OFFSET + (BINFO_FIELD_STAT_ICON_WIDTH / 2);
                    gSprites[statSpriteId].y = windowTop + drawY;
                    gSprites[statSpriteId].invisible = FALSE;
                }

                slotIndex++;
                indicatorCount++;
            }
        }

        if (!outOfSpace)
        {
            u8 lines[BINFO_FIELD_EFFECT_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN];
            u8 effectCount = BattleInfo_CollectFieldEffects(lines, battler);
            u8 effectsText[BINFO_FIELD_EFFECT_LINE_LEN * BINFO_FIELD_EFFECT_MAX_LINES + 16];
            u8 line;

            effectsText[0] = EOS;
            for (line = 0; line < effectCount; line++)
            {
                if (effectsText[0] != EOS)
                    StringAppend(effectsText, COMPOUND_STRING(", "));
                StringAppend(effectsText, lines[line]);
            }

            if (effectsText[0] != EOS)
            {
                if (slotIndex >= BINFO_FIELD_DETAIL_SLOTS_PER_ROW)
                {
                    rowIndex++;
                    slotIndex = 0;
                }

                if (rowIndex < BINFO_FIELD_FLOW_ROWS)
                {
                    s16 slotX = reservedLeft + (slotIndex * BINFO_FIELD_CELL_WIDTH);
                    s16 drawY = rowY[rowIndex];

                    BattleInfo_PrintText(data->contentWindowId, FONT_SMALL, effectsText,
                                         slotX, drawY + BINFO_FIELD_FLOW_TEXT_Y_OFFSET, 0, NULL);
                }
            }
        }
    }
}

static void BattleInfo_DrawFieldEffects(struct BattleInfoMenu *data)
{
    u8 fieldLines[BINFO_FIELD_PAGE_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN];
    u8 sideLinesLeft[BINFO_FIELD_PAGE_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN];
    u8 sideLinesRight[BINFO_FIELD_PAGE_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN];
    u8 sideColorsLeft[BINFO_FIELD_PAGE_MAX_LINES];
    u8 sideColorsRight[BINFO_FIELD_PAGE_MAX_LINES];
    u8 fieldCount;
    u8 sideLeftCount;
    u8 sideRightCount;
    bool8 hasFieldConditions;
    bool8 hasSideConditions;
    u16 windowWidth;
    u16 windowHeight;
    s16 contentTop;
    s16 y;
    u8 maxFieldLines;
    u8 maxSideLines;

    FillWindowPixelBuffer(data->contentWindowId, PIXEL_FILL(0));
    contentTop = BattleInfo_DrawFieldTitle(data, sText_FieldEffectsHeader);
    windowWidth = GetWindowAttribute(data->contentWindowId, WINDOW_WIDTH) * 8;
    windowHeight = GetWindowAttribute(data->contentWindowId, WINDOW_HEIGHT) * 8;
    y = contentTop;

    fieldCount = BattleInfo_CollectGlobalEffects(fieldLines);
    sideLeftCount = BattleInfo_CollectSideConditions(B_SIDE_PLAYER, sideLinesLeft, sideColorsLeft);
    sideRightCount = BattleInfo_CollectSideConditions(B_SIDE_OPPONENT, sideLinesRight, sideColorsRight);

    hasFieldConditions = (fieldCount > 0);
    hasSideConditions = (sideLeftCount > 0 || sideRightCount > 0);

    if (!hasFieldConditions && !hasSideConditions)
    {
        u8 x = GetStringCenterAlignXOffset(FONT_SMALL, sText_FieldNoEffects, windowWidth);
        BattleInfo_PrintText(data->contentWindowId, FONT_SMALL, sText_FieldNoEffects, x, contentTop, 0, NULL);
        CopyWindowToVram(data->contentWindowId, COPYWIN_FULL);
        return;
    }

    if (hasFieldConditions)
    {
        s16 remainingHeight = windowHeight - y;
        s16 reserveHeight = 0;

        if (hasSideConditions)
            reserveHeight = (BINFO_FIELD_EFFECT_LINE_HEIGHT * 3);

        if (remainingHeight > reserveHeight + BINFO_FIELD_EFFECT_LINE_HEIGHT)
            maxFieldLines = (remainingHeight - reserveHeight - BINFO_FIELD_EFFECT_LINE_HEIGHT) / BINFO_FIELD_EFFECT_LINE_HEIGHT;
        else
            maxFieldLines = 0;

        if (fieldCount > maxFieldLines)
            fieldCount = maxFieldLines;

        BattleInfo_PrintText(data->contentWindowId, FONT_SMALL, sText_FieldConditionsHeader, 0, y, 0, NULL);
        y += BINFO_FIELD_EFFECT_LINE_HEIGHT;
        if (fieldCount != 0)
        {
            BattleInfo_PrintEffectLines(data, fieldLines, fieldCount, 0, y);
            y += fieldCount * BINFO_FIELD_EFFECT_LINE_HEIGHT;
        }
    }

    if (hasSideConditions)
    {
        s16 colWidth = windowWidth / 2;
        s16 leftX = 0;
        s16 rightX = colWidth;
        s16 remainingHeight = windowHeight - y;

        if (hasFieldConditions && remainingHeight > BINFO_FIELD_EFFECT_SECTION_GAP)
        {
            y += BINFO_FIELD_EFFECT_SECTION_GAP;
            remainingHeight -= BINFO_FIELD_EFFECT_SECTION_GAP;
        }

        if (sideLeftCount == 0 && sideRightCount > 0)
        {
            StringCopy(sideLinesLeft[0], sText_HazardNone);
            sideColorsLeft[0] = BINFO_EFFECT_COLOR_DEFAULT;
            sideLeftCount = 1;
        }
        if (sideRightCount == 0 && sideLeftCount > 0)
        {
            StringCopy(sideLinesRight[0], sText_HazardNone);
            sideColorsRight[0] = BINFO_EFFECT_COLOR_DEFAULT;
            sideRightCount = 1;
        }

        if (remainingHeight >= (BINFO_FIELD_EFFECT_LINE_HEIGHT * 2))
        {
            BattleInfo_PrintText(data->contentWindowId, FONT_SMALL, sText_SideConditionsHeader, 0, y, 0, NULL);
            y += BINFO_FIELD_EFFECT_LINE_HEIGHT;
            BattleInfo_PrintText(data->contentWindowId, FONT_SMALL, sText_AllySideHeader, leftX, y, 0, NULL);
            BattleInfo_PrintText(data->contentWindowId, FONT_SMALL, sText_OpponentSideHeader, rightX, y, 0, NULL);
            y += BINFO_FIELD_EFFECT_LINE_HEIGHT;

            if (remainingHeight > (BINFO_FIELD_EFFECT_LINE_HEIGHT * 2))
                maxSideLines = (remainingHeight - (BINFO_FIELD_EFFECT_LINE_HEIGHT * 2)) / BINFO_FIELD_EFFECT_LINE_HEIGHT;
            else
                maxSideLines = 0;

            if (sideLeftCount > maxSideLines)
                sideLeftCount = maxSideLines;
            if (sideRightCount > maxSideLines)
                sideRightCount = maxSideLines;

            BattleInfo_PrintEffectLinesColored(data, sideLinesLeft, sideColorsLeft, sideLeftCount, leftX, y);
            BattleInfo_PrintEffectLinesColored(data, sideLinesRight, sideColorsRight, sideRightCount, rightX, y);
        }
    }

    CopyWindowToVram(data->contentWindowId, COPYWIN_FULL);
}

static bool8 BattleInfo_IsWeatherActive(void)
{
    u32 weatherFlags = B_WEATHER_ANY | B_WEATHER_RAIN_PRIMAL | B_WEATHER_SUN_PRIMAL;

    return (gBattleWeather & weatherFlags) != 0;
}

static bool8 BattleInfo_IsTerrainActive(void)
{
    return (gFieldStatuses & STATUS_FIELD_TERRAIN_ANY) != 0;
}

static u8 BattleInfo_CollectGlobalEffects(u8 lines[BINFO_FIELD_PAGE_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN])
{
    u8 count = 0;

    if (BattleInfo_IsWeatherActive() && count < BINFO_FIELD_PAGE_MAX_LINES)
        BattleInfo_BuildWeatherLine(lines[count++]);
    if (BattleInfo_IsTerrainActive() && count < BINFO_FIELD_PAGE_MAX_LINES)
        BattleInfo_BuildTerrainLine(lines[count++]);
    if (gFieldTimers.trickRoomTimer && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 *dst = lines[count++];
        dst = StringCopy(dst, sText_RoomTrickRoom);
        BattleInfo_AppendFieldConditionTurns(dst, BINFO_FIELD_COND_TRICK_ROOM, gFieldTimers.trickRoomTimer, FALSE);
    }
    if (gFieldTimers.gravityTimer && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 *dst = lines[count++];
        dst = StringCopy(dst, sText_RoomGravity);
        BattleInfo_AppendFieldConditionTurns(dst, BINFO_FIELD_COND_GRAVITY, gFieldTimers.gravityTimer, FALSE);
    }
    if (gFieldTimers.magicRoomTimer && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 *dst = lines[count++];
        dst = StringCopy(dst, sText_RoomMagicRoom);
        BattleInfo_AppendFieldConditionTurns(dst, BINFO_FIELD_COND_MAGIC_ROOM, gFieldTimers.magicRoomTimer, FALSE);
    }
    if (gFieldTimers.wonderRoomTimer && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 *dst = lines[count++];
        dst = StringCopy(dst, sText_RoomWonderRoom);
        BattleInfo_AppendFieldConditionTurns(dst, BINFO_FIELD_COND_WONDER_ROOM, gFieldTimers.wonderRoomTimer, FALSE);
    }
    if (gFieldTimers.mudSportTimer && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 *dst = lines[count++];
        dst = StringCopy(dst, sText_RoomMudSport);
        BattleInfo_AppendFieldConditionTurns(dst, BINFO_FIELD_COND_MUD_SPORT, gFieldTimers.mudSportTimer, FALSE);
    }
    if (gFieldTimers.waterSportTimer && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 *dst = lines[count++];
        dst = StringCopy(dst, sText_RoomWaterSport);
        BattleInfo_AppendFieldConditionTurns(dst, BINFO_FIELD_COND_WATER_SPORT, gFieldTimers.waterSportTimer, FALSE);
    }
    if (gFieldTimers.fairyLockTimer && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 *dst = lines[count++];
        dst = StringCopy(dst, sText_RoomFairyLock);
        BattleInfo_AppendFieldConditionTurns(dst, BINFO_FIELD_COND_FAIRY_LOCK, gFieldTimers.fairyLockTimer, FALSE);
    }

    return count;
}

static u8 BattleInfo_CollectHazardLines(u8 side, u8 lines[BINFO_FIELD_PAGE_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN])
{
    u8 count = 0;

    if (IsHazardOnSide(side, HAZARDS_STEALTH_ROCK) && count < BINFO_FIELD_PAGE_MAX_LINES)
        StringCopy(lines[count++], sText_HazardStealthRock);
    if (IsHazardOnSide(side, HAZARDS_SPIKES) && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 layers = gSideTimers[side].spikesAmount;
        u8 *dst = lines[count++];

        if (layers == 0)
            layers = 1;
        dst = StringCopy(dst, sText_HazardSpikesPrefix);
        ConvertIntToDecimalStringN(dst, layers, STR_CONV_MODE_LEFT_ALIGN, 1);
    }
    if (IsHazardOnSide(side, HAZARDS_TOXIC_SPIKES) && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 layers = gSideTimers[side].toxicSpikesAmount;
        u8 *dst = lines[count++];

        if (layers == 0)
            layers = 1;
        dst = StringCopy(dst, sText_HazardToxicSpikesPrefix);
        ConvertIntToDecimalStringN(dst, layers, STR_CONV_MODE_LEFT_ALIGN, 1);
    }
    if (IsHazardOnSide(side, HAZARDS_STICKY_WEB) && count < BINFO_FIELD_PAGE_MAX_LINES)
        StringCopy(lines[count++], sText_HazardStickyWeb);
    if (IsHazardOnSide(side, HAZARDS_STEELSURGE) && count < BINFO_FIELD_PAGE_MAX_LINES)
        StringCopy(lines[count++], sText_HazardSteelSurge);

    return count;
}

static u8 BattleInfo_CollectSideConditions(u8 side, u8 lines[BINFO_FIELD_PAGE_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN], u8 colors[BINFO_FIELD_PAGE_MAX_LINES])
{
    u8 count = 0;

    if (gSideStatuses[side] & SIDE_STATUS_REFLECT && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 *dst = lines[count];
        dst = StringCopy(dst, sText_SideReflect);
        BattleInfo_AppendSideConditionTurns(dst, side, BINFO_SIDE_COND_REFLECT, gSideTimers[side].reflectTimer, TRUE);
        colors[count++] = BINFO_EFFECT_COLOR_HELPFUL;
    }
    if (gSideStatuses[side] & SIDE_STATUS_LIGHTSCREEN && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 *dst = lines[count];
        dst = StringCopy(dst, sText_SideLightScreen);
        BattleInfo_AppendSideConditionTurns(dst, side, BINFO_SIDE_COND_LIGHT_SCREEN, gSideTimers[side].lightscreenTimer, TRUE);
        colors[count++] = BINFO_EFFECT_COLOR_HELPFUL;
    }
    if (gSideStatuses[side] & SIDE_STATUS_AURORA_VEIL && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 *dst = lines[count];
        dst = StringCopy(dst, sText_SideAuroraVeil);
        BattleInfo_AppendSideConditionTurns(dst, side, BINFO_SIDE_COND_AURORA_VEIL, gSideTimers[side].auroraVeilTimer, TRUE);
        colors[count++] = BINFO_EFFECT_COLOR_HELPFUL;
    }
    if (gSideStatuses[side] & SIDE_STATUS_TAILWIND && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 *dst = lines[count];
        dst = StringCopy(dst, sText_SideTailwind);
        BattleInfo_AppendSideConditionTurns(dst, side, BINFO_SIDE_COND_TAILWIND, gSideTimers[side].tailwindTimer, FALSE);
        colors[count++] = BINFO_EFFECT_COLOR_HELPFUL;
    }
    if (gSideStatuses[side] & SIDE_STATUS_SAFEGUARD && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 *dst = lines[count];
        dst = StringCopy(dst, sText_SideSafeguard);
        BattleInfo_AppendSideConditionTurns(dst, side, BINFO_SIDE_COND_SAFEGUARD, gSideTimers[side].safeguardTimer, FALSE);
        colors[count++] = BINFO_EFFECT_COLOR_HELPFUL;
    }
    if (gSideStatuses[side] & SIDE_STATUS_MIST && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 *dst = lines[count];
        dst = StringCopy(dst, sText_SideMist);
        BattleInfo_AppendSideConditionTurns(dst, side, BINFO_SIDE_COND_MIST, gSideTimers[side].mistTimer, FALSE);
        colors[count++] = BINFO_EFFECT_COLOR_HELPFUL;
    }
    if (gSideStatuses[side] & SIDE_STATUS_LUCKY_CHANT && count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 *dst = lines[count];
        dst = StringCopy(dst, sText_SideLuckyChant);
        BattleInfo_AppendSideConditionTurns(dst, side, BINFO_SIDE_COND_LUCKY_CHANT, gSideTimers[side].luckyChantTimer, FALSE);
        colors[count++] = BINFO_EFFECT_COLOR_HELPFUL;
    }

    if (count < BINFO_FIELD_PAGE_MAX_LINES)
    {
        u8 hazardLines[BINFO_FIELD_PAGE_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN];
        u8 hazardCount = BattleInfo_CollectHazardLines(side, hazardLines);
        u8 i;

        for (i = 0; i < hazardCount && count < BINFO_FIELD_PAGE_MAX_LINES; i++)
        {
            StringCopy(lines[count], hazardLines[i]);
            colors[count++] = BINFO_EFFECT_COLOR_HAZARD;
        }
    }

    return count;
}

static void BattleInfo_PrintEffectLines(struct BattleInfoMenu *data,
                                        u8 lines[BINFO_FIELD_PAGE_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN],
                                        u8 count, s16 x, s16 yStart)
{
    u8 i;

    for (i = 0; i < count; i++)
    {
        BattleInfo_PrintText(data->contentWindowId, FONT_SMALL, lines[i], x,
                             yStart + (i * BINFO_FIELD_EFFECT_LINE_HEIGHT), 0, NULL);
    }
}

static void BattleInfo_PrintEffectLinesColored(struct BattleInfoMenu *data,
                                               u8 lines[BINFO_FIELD_PAGE_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN],
                                               u8 colors[BINFO_FIELD_PAGE_MAX_LINES],
                                               u8 count, s16 x, s16 yStart)
{
    u8 i;
    const u8 *textColors;

    for (i = 0; i < count; i++)
    {
        switch (colors[i])
        {
        case BINFO_EFFECT_COLOR_HELPFUL:
            textColors = sBattleInfoTextColors_Green;
            break;
        case BINFO_EFFECT_COLOR_HAZARD:
            textColors = sBattleInfoTextColors_Red;
            break;
        default:
            textColors = sBattleInfoTextColors_Black;
            break;
        }

        BattleInfo_PrintTextColored(data->contentWindowId, FONT_SMALL, lines[i], x,
                                    yStart + (i * BINFO_FIELD_EFFECT_LINE_HEIGHT), textColors);
    }
}

static void BattleInfo_AppendTurnCount(u8 *dst, u16 turns, bool8 showLeft)
{
    StringAppend(dst, COMPOUND_STRING(" ("));
    ConvertIntToDecimalStringN(dst + StringLength(dst), turns, STR_CONV_MODE_LEFT_ALIGN, 2);
    if (turns == 1)
    {
        if (showLeft)
            StringAppend(dst, COMPOUND_STRING(BINFO_FIELD_TURNS_LEFT_SINGULAR));
        else
            StringAppend(dst, COMPOUND_STRING(BINFO_FIELD_TURNS_PASSED_SINGULAR));
    }
    else
    {
        if (showLeft)
            StringAppend(dst, COMPOUND_STRING(BINFO_FIELD_TURNS_LEFT_PLURAL));
        else
            StringAppend(dst, COMPOUND_STRING(BINFO_FIELD_TURNS_PASSED_PLURAL));
    }
}

static void BattleInfo_AppendFieldConditionTurns(u8 *dst, enum BattleInfoFieldCondition condition, u16 timer, bool8 variableDuration)
{
    if (timer == 0)
        return;

    if (BattleInfo_IsFieldDurationKnown(condition, variableDuration))
        BattleInfo_AppendTurnCount(dst, timer, TRUE);
    else
        BattleInfo_AppendTurnCount(dst, BattleInfo_GetFieldTurnsPassed(condition), FALSE);
}

static void BattleInfo_AppendSideConditionTurns(u8 *dst, u8 side, enum BattleInfoSideCondition condition, u16 timer, bool8 variableDuration)
{
    if (timer == 0)
        return;

    if (BattleInfo_IsSideDurationKnown(side, condition, variableDuration))
        BattleInfo_AppendTurnCount(dst, timer, TRUE);
    else
        BattleInfo_AppendTurnCount(dst, BattleInfo_GetSideTurnsPassed(side, condition), FALSE);
}

static u16 BattleInfo_GetCurrentTurn(void)
{
    return gBattleTurnCounter + 1;
}

static u16 BattleInfo_GetTurnsPassed(u16 startTurn)
{
    u16 currentTurn = BattleInfo_GetCurrentTurn();

    if (startTurn == 0 || currentTurn < startTurn)
        return 0;

    return currentTurn - startTurn;
}

static u16 BattleInfo_GetFieldTurnsPassed(enum BattleInfoFieldCondition condition)
{
    if (gBattleStruct == NULL)
        return 0;

    return BattleInfo_GetTurnsPassed(gBattleStruct->battleInfo.fieldConditionStartTurns[condition]);
}

static u16 BattleInfo_GetSideTurnsPassed(u8 side, enum BattleInfoSideCondition condition)
{
    if (gBattleStruct == NULL || side >= NUM_BATTLE_SIDES)
        return 0;

    return BattleInfo_GetTurnsPassed(gBattleStruct->battleInfo.sideConditionStartTurns[side][condition]);
}

static u8 BattleInfo_GetFieldConditionBattler(enum BattleInfoFieldCondition condition)
{
    u8 battler;

    if (gBattleStruct == NULL)
        return MAX_BATTLERS_COUNT;

    battler = gBattleStruct->battleInfo.fieldConditionBattlers[condition];
    if (battler == 0)
        return MAX_BATTLERS_COUNT;

    return battler - 1;
}

static u8 BattleInfo_GetSideConditionBattler(u8 side, enum BattleInfoSideCondition condition)
{
    u8 battler;

    if (gBattleStruct == NULL || side >= NUM_BATTLE_SIDES)
        return MAX_BATTLERS_COUNT;

    battler = gBattleStruct->battleInfo.sideConditionBattlers[side][condition];
    if (battler == 0)
        return MAX_BATTLERS_COUNT;

    return battler - 1;
}

static bool8 BattleInfo_IsBattlerItemKnown(u32 battler)
{
    u8 slot;

    if (gBattleStruct == NULL || battler >= MAX_BATTLERS_COUNT)
        return FALSE;
    if (GetBattlerSide(battler) == B_SIDE_PLAYER)
        return TRUE;

    slot = gBattlerPartyIndexes[battler];
    return gBattleStruct->battleInfo.mon[B_SIDE_OPPONENT][slot].itemId != ITEM_NONE;
}

static bool8 BattleInfo_IsFieldDurationKnown(enum BattleInfoFieldCondition condition, bool8 variableDuration)
{
    u8 battler;

    if (!variableDuration)
        return TRUE;

    battler = BattleInfo_GetFieldConditionBattler(condition);
    if (battler >= MAX_BATTLERS_COUNT)
        return FALSE;

    return BattleInfo_IsBattlerItemKnown(battler);
}

static bool8 BattleInfo_IsSideDurationKnown(u8 side, enum BattleInfoSideCondition condition, bool8 variableDuration)
{
    u8 battler;

    if (side == B_SIDE_PLAYER || !variableDuration)
        return TRUE;

    battler = BattleInfo_GetSideConditionBattler(side, condition);
    if (battler >= MAX_BATTLERS_COUNT)
        return FALSE;

    return BattleInfo_IsBattlerItemKnown(battler);
}

static void BattleInfo_OpenAbilityPopup(struct BattleInfoMenu *data, enum Ability ability)
{
    u8 y = 1;

    if (data->abilityWindowId != WINDOW_NONE)
        return;

    data->abilityWindowId = AddWindow(&sAbilityWindowTemplate);
    PutWindowTilemap(data->abilityWindowId);
    FillWindowPixelBuffer(data->abilityWindowId, PIXEL_FILL(0));

    BattleInfo_PrintText(data->abilityWindowId, FONT_NORMAL, gAbilitiesInfo[ability].name, 2, y, 0, NULL);
    y += 16;
    BattleInfo_PrintText(data->abilityWindowId, FONT_SMALL, gAbilitiesInfo[ability].description, 2, y, 0, NULL);
    CopyWindowToVram(data->abilityWindowId, COPYWIN_FULL);
    data->abilityMode = ABILITY_MODE_POPUP;
}

static void BattleInfo_CloseAbilityPopup(struct BattleInfoMenu *data)
{
    if (data->abilityWindowId == WINDOW_NONE)
        return;

    ClearStdWindowAndFrameToTransparent(data->abilityWindowId, TRUE);
    RemoveWindow(data->abilityWindowId);
    data->abilityWindowId = WINDOW_NONE;
}

static void BattleInfo_MoveEnemySlot(s8 delta, u8 maxCount)
{
    s8 slot = gBattleStruct->battleInfo.enemySlot;

    if (maxCount == 0)
        return;

    slot += delta;
    if (slot < 0)
        slot = maxCount - 1;
    else if (slot >= maxCount)
        slot = 0;

    gBattleStruct->battleInfo.enemySlot = slot;
}

static u8 BattleInfo_GetEnemyPartyCount(void)
{
    u8 count = 0;

    if (gAiPartyData != NULL)
        count = gAiPartyData->count[B_SIDE_OPPONENT];
    if (count == 0)
        count = CalculateEnemyPartyCount();
    if (count > PARTY_SIZE)
        count = PARTY_SIZE;
    return count;
}

static u8 BattleInfo_GetEnemyPartySlotByDisplaySlot(u8 displaySlot)
{
    u8 count = BattleInfo_GetEnemyPartyCount();
    u8 seenCount = gBattleStruct->battleInfo.seenOrderCount[B_SIDE_OPPONENT];
    u8 i;

    if (count == 0)
        return 0;
    if (seenCount > count)
        seenCount = count;

    if (displaySlot < seenCount)
        return gBattleStruct->battleInfo.seenOrder[B_SIDE_OPPONENT][displaySlot];

    displaySlot -= seenCount;
    for (i = 0; i < count; i++)
    {
        if (gBattleStruct->battleInfo.mon[B_SIDE_OPPONENT][i].seen)
            continue;
        if (displaySlot == 0)
            return i;
        displaySlot--;
    }

    return 0;
}

static bool32 BattleInfo_IsEnemySlotSeen(u8 slot)
{
    if (slot >= PARTY_SIZE)
        return FALSE;
    return gBattleStruct->battleInfo.mon[B_SIDE_OPPONENT][slot].seen;
}

static u8 BattleInfo_GetGenderDisplay(u8 battlerGender)
{
    if (battlerGender == MON_FEMALE)
        return BINFO_GENDER_FEMALE;
    if (battlerGender == MON_GENDERLESS)
        return BINFO_GENDERLESS;
    return BINFO_GENDER_MALE;
}

static const u8 *BattleInfo_GetGenderText(u8 gender)
{
    switch (gender)
    {
    case BINFO_GENDER_FEMALE:
        return gText_FemaleSymbol;
    case BINFO_GENDERLESS:
        return gText_GenderlessSymbol;
    default:
        return gText_MaleSymbol;
    }
}

static void BattleInfo_FormatBarOnly(u8 *dst, u8 hpFraction, u8 segments)
{
    u8 filled;
    u8 i;

    if (segments == 0)
    {
        dst[0] = EOS;
        return;
    }

    filled = (hpFraction * segments + (BINFO_HP_BAR_SCALE - 1)) / BINFO_HP_BAR_SCALE;
    if (filled > segments)
        filled = segments;

    for (i = 0; i < segments; i++)
        dst[i] = (i < filled) ? CHAR_EQUALS : CHAR_PERIOD;

    dst[segments] = EOS;
}

static void BattleInfo_FormatStatus(u8 *dst, u32 status1)
{
    u8 *ptr;
    u8 value;
    u8 numBuf[4];

    if (status1 & STATUS1_SLEEP)
    {
        ptr = StringCopy(dst, sText_StatusSLP);
        value = status1 & STATUS1_SLEEP;
        ConvertIntToDecimalStringN(numBuf, value, STR_CONV_MODE_LEFT_ALIGN, 1);
        StringAppend(ptr, numBuf);
        return;
    }

    if (status1 & STATUS1_TOXIC_POISON)
    {
        ptr = StringCopy(dst, sText_StatusTOX);
        value = (status1 & STATUS1_TOXIC_COUNTER) >> 8;
        ConvertIntToDecimalStringN(numBuf, value, STR_CONV_MODE_LEFT_ALIGN, 1);
        StringAppend(ptr, numBuf);
        return;
    }

    if (status1 & STATUS1_POISON)
        StringCopy(dst, sText_StatusPSN);
    else if (status1 & STATUS1_BURN)
        StringCopy(dst, sText_StatusBRN);
    else if (status1 & STATUS1_FREEZE)
        StringCopy(dst, sText_StatusFRZ);
    else if (status1 & STATUS1_FROSTBITE)
        StringCopy(dst, sText_StatusFRB);
    else if (status1 & STATUS1_PARALYSIS)
        StringCopy(dst, sText_StatusPAR);
    else
        StringCopy(dst, sText_StatusOK);
}

static u8 BattleInfo_GetAbilityList(u16 species, enum Ability *abilities)
{
    u8 count = 0;
    u32 i;

    for (i = 0; i < NUM_ABILITY_SLOTS; i++)
    {
        enum Ability ability = GetSpeciesAbility(species, i);
        u32 j;
        bool8 duplicate = FALSE;

        if (ability == ABILITY_NONE)
            continue;
        for (j = 0; j < count; j++)
        {
            if (abilities[j] == ability)
            {
                duplicate = TRUE;
                break;
            }
        }
        if (!duplicate)
            abilities[count++] = ability;
    }

    return count;
}

static enum Ability BattleInfo_GetConfirmedAbility(u8 slot, const enum Ability *abilities, u8 abilityCount)
{
    enum Ability known = gAiPartyData->mons[B_SIDE_OPPONENT][slot].ability;

    if (known != ABILITY_NONE)
        return known;
    if (abilityCount == 1)
        return abilities[0];
    return ABILITY_NONE;
}

static void BattleInfo_PrintEnemyHeader(struct BattleInfoMenu *data, u8 *y, u8 slot)
{
    u8 text[64];
    struct BattleInfoMon *info = &gBattleStruct->battleInfo.mon[B_SIDE_OPPONENT][slot];

    *y += 16;

    if (info->seen)
    {
        StringCopy(text, sText_SpeciesLine);
        StringAppend(text, GetSpeciesName(info->species));
        StringAppend(text, COMPOUND_STRING(" "));
        StringAppend(text, BattleInfo_GetGenderText(info->gender));
        BattleInfo_PrintText(data->contentWindowId, FONT_NORMAL, text, 0, *y, 0, NULL);
        *y += 16;
    }
}

static u8 BattleInfo_GetAilmentFromStatus(const struct BattleInfoMon *info)
{
    if (info->fainted)
        return AILMENT_FNT;
    if (info->status1 & STATUS1_PSN_ANY)
        return AILMENT_PSN;
    if (info->status1 & STATUS1_PARALYSIS)
        return AILMENT_PRZ;
    if (info->status1 & STATUS1_SLEEP)
        return AILMENT_SLP;
    if (info->status1 & STATUS1_FREEZE)
        return AILMENT_FRZ;
    if (info->status1 & STATUS1_BURN)
        return AILMENT_BRN;
    if (info->status1 & STATUS1_FROSTBITE)
        return AILMENT_FRB;
    return AILMENT_NONE;
}

static u8 BattleInfo_GetAilmentFromStatus1(u32 status1)
{
    if (status1 & STATUS1_PSN_ANY)
        return AILMENT_PSN;
    if (status1 & STATUS1_PARALYSIS)
        return AILMENT_PRZ;
    if (status1 & STATUS1_SLEEP)
        return AILMENT_SLP;
    if (status1 & STATUS1_FREEZE)
        return AILMENT_FRZ;
    if (status1 & STATUS1_BURN)
        return AILMENT_BRN;
    if (status1 & STATUS1_FROSTBITE)
        return AILMENT_FRB;
    return AILMENT_NONE;
}

static void BattleInfo_DrawStatusIconSprite(struct BattleInfoMenu *data, const struct BattleInfoMon *info, s16 x, s16 y)
{
    u8 tileLeft = GetWindowAttribute(data->contentWindowId, WINDOW_TILEMAP_LEFT);
    u8 tileTop = GetWindowAttribute(data->contentWindowId, WINDOW_TILEMAP_TOP);
    u8 ailment = BattleInfo_GetAilmentFromStatus(info);

    BattleInfo_EnsureStatusIconSprite(data);
    if (data->statusIconSpriteId == MAX_SPRITES)
        return;

    if (ailment == AILMENT_NONE)
    {
        gSprites[data->statusIconSpriteId].invisible = TRUE;
        return;
    }

    gSprites[data->statusIconSpriteId].x = (tileLeft * 8) + x + BINFO_HP_BAR_TOTAL_WIDTH + BINFO_STATUS_INDICATOR_GAP + 16;
    gSprites[data->statusIconSpriteId].y = (tileTop * 8) + y + BINFO_STATUS_INDICATOR_Y_OFFSET;
    StartSpriteAnim(&gSprites[data->statusIconSpriteId], ailment - 1);
    gSprites[data->statusIconSpriteId].invisible = FALSE;
}

static void BattleInfo_DrawHpBarSprite(struct BattleInfoMenu *data, s16 x, s16 y, u8 hpFraction)
{
    u8 tileLeft = GetWindowAttribute(data->contentWindowId, WINDOW_TILEMAP_LEFT);
    u8 tileTop = GetWindowAttribute(data->contentWindowId, WINDOW_TILEMAP_TOP);

    BattleInfo_EnsureHpBarSprite(data);
    if (data->hpBarSpriteId == MAX_SPRITES)
        return;

    BattleInfo_UpdateHpBarTiles(data->hpBarSpriteId, hpFraction);
    gSprites[data->hpBarSpriteId].x = (tileLeft * 8) + x + 16;
    gSprites[data->hpBarSpriteId].y = (tileTop * 8) + y;
    gSprites[data->hpBarSpriteId].invisible = FALSE;
}

static void BattleInfo_UpdateHpBarTiles(u8 spriteId, u8 hpFraction)
{
    u8 pixels = hpFraction;
    u8 filledPixels = hpFraction;
    u8 barElementId;
    u8 i;
    u8 maxTiles = BINFO_HP_BAR_SCALE / BINFO_HP_BAR_PIXELS_PER_TILE;
    u16 tileNum = gSprites[spriteId].oam.tileNum;

    if (filledPixels > BINFO_HP_BAR_SCALE)
        filledPixels = BINFO_HP_BAR_SCALE;
    pixels = filledPixels;

    if (filledPixels > (BINFO_HP_BAR_SCALE * 50 / 100))
        barElementId = BINFO_HEALTHBOX_GFX_HP_BAR_GREEN;
    else if (filledPixels > (BINFO_HP_BAR_SCALE * 20 / 100))
        barElementId = BINFO_HEALTHBOX_GFX_HP_BAR_YELLOW;
    else
        barElementId = BINFO_HEALTHBOX_GFX_HP_BAR_RED;

    CpuCopy32((const void *)gHealthboxElementsGfxTable[BINFO_HEALTHBOX_GFX_HP_BAR_H],
              (void *)(OBJ_VRAM0 + tileNum * TILE_SIZE_4BPP),
              TILE_SIZE_4BPP * 2);

    for (i = 0; i < maxTiles; i++)
    {
        u8 fill = 0;

        if (pixels >= BINFO_HP_BAR_PIXELS_PER_TILE)
        {
            fill = BINFO_HP_BAR_PIXELS_PER_TILE;
            pixels -= BINFO_HP_BAR_PIXELS_PER_TILE;
        }
        else
        {
            fill = pixels;
            pixels = 0;
        }

        if (i < 2)
        {
            CpuCopy32((const void *)gHealthboxElementsGfxTable[barElementId + fill],
                      (void *)(OBJ_VRAM0 + (tileNum + 2 + i) * TILE_SIZE_4BPP),
                      TILE_SIZE_4BPP);
        }
        else
        {
            CpuCopy32((const void *)gHealthboxElementsGfxTable[barElementId + fill],
                      (void *)(OBJ_VRAM0 + 64 + (tileNum + i) * TILE_SIZE_4BPP),
                      TILE_SIZE_4BPP);
        }
    }

}

static void BattleInfo_DrawTypeLineAndIcons(struct BattleInfoMenu *data, u16 species, s16 x, s16 y)
{
    s16 tileLeft = GetWindowAttribute(data->contentWindowId, WINDOW_TILEMAP_LEFT);
    s16 tileTop = GetWindowAttribute(data->contentWindowId, WINDOW_TILEMAP_TOP);
    s16 iconX = (tileLeft * 8) + x + GetStringWidth(FONT_NORMAL, sText_TypeLine, 0) + BINFO_TYPE_LABEL_GAP + 16;
    s16 iconY = (tileTop * 8) + y + 8;
    enum Type type1 = GetSpeciesType(species, 0);
    enum Type type2 = GetSpeciesType(species, 1);

    AddTextPrinterParameterized4(data->contentWindowId, FONT_NORMAL, x, y, 0, 0, sBattleInfoTextColors_Black, 0, sText_TypeLine);
    BattleInfo_EnsureTypeIcons(data);
    if (!data->typeIconsCreated)
        return;

    if (data->typeIconSpriteIds[0] != MAX_SPRITES)
    {
        struct Sprite *sprite = &gSprites[data->typeIconSpriteIds[0]];
        StartSpriteAnim(sprite, type1);
        sprite->oam.paletteNum = gTypesInfo[type1].palette;
        sprite->x = iconX;
        sprite->y = iconY;
        sprite->invisible = FALSE;
    }

    if (type1 == type2)
    {
        if (data->typeIconSpriteIds[1] != MAX_SPRITES)
            gSprites[data->typeIconSpriteIds[1]].invisible = TRUE;
    }
    else if (data->typeIconSpriteIds[1] != MAX_SPRITES)
    {
        struct Sprite *sprite = &gSprites[data->typeIconSpriteIds[1]];
        StartSpriteAnim(sprite, type2);
        sprite->oam.paletteNum = gTypesInfo[type2].palette;
        sprite->x = iconX + BINFO_TYPE_ICON_WIDTH + BINFO_TYPE_ICON_GAP;
        sprite->y = iconY;
        sprite->invisible = FALSE;
    }
}

static void BattleInfo_DrawTypeLineAndIconsUnknown(struct BattleInfoMenu *data, s16 x, s16 y)
{
    s16 tileLeft = GetWindowAttribute(data->contentWindowId, WINDOW_TILEMAP_LEFT);
    s16 tileTop = GetWindowAttribute(data->contentWindowId, WINDOW_TILEMAP_TOP);
    s16 iconX = (tileLeft * 8) + x + GetStringWidth(FONT_NORMAL, sText_TypeLine, 0) + BINFO_TYPE_LABEL_GAP + 16;
    s16 iconY = (tileTop * 8) + y + 8;

    AddTextPrinterParameterized4(data->contentWindowId, FONT_NORMAL, x, y, 0, 0, sBattleInfoTextColors_Black, 0, sText_TypeLine);
    BattleInfo_EnsureTypeIcons(data);
    if (!data->typeIconsCreated)
        return;

    if (data->typeIconSpriteIds[0] != MAX_SPRITES)
    {
        struct Sprite *sprite = &gSprites[data->typeIconSpriteIds[0]];
        StartSpriteAnim(sprite, TYPE_MYSTERY);
        sprite->oam.paletteNum = gTypesInfo[TYPE_MYSTERY].palette;
        sprite->x = iconX;
        sprite->y = iconY;
        sprite->invisible = FALSE;
    }

    if (data->typeIconSpriteIds[1] != MAX_SPRITES)
        gSprites[data->typeIconSpriteIds[1]].invisible = TRUE;
}

static void UNUSED BattleInfo_PrintActiveBattler(u8 windowId, u8 *y, u32 battler)
{
    u8 text[64];
    u8 barText[16];
    u8 statusText[16];
    u8 name[POKEMON_NAME_LENGTH + 1];
    const u8 *prefix;

    if (battler >= gBattlersCount)
        return;
    if (gAbsentBattlerFlags & (1u << battler))
        return;

    switch (GetBattlerPosition(battler))
    {
    case B_POSITION_PLAYER_LEFT:
        prefix = sText_P1;
        break;
    case B_POSITION_PLAYER_RIGHT:
        prefix = sText_P2;
        break;
    case B_POSITION_OPPONENT_LEFT:
        prefix = sText_O1;
        break;
    default:
        prefix = sText_O2;
        break;
    }

    StringCopyN(name, GetSpeciesName(gBattleMons[battler].species), BINFO_NAME_LIMIT);
    name[BINFO_NAME_LIMIT] = EOS;
    BattleInfo_FormatBarOnly(barText, GetScaledHPFraction(gBattleMons[battler].hp, gBattleMons[battler].maxHP, BINFO_HP_BAR_SCALE), BINFO_HP_BAR_SEGMENTS_FIELD);
    BattleInfo_FormatStatus(statusText, gBattleMons[battler].status1);

    StringCopy(text, prefix);
    StringAppend(text, COMPOUND_STRING(" "));
    StringAppend(text, name);
    StringAppend(text, COMPOUND_STRING(" "));
    StringAppend(text, barText);
    StringAppend(text, COMPOUND_STRING(" "));
    StringAppend(text, statusText);
    BattleInfo_PrintText(windowId, FONT_SMALL, text, 0, *y, 0, NULL);
    *y += 10;

    BattleInfo_FormatStageLine(text, battler);
    BattleInfo_PrintText(windowId, FONT_SMALL, text, 0, *y, 0, NULL);
    *y += 10;

    BattleInfo_FormatVolatileLine(text, battler);
    BattleInfo_PrintText(windowId, FONT_SMALL, text, 0, *y, 0, NULL);
    *y += 12;
}

static void BattleInfo_FormatStageLine(u8 *dst, u32 battler)
{
    s8 stage;
    bool8 any = FALSE;
    u8 *ptr = StringCopy(dst, sText_StagesPrefix);
    u8 numBuf[4];

    struct
    {
        const u8 *label;
        enum Stat stat;
    } stats[] =
    {
        {sText_StageAtk, STAT_ATK},
        {sText_StageDef, STAT_DEF},
        {sText_StageSpAtk, STAT_SPATK},
        {sText_StageSpDef, STAT_SPDEF},
        {sText_StageSpe, STAT_SPEED},
        {sText_StageAcc, STAT_ACC},
        {sText_StageEva, STAT_EVASION},
    };
    u32 i;

    for (i = 0; i < ARRAY_COUNT(stats); i++)
    {
        stage = gBattleMons[battler].statStages[stats[i].stat] - DEFAULT_STAT_STAGE;
        if (stage == 0)
            continue;
        if (any)
            *ptr++ = CHAR_SPACE;
        ptr = StringCopy(ptr, stats[i].label);
        *ptr++ = (stage > 0) ? CHAR_PLUS : CHAR_HYPHEN;
        ConvertIntToDecimalStringN(numBuf, (stage < 0) ? -stage : stage, STR_CONV_MODE_LEFT_ALIGN, 1);
        ptr = StringCopy(ptr, numBuf);
        any = TRUE;
    }

    if (!any)
        StringAppend(dst, sText_None);
}

static void BattleInfo_FormatVolatileLine(u8 *dst, u32 battler)
{
    bool8 any = FALSE;
    u8 *ptr = StringCopy(dst, sText_VolatilePrefix);
    u8 numBuf[4];

    if (gBattleMons[battler].volatiles.confusionTurns > 0 || gBattleMons[battler].volatiles.infiniteConfusion)
    {
        ptr = StringCopy(ptr, sText_Conf);
        any = TRUE;
    }
    if (gBattleMons[battler].volatiles.leechSeed)
    {
        if (any)
            *ptr++ = CHAR_SPACE;
        ptr = StringCopy(ptr, sText_Leech);
        any = TRUE;
    }
    if (gDisableStructs[battler].perishSongTimer)
    {
        if (any)
            *ptr++ = CHAR_SPACE;
        ptr = StringCopy(ptr, sText_Per);
        ConvertIntToDecimalStringN(numBuf, gDisableStructs[battler].perishSongTimer, STR_CONV_MODE_LEFT_ALIGN, 1);
        ptr = StringCopy(ptr, numBuf);
        any = TRUE;
    }

    if (!any)
        StringAppend(dst, sText_None);
}

static u8 BattleInfo_CollectFieldEffects(u8 lines[BINFO_FIELD_EFFECT_MAX_LINES][BINFO_FIELD_EFFECT_LINE_LEN], u32 battler)
{
    u8 count = 0;

    if (gBattleMons[battler].volatiles.confusionTurns > 0 || gBattleMons[battler].volatiles.infiniteConfusion)
    {
        if (count < BINFO_FIELD_EFFECT_MAX_LINES)
            StringCopy(lines[count++], sText_EffectConfused);
    }

    if (gBattleMons[battler].volatiles.infatuation)
    {
        if (count < BINFO_FIELD_EFFECT_MAX_LINES)
            StringCopy(lines[count++], sText_EffectInfatuated);
    }

    {
        enum Type types[3];
        enum Type baseType1 = GetSpeciesType(gBattleMons[battler].species, 0);
        enum Type baseType2 = GetSpeciesType(gBattleMons[battler].species, 1);
        bool8 typeChanged;

        GetBattlerTypes(battler, TRUE, types);
        typeChanged = (types[2] != TYPE_MYSTERY) || (types[0] != baseType1) || (types[1] != baseType2);
        if (typeChanged && count < BINFO_FIELD_EFFECT_MAX_LINES)
        {
            u8 *dst = lines[count++];
            bool8 hasThirdType = (types[2] != TYPE_MYSTERY && types[2] != types[0] && types[2] != types[1]);

            if (hasThirdType)
                dst = StringCopy(dst, sText_EffectTypeAddPrefix);
            else
                dst = StringCopy(dst, sText_EffectTypePrefix);

            if (hasThirdType)
            {
                StringCopy(dst, gTypesInfo[types[2]].name);
            }
            else if (types[0] == types[1] || types[1] == TYPE_MYSTERY)
            {
                StringCopy(dst, gTypesInfo[types[0]].name);
            }
            else
            {
                dst = StringCopy(dst, gTypesInfo[types[0]].name);
                dst = StringCopy(dst, COMPOUND_STRING("/"));
                StringCopy(dst, gTypesInfo[types[1]].name);
            }
        }
    }

    if (gBattleMons[battler].volatiles.leechSeed)
    {
        if (count < BINFO_FIELD_EFFECT_MAX_LINES)
            StringCopy(lines[count++], sText_EffectLeechSeed);
    }

    if (gDisableStructs[battler].perishSongTimer)
    {
        if (count < BINFO_FIELD_EFFECT_MAX_LINES)
        {
            u8 *dst = lines[count++];
            dst = StringCopy(dst, sText_EffectPerish);
            ConvertIntToDecimalStringN(dst, gDisableStructs[battler].perishSongTimer, STR_CONV_MODE_LEFT_ALIGN, 1);
        }
    }

    return count;
}

static void BattleInfo_BuildWeatherLine(u8 *dst)
{
    const u8 *weatherText = sText_WeatherNone;
    u8 timer = gWishFutureKnock.weatherDuration;

    if (gBattleWeather & B_WEATHER_RAIN_PRIMAL)
        weatherText = sText_WeatherHeavyRain;
    else if (gBattleWeather & B_WEATHER_SUN_PRIMAL)
        weatherText = sText_WeatherHarshSun;
    else if (gBattleWeather & B_WEATHER_STRONG_WINDS)
        weatherText = sText_WeatherStrongWinds;
    else if (gBattleWeather & B_WEATHER_RAIN)
        weatherText = sText_WeatherRain;
    else if (gBattleWeather & B_WEATHER_SUN)
        weatherText = sText_WeatherSun;
    else if (gBattleWeather & B_WEATHER_SANDSTORM)
        weatherText = sText_WeatherSand;
    else if (gBattleWeather & B_WEATHER_HAIL)
        weatherText = sText_WeatherHail;
    else if (gBattleWeather & B_WEATHER_SNOW)
        weatherText = sText_WeatherSnow;
    else if (gBattleWeather & B_WEATHER_FOG)
        weatherText = sText_WeatherFog;

    StringCopy(dst, sText_WeatherPrefix);
    StringAppend(dst, weatherText);
    if ((gBattleWeather & B_WEATHER_ANY) && timer != 0)
    {
        bool8 showLeft = BattleInfo_IsFieldDurationKnown(BINFO_FIELD_COND_WEATHER, TRUE);
        u16 turns = showLeft ? timer : BattleInfo_GetFieldTurnsPassed(BINFO_FIELD_COND_WEATHER);

        BattleInfo_AppendTurnCount(dst, turns, showLeft);
    }
}

static void BattleInfo_BuildTerrainLine(u8 *dst)
{
    const u8 *terrainText = sText_TerrainNone;
    u8 timer = gFieldTimers.terrainTimer;

    if (gFieldStatuses & STATUS_FIELD_GRASSY_TERRAIN)
        terrainText = sText_TerrainGrassy;
    else if (gFieldStatuses & STATUS_FIELD_MISTY_TERRAIN)
        terrainText = sText_TerrainMisty;
    else if (gFieldStatuses & STATUS_FIELD_ELECTRIC_TERRAIN)
        terrainText = sText_TerrainElectric;
    else if (gFieldStatuses & STATUS_FIELD_PSYCHIC_TERRAIN)
        terrainText = sText_TerrainPsychic;

    StringCopy(dst, sText_TerrainPrefix);
    StringAppend(dst, terrainText);
    if ((gFieldStatuses & STATUS_FIELD_TERRAIN_ANY) && timer != 0)
    {
        bool8 showLeft = BattleInfo_IsFieldDurationKnown(BINFO_FIELD_COND_TERRAIN, TRUE);
        u16 turns = showLeft ? timer : BattleInfo_GetFieldTurnsPassed(BINFO_FIELD_COND_TERRAIN);

        BattleInfo_AppendTurnCount(dst, turns, showLeft);
    }
}

void BattleInfo_RecordSwitchIn(u32 battler)
{
    struct BattleInfoMon *info;
    u8 side;
    u8 slot;

    if (gBattleStruct == NULL)
        return;

    side = GetBattlerSide(battler);
    slot = gBattlerPartyIndexes[battler];
    info = &gBattleStruct->battleInfo.mon[side][slot];
    if (!info->seen)
    {
        u8 seenCount = gBattleStruct->battleInfo.seenOrderCount[side];

        if (seenCount < PARTY_SIZE)
        {
            gBattleStruct->battleInfo.seenOrder[side][seenCount] = slot;
            gBattleStruct->battleInfo.seenOrderCount[side] = seenCount + 1;
        }
    }
    info->species = gBattleMons[battler].species;
    info->personality = gBattleMons[battler].personality;
    info->status1 = gBattleMons[battler].status1;
    info->hpFraction = GetScaledHPFraction(gBattleMons[battler].hp, gBattleMons[battler].maxHP, BINFO_HP_BAR_SCALE);
    info->gender = BattleInfo_GetGenderDisplay(GetBattlerGender(battler));
    info->shiny = gBattleMons[battler].isShiny;
    info->seen = TRUE;
    info->fainted = FALSE;
}

void BattleInfo_RecordFaint(u32 battler)
{
    struct BattleInfoMon *info;

    if (gBattleStruct == NULL)
        return;

    info = &gBattleStruct->battleInfo.mon[GetBattlerSide(battler)][gBattlerPartyIndexes[battler]];
    info->fainted = TRUE;
    info->hpFraction = 0;
}

void BattleInfo_RecordHp(u32 battler)
{
    struct BattleInfoMon *info;

    if (gBattleStruct == NULL)
        return;

    info = &gBattleStruct->battleInfo.mon[GetBattlerSide(battler)][gBattlerPartyIndexes[battler]];
    info->hpFraction = GetScaledHPFraction(gBattleMons[battler].hp, gBattleMons[battler].maxHP, BINFO_HP_BAR_SCALE);
}

void BattleInfo_RecordStatus(u32 battler)
{
    struct BattleInfoMon *info;

    if (gBattleStruct == NULL)
        return;

    info = &gBattleStruct->battleInfo.mon[GetBattlerSide(battler)][gBattlerPartyIndexes[battler]];
    info->status1 = gBattleMons[battler].status1;
}

void BattleInfo_RecordKnownMove(u32 battler, u16 move, u8 moveIndex)
{
    struct BattleInfoMon *info;

    if (gBattleStruct == NULL)
        return;
    if (move == MOVE_NONE || moveIndex >= MAX_MON_MOVES)
        return;
    if (gBattleMons[battler].moves[moveIndex] != move)
        return;

    info = &gBattleStruct->battleInfo.mon[GetBattlerSide(battler)][gBattlerPartyIndexes[battler]];
    info->knownMoves[moveIndex] = move;
}

void BattleInfo_RecordMoveUsed(u32 battler, u16 move, u8 moveIndex)
{
    struct BattleInfoMon *info;

    if (gBattleStruct == NULL)
        return;
    if (move == MOVE_NONE || moveIndex >= MAX_MON_MOVES)
        return;
    if (gBattleMons[battler].moves[moveIndex] != move)
        return;

    BattleInfo_RecordKnownMove(battler, move, moveIndex);
    info = &gBattleStruct->battleInfo.mon[GetBattlerSide(battler)][gBattlerPartyIndexes[battler]];
    if (info->ppUsed[moveIndex] < 0xFE)
        info->ppUsed[moveIndex]++;
}

void BattleInfo_RecordItem(u32 battler, u16 itemId)
{
    struct BattleInfoMon *info;

    if (gBattleStruct == NULL)
        return;
    if (itemId == ITEM_NONE)
        return;

    info = &gBattleStruct->battleInfo.mon[GetBattlerSide(battler)][gBattlerPartyIndexes[battler]];
    info->itemId = itemId;
}

void BattleInfo_RecordFieldCondition(enum BattleInfoFieldCondition condition, u32 battler)
{
    if (gBattleStruct == NULL || condition >= BINFO_FIELD_COND_COUNT)
        return;

    gBattleStruct->battleInfo.fieldConditionStartTurns[condition] = BattleInfo_GetCurrentTurn();
    if (battler < MAX_BATTLERS_COUNT)
        gBattleStruct->battleInfo.fieldConditionBattlers[condition] = battler + 1;
    else
        gBattleStruct->battleInfo.fieldConditionBattlers[condition] = 0;
}

void BattleInfo_RecordSideCondition(u32 side, enum BattleInfoSideCondition condition, u32 battler)
{
    if (gBattleStruct == NULL || side >= NUM_BATTLE_SIDES || condition >= BINFO_SIDE_COND_COUNT)
        return;

    gBattleStruct->battleInfo.sideConditionStartTurns[side][condition] = BattleInfo_GetCurrentTurn();
    if (battler < MAX_BATTLERS_COUNT)
        gBattleStruct->battleInfo.sideConditionBattlers[side][condition] = battler + 1;
    else
        gBattleStruct->battleInfo.sideConditionBattlers[side][condition] = 0;
}
