#include "global.h"
#include "pokemon_storage_system.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "bg.h"
#include "data.h"
#include "daycare.h"
#include "decompress.h"
#include "dexnav.h"
#include "item_menu.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "field_effect.h"
#include "field_effect_helpers.h"
#include "field_message_box.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "fieldmap.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "item.h"
#include "international_string_util.h"
#include "m4a.h"
#include "map_name_popup.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "metatile_behavior.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_icon.h"
#include "pokemon_summary_screen.h"
#include "random.h"
#include "region_map.h"
#include "scanline_effect.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "sound.h"
#include "sprite.h"
#include "start_menu.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "wild_encounter.h"
#include "constants/layouts.h"
#include "window.h"
#include "constants/map_types.h"
#include "constants/species.h"
#include "constants/maps.h"
#include "constants/field_effects.h"
#include "constants/items.h"
#include "constants/songs.h"
#include "constants/abilities.h"
#include "constants/rgb.h"
#include "constants/region_map_sections.h"
#include "gba/m4a_internal.h"

// Defines
enum WindowIds
{
    WINDOW_INFO,
    WINDOW_REGISTERED,
    WINDOW_HINTS,
    WINDOW_COUNT,
};

enum Statuses
{
    STATUS_INVALID_SEARCH,
    STATUS_CHOOSE_MON,
    STATUS_LOCKED,
    STATUS_NO_DATA,
    STATUS_INCORRECT_AREA,
};

struct DexNavSearch
{
    u16 species;
    u16 moves[MAX_MON_MOVES];
    u16 heldItem;
    u8 abilityNum;
    u8 potential;
    u8 searchLevel;
    u8 monLevel;
    u8 proximity;
    u8 environment;
    s16 tileX;
    s16 tileY;
    u8 fldEffSpriteId;
    u8 fldEffId;
    SpriteCallback fldEffCallback; // Gunk's: the patch sprite's callback, to tell it from a reused sprite slot
    u8 movementCount;
    u8 windowId;
    u8 iconSpriteId;
    u8 eyeSpriteId;
    u8 itemSpriteId;
    u8 starSpriteIds[3];
    u8 ownedIconSpriteId;
    u8 exclamationSpriteId;
    u8 hiddenSearch:1;
    u8 isHiddenMon:1;
    u8 revealed:1; // Gunk's: the task's tRevealed, kept while a menu has the task stopped
    u8 gfxHidden:1; // Gunk's: the window and sprites are hidden under the START menu
    u8 unk:4;
    u16 frameCount; // Gunk's: the task's tFrameCount, kept while a menu has the task stopped
    u8 shownSprites; // Gunk's: which search sprites were visible before the START menu hid them
    u16 palBuffer[16];
    struct Pokemon mon; // Gunk's: the Pokemon rolled when the search starts
    struct GunkWildRoll peeked; // Gunk's: what the peek saw of it (tests check the replay made the same Pokemon)
};

struct DexNavGUI
{
    MainCallback savedCallback;
    u8 state;
    u8 cursorSpriteId;
    u16 landSpecies[LAND_WILD_COUNT];
    u16 waterSpecies[WATER_WILD_COUNT];
    u16 hiddenSpecies[HIDDEN_WILD_COUNT];
    u8 cursorRow;
    u8 cursorCol;
    u8 environment;
    u8 potential;
    u8 typeIconSpriteIds[2];
    u8 starSpriteIds[3];
    u16 titleTimer; // Gunk's: during a swarm the title bar takes turns showing it
};

// RAM

EWRAM_DATA static struct DexNavSearch *sDexNavSearchDataPtr = NULL;
EWRAM_DATA static struct DexNavGUI *sDexNavUiDataPtr = NULL;
EWRAM_DATA static u8 *sBg1TilemapBuffer = NULL;
EWRAM_DATA bool8 gDexnavBattle = FALSE;
// Gunk's: the search lives here, not on the heap: the field resets the heap when it comes back from a menu,
// and a search paused by the START menu has to survive the Bag
EWRAM_DATA static struct DexNavSearch sDexNavSearchData = {0};

//// Function Declarations
//GUI
static void Task_DexNavWaitFadeIn(u8 taskId);
static void Task_DexNavMain(u8 taskId);
static void PrintCurrentSpeciesInfo(void);
// SEARCH
static bool8 TryStartHiddenMonFieldEffect(u8 environment, u8 xSize, u8 ySize, bool8 smallScan);
static void DexNavGenerateMoveset(u16 species, u8 searchLevel, u8 encounterLevel, u16* moveDst);
static u16 DexNavGenerateHeldItem(u16 species, u8 searchLevel);
static void GunkDexNavRollMon(u16 species, u8 environment, u8 potential);
static u32 GunkCountPerfectIvs(const struct GunkWildRoll *roll, u32 *total);
static u8 DexNavGeneratePotential(u8 searchLevel);
static u8 DexNavTryGenerateMonLevel(u16 species, u8 environment);
static u8 GetEncounterLevelFromMapData(u16 species, u8 environment);
static void CreateDexNavWildMon(u16 species, u8 potential, u8 level, u8 abilityNum, u16 item, u16* moves);
static u8 GetPlayerDistance(s16 x, s16 y);
static u8 DexNavPickTile(u8 environment, u8 xSize, u8 ySize, bool8 smallScan);
static void DexNavProximityUpdate(void);
static void DexNavDrawIcons(void);
static void DexNavUpdateSearchWindow(u8 proximity, u8 searchLevel);
//static void DexNavSightUpdate(u8 index);
static void Task_DexNavSearch(u8 taskId);
static void EndDexNavSearchSetupScript(const u8 *script, u8 taskId);
// HIDDEN MONS
static void DexNavDrawHiddenIcons(void);
static void DrawHiddenSearchWindow(u8 width);
static void DrawHiddenSearchText(void);

//// Const Data
// gui image data
static const u32 sDexNavGuiTiles[] = INCBIN_U32("graphics/dexnav/gui_tiles.4bpp.lz");
static const u32 sDexNavGuiTilemap[] = INCBIN_U32("graphics/dexnav/gui_tilemap.bin.lz");
static const u32 sDexNavGuiPal[] = INCBIN_U32("graphics/dexnav/gui.gbapal");

static const u32 sSelectionCursorGfx[] = INCBIN_U32("graphics/dexnav/cursor.4bpp.lz");
static const u16 sSelectionCursorPal[] = INCBIN_U16("graphics/dexnav/cursor.gbapal");
static const u32 sCapturedAllMonsTiles[] = INCBIN_U32("graphics/dexnav/captured_all.4bpp.lz");  //uses selection cursor pal

static const u32 sNoDataGfx[] = INCBIN_U32("graphics/dexnav/no_data.4bpp.lz");

// searching image data
static const u32 sPotentialStarGfx[] = INCBIN_U32("graphics/dexnav/star.4bpp.lz");
//static const u32 sEyeGfx[] = INCBIN_U32("graphics/dexnav/vision.4bpp.lz");
static const u32 sHiddenSearchIconGfx[] = INCBIN_U32("graphics/dexnav/hidden_search.4bpp.lz");
static const u32 sOwnedIconGfx[] = INCBIN_U32("graphics/dexnav/owned_icon.4bpp.lz");
static const u32 sHiddenMonIconGfx[] = INCBIN_U32("graphics/dexnav/hidden.4bpp.lz");

// strings
static const u8 sText_DexNav_NoInfo[] = _("--------");
static const u8 sText_DexNav_CaptureToSee[] = _("Capture first!");
static const u8 sText_DexNav_PressRToRegister[] = _("{R_BUTTON} Nothing registered");
static const u8 sText_DexNav_SearchForRegisteredSpecies[] = _("{R_BUTTON} {STR_VAR_1}");
static const u8 sText_DexNav_Caught[] = _("CAUGHT {STR_VAR_1}/{STR_VAR_2}");
static const u8 sText_DexNav_Swarm[] = _("SWARM: {STR_VAR_1} on {STR_VAR_2}");
#define GUNK_SWARM_TITLE_FRAMES 180
static const u8 sText_DexNav_Unknown[] = _("?????");
static const u8 sText_DexNav_StatusCaught[] = _("Caught");
static const u8 sText_DexNav_StatusSeen[] = _("Seen");
static const u8 sText_DexNav_StatusNotSeen[] = _("Not seen yet");
static const u8 sText_DexNav_Found[] = _("Found");
static const u8 sText_DexNav_FoundGrass[] = _("Grass");
static const u8 sText_DexNav_FoundCave[] = _("Cave");
static const u8 sText_DexNav_FoundSurfing[] = _("Surfing");
static const u8 sText_DexNav_FoundHidden[] = _("Hidden");
static const u8 sText_DexNav_Level[] = _("Level");
static const u8 sText_DexNav_LevelRange[] = _("{LV}. {STR_VAR_1}-{STR_VAR_2}");
static const u8 sText_DexNav_LevelOne[] = _("{LV}. {STR_VAR_1}");
static const u8 sText_DexNav_SearchLv[] = _("Search {LV}. {STR_VAR_1}");
static const u8 sText_DexNav_ChainLabel[] = _("Chain");
static const u8 sText_DexNav_HintA[] = _("{A_BUTTON} Search now");
static const u8 sText_DexNav_HintR[] = _("{R_BUTTON} Register");
static const u8 sText_DexNav_HintB[] = _("{B_BUTTON} Close");
static const u8 sText_DexNav_NotFoundHere[] = _("This Pokémon cannot be found here!");
static const u8 sText_ThreeQmarks[] = _("???");
static const u8 sText_SearchLevel[] = _("SEARCH {LV}. {STR_VAR_1}");
static const u8 sText_MonLevel[] = _("{LV}. {STR_VAR_1}");
static const u8 sText_EggMove[] = _("{STR_VAR_1}");  // Gunk's: no "MOVE:", to leave room for the shiny mark
static const u8 sText_DexNav_Shiny[] = _("SHINY");
static const u8 sText_DexNav_BarSearchLevel[] = _("Search {STR_VAR_1}");
static const u8 sText_HeldItem[] = _("{STR_VAR_1}");
static const u8 sText_StartExit[] = _("{START_BUTTON} EXIT");
static const u8 sText_DexNavChain[] = _("{NO} {STR_VAR_1}");
static const u8 sText_DexNavChainLong[] = _("{NO}{STR_VAR_1}");

static const u8 sText_ArrowLeft[] = _("{LEFT_ARROW}");
static const u8 sText_ArrowRight[] = _("{RIGHT_ARROW}");
static const u8 sText_ArrowUp[] = _("{UP_ARROW}");
static const u8 sText_ArrowDown[] = _("{DOWN_ARROW}");

static const struct WindowTemplate sDexNavGuiWindowTemplates[] =
{
    // Gunk's: the screen follows Gunk's Platinum's DexNav: a title bar with the area and its caught count, one info
    // panel for the highlighted species and the buttons under the grids (see GunkDexNavRelayout)
    [WINDOW_INFO] =
    {
        .bg = 0,
        .tilemapLeft = 20,
        .tilemapTop = 2,
        .width = 10,
        .height = 17,
        .paletteNum = 15,
        .baseBlock = 1,
    },
    [WINDOW_REGISTERED] =
    {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 30,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 200,
    },
    [WINDOW_HINTS] =
    {
        .bg = 0,
        .tilemapLeft = 12,
        .tilemapTop = 15,
        .width = 8,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 260,
    },
    DUMMY_WIN_TEMPLATE
};

//gui font
static const u8 sFontColor_Black[3] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};
static const u8 sFontColor_White[3] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY};
//search window font
static const u8 sSearchFontColor[3] = {0, 15, 13};

static const struct OamData sNoDataIconOam =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 1,
};

static const struct OamData sHeldItemOam =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(8x8),
    .size = SPRITE_SIZE(8x8),
    .priority = 0,
    .paletteNum = 13,
};

static const struct OamData sCapturedAllOam =
{
    .y = 0,
    .affineMode = 1,
    .objMode = 0,
    .mosaic = 0,
    .bpp = 0,
    .shape = SPRITE_SHAPE(8x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(8x8),
    .tileNum = 0,
    .priority = 0, //Highest
    .paletteNum = 12,
    .affineParam = 0,
};

static const struct OamData sSearchIconOam =
{
    .y = 0,
    .affineMode = 0,
    .objMode = 0,
    .mosaic = 0,
    .bpp = 0,
    .shape = 0,
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 0, // above BG layers
    .paletteNum = 13,
    .affineParam = 0
};

static const struct OamData sSelectionCursorOam =
{
    .y = 0,
    .affineMode = 0,
    .objMode = 0,
    .mosaic = 0,
    .bpp = 0,
    .shape = 0,
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 0, // above BG layers
    .paletteNum = 12,
    .affineParam = 0
};

static const struct OamData sSightOam =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(16x8),
    .size = SPRITE_SIZE(16x8),
    .priority = 0,
};
static const union AnimCmd sAnimCmdSight0[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END
};
static const union AnimCmd sAnimCmdSight1[] =
{
    ANIMCMD_FRAME(2, 1),
    ANIMCMD_END
};
static const union AnimCmd sAnimCmdSight2[] =
{
    ANIMCMD_FRAME(4, 1),
    ANIMCMD_END
};
static const union AnimCmd *const sAnimCmdTable_Sight[] =
{
    sAnimCmdSight0,
    sAnimCmdSight1,
    sAnimCmdSight2,
};

// gui sprite templates
static const struct SpriteTemplate sNoDataIconTemplate =
{
    .tileTag = ICON_GFX_TAG,
    .paletteTag = ICON_PAL_TAG,
    .oam = &sNoDataIconOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sCaptureAllMonsSpriteTemplate =
{
    .tileTag = CAPTURED_ALL_TAG,
    .paletteTag = 0xFFFF,
    .oam = &sCapturedAllOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sSelectionCursorSpriteTemplate =
{
    .tileTag = SELECTION_CURSOR_TAG,
    .paletteTag = 0xFFFF,
    .oam = &sSelectionCursorOam,
    .anims =  gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

// search window sprite templates
static const struct SpriteTemplate sHeldItemTemplate =
{
    .tileTag = HELD_ITEM_TAG,
    .paletteTag = 0xFFFF,
    .oam = &sHeldItemOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sPotentialStarTemplate =
{
    .tileTag = LIT_STAR_TILE_TAG,
    .paletteTag = 0xFFFF,   //held item pal
    .oam = &sHeldItemOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

/*static const struct SpriteTemplate sSightTemplate =
{
    .tileTag = SIGHT_TAG,
    .paletteTag = 0xFFFF,   //held item pal
    .oam = &sSightOam,
    .anims = sAnimCmdTable_Sight,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};*/

static const struct SpriteTemplate sSearchIconSpriteTemplate =
{
    .tileTag = HIDDEN_SEARCH_TAG,
    .paletteTag = 0xFFFF,   //held item pal
    .oam = &sSearchIconOam,
    .anims =  gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sOwnedIconTemplate = 
{
    .tileTag = OWNED_ICON_TAG,
    .paletteTag = 0xFFFF,   //held item pal
    .oam = &sHeldItemOam,
    .anims =  gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sHiddenMonIconTemplate = 
{
    .tileTag = HIDDEN_MON_ICON_TAG,
    .paletteTag = 0xFFFF,   //held item pal
    .oam = &sHeldItemOam,
    .anims =  gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

// gui sprite sheets
static const struct CompressedSpriteSheet sNoDataIconSpriteSheet = {sNoDataGfx, (32 * 32) / 2, ICON_GFX_TAG};
static const struct CompressedSpriteSheet sCapturedAllPokemonSpriteSheet = {sCapturedAllMonsTiles, (8 * 8) / 2, CAPTURED_ALL_TAG};
// search sprite sheets
static const struct CompressedSpriteSheet sPotentialStarSpriteSheet = {sPotentialStarGfx, (8 * 8) / 2, LIT_STAR_TILE_TAG};
//static const struct CompressedSpriteSheet sSightSpriteSheet = {sEyeGfx, (16 * 8 * 3) / 2, SIGHT_TAG};
static const struct CompressedSpriteSheet sOwnedIconSpriteSheet = {sOwnedIconGfx, (8 * 8) / 2, OWNED_ICON_TAG};
static const struct CompressedSpriteSheet sHiddenMonIconSpriteSheet = {sHiddenMonIconGfx, (8 * 8) / 2, HIDDEN_MON_ICON_TAG};

//// functions
///////////////////////
//// DEXNAV SEARCH ////
///////////////////////
static s16 GetSearchWindowY(void)
{
    return (GetWindowAttribute(sDexNavSearchDataPtr->windowId, WINDOW_TILEMAP_TOP) * 8);
}

#define SPECIES_ICON_X 28
static void DrawDexNavSearchMonIcon(u16 species, u8 *dst, bool8 owned)
{
    u8 spriteId;

    LoadMonIconPalette(species);
    spriteId = CreateMonIcon(species, SpriteCB_MonIcon, SPECIES_ICON_X - 6, GetSearchWindowY() + 8, 0, 0xFFFFFFFF, 0);
    gSprites[spriteId].oam.priority = 0;
    *dst = spriteId;
    
    if (owned)
        sDexNavSearchDataPtr->ownedIconSpriteId = CreateSprite(&sOwnedIconTemplate, SPECIES_ICON_X + 6, GetSearchWindowY() + 4, 0);
}

static void AddSearchWindow(u8 width)
{
    struct WindowTemplate template;
    u16 y = 16;
    
    if (sDexNavSearchDataPtr->tileY > (gSaveBlock1Ptr->pos.y + 7))
        y = 1;  //draw at top if chosen tile is below

    LoadDexNavWindowGfx(sDexNavSearchDataPtr->windowId, 0x1d5, 14 * 16);
    
    SetWindowTemplateFields(&template, 0, 1, y, width, 3, 14, 8);
    
    sDexNavSearchDataPtr->windowId = AddWindow(&template);
    FillWindowPixelBuffer(sDexNavSearchDataPtr->windowId, PIXEL_FILL(1));
    PutWindowTilemap(sDexNavSearchDataPtr->windowId);
    CopyWindowToVram(sDexNavSearchDataPtr->windowId, 3);
    
    DrawStdFrameWithCustomTileAndPalette(sDexNavSearchDataPtr->windowId, TRUE, 0x214, 14);
}

#define WINDOW_COL_0        (SPECIES_ICON_X + 4)
#define WINDOW_COL_1        (WINDOW_COL_0 + (GetFontAttribute(sDexNavSearchDataPtr->windowId, FONTATTR_MAX_LETTER_WIDTH) * (POKEMON_NAME_LENGTH)))
#define WINDOW_MOVE_NAME_X  (WINDOW_COL_1 + (GetFontAttribute(sDexNavSearchDataPtr->windowId, FONTATTR_MAX_LETTER_WIDTH) * 6))
#define SEARCH_ARROW_X      (WINDOW_MOVE_NAME_X + 90)
#define SEARCH_ARROW_Y      0
// Gunk's: the search bar's second row holds nature, ability and search level, the first one a shiny mark
#define SEARCH_BAR_LEVEL_X          WINDOW_COL_1
#define SEARCH_BAR_MOVE_X           124
#define SEARCH_BAR_SHINY_X          182
#define SEARCH_BAR_ABILITY_X        72
#define SEARCH_BAR_SEARCH_LEVEL_X   136
static void AddSearchWindowText(u16 species, u8 proximity, u8 searchLevel, bool8 hidden)
{
    u8 windowId = sDexNavSearchDataPtr->windowId;
    
    //species name - always present
    if (hidden)
    {
        StringCopy(gStringVar4, sText_ThreeQmarks);
        AddTextPrinterParameterized3(sDexNavSearchDataPtr->windowId, 0, WINDOW_COL_0, 0, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar4);
        return;
    }
    else
    {
        StringCopy(gStringVar1, gSpeciesNames[species]);
        AddTextPrinterParameterized3(sDexNavSearchDataPtr->windowId, 0, WINDOW_COL_0, 0, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar1);
    }
    
    //level - always present
    ConvertIntToDecimalStringN(gStringVar1, sDexNavSearchDataPtr->monLevel, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringExpandPlaceholders(gStringVar4, sText_MonLevel);
    AddTextPrinterParameterized3(sDexNavSearchDataPtr->windowId, 0, SEARCH_BAR_LEVEL_X, 0, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar4);

    // Gunk's: like Gunk's Platinum's search bar, always show a shiny mark, the nature, the ability and the search
    // level (the rolled Pokemon is already made, so this is what the battle gives)
    if (IsMonShiny(&sDexNavSearchDataPtr->mon))
        AddTextPrinterParameterized3(windowId, 0, SEARCH_BAR_SHINY_X, 0, sSearchFontColor, TEXT_SKIP_DRAW, sText_DexNav_Shiny);
    AddTextPrinterParameterized3(windowId, 0, WINDOW_COL_0, 12, sSearchFontColor, TEXT_SKIP_DRAW,
                                 gNatureNamePointers[GetNature(&sDexNavSearchDataPtr->mon)]);
    AddTextPrinterParameterized3(windowId, 0, SEARCH_BAR_ABILITY_X, 12, sSearchFontColor, TEXT_SKIP_DRAW,
                                 gAbilityNames[GetAbilityBySpecies(species, sDexNavSearchDataPtr->abilityNum)]);
    ConvertIntToDecimalStringN(gStringVar1, searchLevel, STR_CONV_MODE_LEFT_ALIGN, 4);
    StringExpandPlaceholders(gStringVar4, sText_DexNav_BarSearchLevel);
    AddTextPrinterParameterized3(windowId, 0, SEARCH_BAR_SEARCH_LEVEL_X, 12, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar4);

    if (proximity <= SNEAKING_PROXIMITY)
    {
        PlaySE(SE_POKENAV_ON);
        // move
        if (searchLevel > 1 && sDexNavSearchDataPtr->moves[0])
        {
            StringCopy(gStringVar1, gMoveNames[sDexNavSearchDataPtr->moves[0]]);
            StringExpandPlaceholders(gStringVar4, sText_EggMove);
            AddTextPrinterParameterized3(windowId, 0, SEARCH_BAR_MOVE_X, 0, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar4);
        }
        // the held item shows as its icon (DexNavUpdateSearchWindow)
    }

    //chain level - always present
    ConvertIntToDecimalStringN(gStringVar1, gSaveBlock1Ptr->dexNavChain, STR_CONV_MODE_LEFT_ALIGN, 3);
    if (gSaveBlock1Ptr->dexNavChain > 99)
        StringExpandPlaceholders(gStringVar4, sText_DexNavChainLong);
    else
        StringExpandPlaceholders(gStringVar4, sText_DexNavChain);
    AddTextPrinterParameterized3(windowId, 0, SEARCH_ARROW_X - 16, 12, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar4);    
    
    CopyWindowToVram(sDexNavSearchDataPtr->windowId, 2);
}

static void DrawSearchWindow(u16 species, u8 potential, bool8 hidden)
{
    u8 searchLevel = sDexNavSearchDataPtr->searchLevel;
    
    AddSearchWindow(28);
    AddSearchWindowText(species, sDexNavSearchDataPtr->proximity, searchLevel, hidden);
}

static void RemoveDexNavWindowAndGfx(void)
{
    u32 i;
    
    // try remove sprites
    if (sDexNavSearchDataPtr->iconSpriteId != MAX_SPRITES)
        DestroySprite(&gSprites[sDexNavSearchDataPtr->iconSpriteId]);
    if (sDexNavSearchDataPtr->itemSpriteId != MAX_SPRITES)
        DestroySprite(&gSprites[sDexNavSearchDataPtr->itemSpriteId]);
    if (sDexNavSearchDataPtr->eyeSpriteId != MAX_SPRITES)
        DestroySprite(&gSprites[sDexNavSearchDataPtr->eyeSpriteId]);
    if (sDexNavSearchDataPtr->ownedIconSpriteId != MAX_SPRITES)
        DestroySprite(&gSprites[sDexNavSearchDataPtr->ownedIconSpriteId]);
    if (sDexNavSearchDataPtr->exclamationSpriteId != MAX_SPRITES)
        DestroySprite(&gSprites[sDexNavSearchDataPtr->exclamationSpriteId]);
    
    for (i = 0; i < NELEMS(sDexNavSearchDataPtr->starSpriteIds); i++)
    {
        if (sDexNavSearchDataPtr->starSpriteIds[i] != MAX_SPRITES)
            DestroySprite(&gSprites[sDexNavSearchDataPtr->starSpriteIds[i]]);
    }
    
    FreeSpriteTilesByTag(HELD_ITEM_TAG);
    FreeSpriteTilesByTag(OWNED_ICON_TAG);
    FreeSpriteTilesByTag(HIDDEN_SEARCH_TAG);
    FreeSpriteTilesByTag(HIDDEN_MON_ICON_TAG);
    FreeSpriteTilesByTag(LIT_STAR_TILE_TAG);
    FreeSpritePaletteByTag(HELD_ITEM_TAG);
    SafeFreeMonIconPalette(sDexNavSearchDataPtr->species);
    
    // remove window
    ClearStdWindowAndFrameToTransparent(sDexNavSearchDataPtr->windowId, FALSE);
    CopyWindowToVram(sDexNavSearchDataPtr->windowId, 3);
    RemoveWindow(sDexNavSearchDataPtr->windowId);
}


//////////////////////
////DEXNAV SEARCH/////
//////////////////////
static u8 GetPlayerDistance(s16 x, s16 y)
{
    u16 deltaX = abs(x - (gSaveBlock1Ptr->pos.x + 7));
    u16 deltaY = abs(y - (gSaveBlock1Ptr->pos.y + 7));
    return deltaX + deltaY;
}

static void DexNavProximityUpdate(void)
{
    sDexNavSearchDataPtr->proximity = GetPlayerDistance(sDexNavSearchDataPtr->tileX, sDexNavSearchDataPtr->tileY);
}

//Pick a specific tile based on environment
static bool8 DexNavPickTile(u8 environment, u8 areaX, u8 areaY, bool8 smallScan)
{
    // area of map to cover starting from camera position {-7, -7}
    s16 topX = gSaveBlock1Ptr->pos.x - SCANSTART_X + (smallScan * 5);
    s16 topY = gSaveBlock1Ptr->pos.y - SCANSTART_Y + (smallScan * 5);
    s16 botX = topX + areaX;
    s16 botY = topY + areaY;
    u8 i;
    bool8 nextIter;
    u8 scale = 0;
    u8 weight = 0;
    u8 currMapType = GetCurrentMapType();
    u8 tileBehaviour;
    u8 tileBuffer = 2;
    
    // loop through every tile in area and evaluate
    while (topY < botY)
    {
        while (topX < botX)
        {
            tileBehaviour = MapGridGetMetatileBehaviorAt(topX, topY);
            
            //gSpecialVar_0x8005 = tileBehaviour;
            
            //Check for objects
            nextIter = FALSE;
            if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_BIKE))
                tileBuffer = SNEAKING_PROXIMITY + 3;
            else if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_DASH))
                tileBuffer = SNEAKING_PROXIMITY + 1;
            
            if (GetPlayerDistance(topX, topY) <= tileBuffer)
            {
                // tile too close to player
                topX++;
                continue;
            }
            
            for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
            {
                if (gObjectEvents[i].currentCoords.x == topX && gObjectEvents[i].currentCoords.y == topY)
                {
                    // cannot be on a tile where an object exists
                    nextIter = TRUE;
                    break;
                }
            }
            
            if (nextIter)
            {
                topX++;
                continue;
            }
            
            switch (environment)
            {
            case ENCOUNTER_TYPE_LAND:
                if (MetatileBehavior_IsLandWildEncounter(tileBehaviour))
                {
                    if (currMapType == MAP_TYPE_UNDERGROUND)
                    { // inside (cave)
                        if (IsElevationMismatchAt(gObjectEvents[gPlayerAvatar.spriteId].currentElevation, topX, topY))
                            break; //occurs at same z coord
                        
                        scale = 440 - (smallScan * 200) - (GetPlayerDistance(topX, topY) / 2)  - (2 * (topX + topY));
                        weight = ((Random() % scale) < 1) && !MapGridGetCollisionAt(topX, topY);
                    }
                    else
                    { // outdoors: grass
                        scale = 100 - (GetPlayerDistance(topX, topY) * 2);
                        weight = (Random() % scale <= 5) && !MapGridGetCollisionAt(topX, topY);
                    }
                }
                break;
            case ENCOUNTER_TYPE_WATER:
                if (MetatileBehavior_IsSurfableWaterOrUnderwater(tileBehaviour))
                {
                    u8 scale = 320 - (smallScan * 200) - (GetPlayerDistance(topX, topY) / 2);
                    if (IsElevationMismatchAt(gObjectEvents[gPlayerAvatar.spriteId].currentElevation, topX, topY))
                        break;

                    weight = (Random() % scale <= 1) && !MapGridGetCollisionAt(topX, topY);
                }
                break;
            default:
                break;
            }
            
            if (weight > 0)
            {
                sDexNavSearchDataPtr->tileX = topX;
                sDexNavSearchDataPtr->tileY = topY;
                return TRUE;
            }
            
            topX++;
        }
        
        topY++;
        topX = gSaveBlock1Ptr->pos.x - SCANSTART_X + (smallScan * 5);
    }

    return FALSE;
}


// Gunk's: stop the patch's field effect if its sprite is still the patch. One-shot effects (the shaking grass)
// end and free their sprite on their own, and the slot may already belong to another sprite.
static void GunkStopPatchEffect(void)
{
    struct Sprite *sprite = &gSprites[sDexNavSearchDataPtr->fldEffSpriteId];

    if (sDexNavSearchDataPtr->fldEffSpriteId < MAX_SPRITES && sprite->inUse
     && sprite->callback == sDexNavSearchDataPtr->fldEffCallback)
        FieldEffectStop(sprite, sDexNavSearchDataPtr->fldEffId);
    sDexNavSearchDataPtr->fldEffSpriteId = MAX_SPRITES;
}

// Gunk's: shake the patch again (same tile, same effect)
static void GunkRestartPatchEffect(void)
{
    GunkStopPatchEffect();
    gFieldEffectArguments[0] = sDexNavSearchDataPtr->tileX;
    gFieldEffectArguments[1] = sDexNavSearchDataPtr->tileY;
    gFieldEffectArguments[2] = 0xFF; // subpriority
    gFieldEffectArguments[3] = 2;    // priority
    sDexNavSearchDataPtr->fldEffSpriteId = FieldEffectStart(sDexNavSearchDataPtr->fldEffId);
    if (sDexNavSearchDataPtr->fldEffSpriteId < MAX_SPRITES)
        sDexNavSearchDataPtr->fldEffCallback = gSprites[sDexNavSearchDataPtr->fldEffSpriteId].callback;
}

static bool8 TryStartHiddenMonFieldEffect(u8 environment, u8 xSize, u8 ySize, bool8 smallScan)
{
    u32 i;
    u8 currMapType = GetCurrentMapType();
    u8 fldEffId = 0;
    
    if (DexNavPickTile(environment, xSize, ySize, smallScan))
    {
        u8 metatileBehaviour = MapGridGetMetatileBehaviorAt(sDexNavSearchDataPtr->tileX, sDexNavSearchDataPtr->tileY);

        switch (environment)
        {
        case ENCOUNTER_TYPE_LAND:
            if (currMapType == MAP_TYPE_UNDERGROUND)
            {
                fldEffId = FLDEFF_CAVE_DUST;
            }
            else if (IsMapTypeIndoors(currMapType))
            {
                if (MetatileBehavior_IsTallGrass(metatileBehaviour)) //Grass in cave
                    fldEffId = FLDEFF_SHAKING_GRASS;
                else if (MetatileBehavior_IsLongGrass(metatileBehaviour)) //Really tall grass
                    fldEffId = FLDEFF_SHAKING_LONG_GRASS;
                else if (MetatileBehavior_IsSandOrDeepSand(metatileBehaviour))
                    fldEffId = FLDEFF_SAND_HOLE;
                else
                    fldEffId = FLDEFF_CAVE_DUST;
            }
            else //outdoor, underwater
            {
                if (MetatileBehavior_IsTallGrass(metatileBehaviour)) //Regular grass
                    fldEffId = FLDEFF_SHAKING_GRASS;
                else if (MetatileBehavior_IsLongGrass(metatileBehaviour)) //Really tall grass
                    fldEffId = FLDEFF_SHAKING_LONG_GRASS;
                else if (MetatileBehavior_IsSandOrDeepSand(metatileBehaviour)) //Desert Sand
                    fldEffId = FLDEFF_SAND_HOLE;
                else if (MetatileBehavior_IsMountain(metatileBehaviour)) //Rough Terrain
                    fldEffId = FLDEFF_CAVE_DUST;
                else
                    fldEffId = FLDEFF_BERRY_TREE_GROWTH_SPARKLE; //default
            }
            break;
        case ENCOUNTER_TYPE_WATER:
            fldEffId = FLDEFF_WATER_SURFACING;
            break;
        default:
            return FALSE;
        }
        
        if (fldEffId != 0)
        {
            gFieldEffectArguments[0] = sDexNavSearchDataPtr->tileX;
            gFieldEffectArguments[1] = sDexNavSearchDataPtr->tileY;
            gFieldEffectArguments[2] = 0xFF; // subpriority
            gFieldEffectArguments[3] = 2;   //priority
            sDexNavSearchDataPtr->fldEffSpriteId = FieldEffectStart(fldEffId);
            if (sDexNavSearchDataPtr->fldEffSpriteId == MAX_SPRITES)
                return FALSE;

            sDexNavSearchDataPtr->fldEffId = fldEffId;
            sDexNavSearchDataPtr->fldEffCallback = gSprites[sDexNavSearchDataPtr->fldEffSpriteId].callback;
            return TRUE;
        }
    }

    return FALSE;
}

static void DrawDexNavSearchHeldItem(u8* dst)
{
    *dst = CreateSprite(&sHeldItemTemplate, SPECIES_ICON_X + 6, GetSearchWindowY() + 18, 0);
    if (*dst != MAX_SPRITES)
        gSprites[*dst].invisible = TRUE;
}

static void LoadSearchIconData(void)
{
    // palettes clash with mon icon, so must load manually
    LoadSpriteSheet(&gSpriteSheet_HeldItem);
    LoadPalette(gHeldItemPalette, 0x100 + (16 * sHeldItemOam.paletteNum), 32);
    LoadCompressedSpriteSheetUsingHeap(&sPotentialStarSpriteSheet);
    //LoadCompressedSpriteSheetUsingHeap(&sSightSpriteSheet);   //eye replaced with arrow
    LoadCompressedSpriteSheetUsingHeap(&sOwnedIconSpriteSheet);
    LoadCompressedSpriteSheetUsingHeap(&sHiddenMonIconSpriteSheet);
}

static u8 GetSearchLevel(u16 dexNum)
{
    u8 searchLevel;
#if USE_DEXNAV_SEARCH_LEVELS == TRUE
    searchLevel = gPokemonStoragePtr->dexNavSearchLevels[dexNum];
#else
    searchLevel = 0;
#endif
    return searchLevel;
}

#define tProximity          data[0]
#define tFrameCount         data[1]
#define tSpecies            data[2]
#define tEnvironment        data[3]
#define tRevealed           data[4]
static void Task_SetUpDexNavSearch(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    
    u16 species = sDexNavSearchDataPtr->species;
    u8 environment = sDexNavSearchDataPtr->environment;
    u8 searchLevel = GetSearchLevel(SpeciesToNationalPokedexNum(species));

    // init sprites
    sDexNavSearchDataPtr->iconSpriteId = MAX_SPRITES;
    sDexNavSearchDataPtr->itemSpriteId = MAX_SPRITES;
    sDexNavSearchDataPtr->eyeSpriteId = MAX_SPRITES;
    sDexNavSearchDataPtr->starSpriteIds[0] = MAX_SPRITES;
    sDexNavSearchDataPtr->starSpriteIds[1] = MAX_SPRITES;
    sDexNavSearchDataPtr->starSpriteIds[2] = MAX_SPRITES;
    sDexNavSearchDataPtr->ownedIconSpriteId = MAX_SPRITES;
    sDexNavSearchDataPtr->exclamationSpriteId = MAX_SPRITES;    
    sDexNavSearchDataPtr->searchLevel = searchLevel;
    
    DexNavGenerateMoveset(species, searchLevel, sDexNavSearchDataPtr->monLevel, &sDexNavSearchDataPtr->moves[0]);
    sDexNavSearchDataPtr->heldItem = DexNavGenerateHeldItem(species, searchLevel);
    sDexNavSearchDataPtr->potential = DexNavGeneratePotential(searchLevel);
    GunkDexNavRollMon(species, environment, sDexNavSearchDataPtr->potential);
    sDexNavSearchDataPtr->abilityNum = GetMonData(&sDexNavSearchDataPtr->mon, MON_DATA_ABILITY_NUM);
    DexNavProximityUpdate();
    
    LoadSearchIconData();
    if (sDexNavSearchDataPtr->hiddenSearch)
    {
        DexNavDrawHiddenIcons();
    }
    else
    {
        DexNavDrawIcons();
        DexNavUpdateSearchWindow(sDexNavSearchDataPtr->proximity, searchLevel);
    }
    
    FlagSet(FLAG_SYS_DEXNAV_SEARCH);
    gPlayerAvatar.creeping = TRUE;  //initialize as true in case mon appears beside you
    gTasks[taskId].tProximity = gSprites[gPlayerAvatar.spriteId].x;
    gTasks[taskId].tFrameCount = 0;
    gTasks[taskId].func = Task_DexNavSearch;
    IncrementGameStat(GAME_STAT_DEXNAV_SCANNED);
}

static void Task_InitDexNavSearch(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    u8 searchLevel;
    u16 species = task->tSpecies;
    u8 environment = task->tEnvironment;
    
    sDexNavSearchDataPtr = &sDexNavSearchData;
    memset(sDexNavSearchDataPtr, 0, sizeof(*sDexNavSearchDataPtr));
    
    // assign non-objects to struct
    sDexNavSearchDataPtr->species = species;
    sDexNavSearchDataPtr->environment = environment;  //updated in DexNavTryGenerateMonLevel if hidden mon
    sDexNavSearchDataPtr->isHiddenMon = (environment == ENCOUNTER_TYPE_HIDDEN) ? TRUE : FALSE;
    sDexNavSearchDataPtr->monLevel = DexNavTryGenerateMonLevel(species, environment);
    
    if (GetFlashLevel() > 0)
    {
        sDexNavSearchDataPtr = NULL;
        FreeMonIconPalettes();
        ScriptContext_SetupScript(EventScript_TooDark);
        DestroyTask(taskId);
        return;
    }
    
    if (sDexNavSearchDataPtr->monLevel == MON_LEVEL_NONEXISTENT || !TryStartHiddenMonFieldEffect(sDexNavSearchDataPtr->environment, 12, 12, FALSE))
    {
        // Gunk's: not in this area at all, or no grass or water close enough
        bool8 notHere = (sDexNavSearchDataPtr->monLevel == MON_LEVEL_NONEXISTENT);

        sDexNavSearchDataPtr = NULL;
        FreeMonIconPalettes();
        if (notHere)
            ScriptContext_SetupScript(EventScript_DexNavNotFoundHere);
        else
            ScriptContext_SetupScript(EventScript_NotFoundNearby);
        DestroyTask(taskId);
        return;
    }
    
    sDexNavSearchDataPtr->hiddenSearch = FALSE;
    task->tRevealed = TRUE; //search window revealed
    task->func = Task_SetUpDexNavSearch;
}

static void DexNavDrawPotentialStars(u8 potential, u8* dst)
{
    u8 spriteId;
    u32 i;
    
    for (i = 0; i < NELEMS(sDexNavSearchDataPtr->starSpriteIds); i++)
    {
        spriteId = MAX_SPRITES;
        if (potential > i)
            spriteId = CreateSprite(&sPotentialStarTemplate, SPECIES_ICON_X - 20, GetSearchWindowY() + 4 + (i * 8), 0);
        
        dst[i] = spriteId;
        if (spriteId != MAX_SPRITES)
            gSprites[spriteId].invisible = TRUE;
    }
}

/*static void DexNavDrawSight(u8 sightLevel, u8* dst)
{
    //LoadSpritePalette(&sHeldItemSpritePalette);
    *dst = CreateSprite(&sSightTemplate, 176 + (16 / 2), GetSearchWindowY() + 18, 0);
    if (*dst != MAX_SPRITES)
        DexNavSightUpdate(sightLevel);
};*/

static void DexNavUpdateDirectionArrow(void)
{
    u16 tileX = sDexNavSearchDataPtr->tileX;
    u16 tileY = sDexNavSearchDataPtr->tileY;
    u16 playerX = gSaveBlock1Ptr->pos.x + 7;
    u16 playerY = gSaveBlock1Ptr->pos.y + 7;
    u16 deltaX = abs(tileX - playerX);
    u16 deltaY = abs(tileY - playerY);
    const u8 *str;
    u8 windowId = sDexNavSearchDataPtr->windowId;
    
    FillWindowPixelRect(windowId, PIXEL_FILL(1), SEARCH_ARROW_X, SEARCH_ARROW_Y, 12, 12);
    if (deltaX <= 1 && deltaY <= 1)
    {
        str = gText_EmptyString2;
    }
    else if (deltaX > deltaY)
    {
        if (playerX > tileX)
            str = sText_ArrowLeft;  //player to right
        else
            str = sText_ArrowRight; //player to left
    }
    else //greater Y diff
    {
        if (playerY > tileY)
            str = sText_ArrowUp;    //player below
        else
            str = sText_ArrowDown;  //player above
    }
    
    AddTextPrinterParameterized3(windowId, 1, SEARCH_ARROW_X, SEARCH_ARROW_Y, sSearchFontColor, TEXT_SKIP_DRAW, str);
    CopyWindowToVram(windowId, 2);
}

static void DexNavDrawIcons(void)
{
    u8 searchLevel = sDexNavSearchDataPtr->searchLevel;
    u16 species = sDexNavSearchDataPtr->species;
    u32 total;
    
    // init sprite ids
    /*sDexNavSearchDataPtr->iconSpriteId = 0xFF;
    sDexNavSearchDataPtr->itemSpriteId = 0xFF;
    for (i = 0; i < NELEMS(sDexNavSearchDataPtr->starSpriteIds); i++)
        sDexNavSearchDataPtr->starSpriteIds[i] = 0xFF;*/
    
    DrawSearchWindow(species, sDexNavSearchDataPtr->potential, FALSE);
    DrawDexNavSearchMonIcon(species, &sDexNavSearchDataPtr->iconSpriteId, GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT));
    DrawDexNavSearchHeldItem(&sDexNavSearchDataPtr->itemSpriteId);
    // Gunk's: one star per perfect IV of the Pokemon this search gives (up to 3), like Gunk's Platinum's
    DexNavDrawPotentialStars(min(GunkCountPerfectIvs(&sDexNavSearchDataPtr->peeked, &total), 3), &sDexNavSearchDataPtr->starSpriteIds[0]);
    DexNavUpdateDirectionArrow();
    //DexNavDrawSight(sDexNavSearchDataPtr->proximity, &sDexNavSearchDataPtr->eyeSpriteId);
}

/////////////////////
//// SEARCH TASK ////
/////////////////////
bool8 TryStartDexnavSearch(void)
{
    u8 taskId;
    u16 val = VarGet(VAR_DEXNAV_SPECIES);
    
    if (FlagGet(FLAG_SYS_DEXNAV_SEARCH))
        return FALSE;

    // Gunk's: say why R can't search, instead of nothing or "couldn't be found nearby"
    if ((val & MASK_SPECIES) == SPECIES_NONE)
    {
        if (!FlagGet(FLAG_SYS_DEXNAV_GET))
            return FALSE;
        ScriptContext_SetupScript(EventScript_DexNavNothingChosen);
        return TRUE;
    }
    if ((val >> 14) == ENCOUNTER_TYPE_WATER && !TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
    {
        ScriptContext_SetupScript(EventScript_DexNavSearchWhileSurfing);
        return TRUE;
    }
    if ((val >> 14) == ENCOUNTER_TYPE_LAND && TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
    {
        ScriptContext_SetupScript(EventScript_DexNavSearchInGrass);
        return TRUE;
    }
    
    HideMapNamePopUpWindow();
    ChangeBgY_ScreenOff(0, 0, 0);
    taskId = CreateTask(Task_InitDexNavSearch, 0);
    gTasks[taskId].tSpecies = val & MASK_SPECIES;
    gTasks[taskId].tEnvironment = val >> 14;
    PlaySE(SE_DEX_SEARCH);
    return FALSE;   //we dont actually want to enable the script context
}

void EndDexNavSearch(u8 taskId)
{
    FlagClear(FLAG_SYS_DEXNAV_SEARCH);
    // Gunk's: no task means a full-screen menu reset the tasks and sprites (or the game was loaded mid-search),
    // so there's no window or sprite of the search left to remove
    if (taskId != TASK_NONE)
        DestroyTask(taskId);
    if (sDexNavSearchDataPtr == NULL)
        return;
    if (taskId != TASK_NONE)
    {
        RemoveDexNavWindowAndGfx();
        GunkStopPatchEffect();
    }
    sDexNavSearchDataPtr = NULL;
}

static void EndDexNavSearchSetupScript(const u8 *script, u8 taskId)
{
    // Gunk's: the chain resets on a timeout (below) or a wild encounter of another species, not here
    EndDexNavSearch(taskId);
    ScriptContext_SetupScript(script);
}

static u8 GetMovementProximityBySearchLevel(void)
{
    if (sDexNavSearchDataPtr->searchLevel < 20)
        return 2;
    else if (sDexNavSearchDataPtr->searchLevel < 50)
        return 3;
    else
        return 4;
}

static void DexNavDrawRevealedMon(void)
{
    u16 species = sDexNavSearchDataPtr->species;
    
    // remove owned icon if it exists
    if (sDexNavSearchDataPtr->ownedIconSpriteId != MAX_SPRITES)
    {
        DestroySprite(&gSprites[sDexNavSearchDataPtr->ownedIconSpriteId]);
        sDexNavSearchDataPtr->ownedIconSpriteId = MAX_SPRITES;
    }
    
    // remove exclamation if it exists
    if (sDexNavSearchDataPtr->exclamationSpriteId != MAX_SPRITES)
    {
        DestroySprite(&gSprites[sDexNavSearchDataPtr->exclamationSpriteId]);
        sDexNavSearchDataPtr->exclamationSpriteId = MAX_SPRITES;
    }
    
    
    if (!GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_SEEN))
    {
        u8 index;
        
        //if not seen, hide name and whiteout mon
        DrawSearchWindow(species, sDexNavSearchDataPtr->potential, TRUE);
        DrawDexNavSearchMonIcon(species, &sDexNavSearchDataPtr->iconSpriteId, FALSE);
        // whiteout icon
        index = IndexOfSpritePaletteTag(gSprites[sDexNavSearchDataPtr->iconSpriteId].template->paletteTag);
        CpuCopy16(&gPlttBufferUnfaded[0x100 + index * 16], sDexNavSearchDataPtr->palBuffer, 32);
        TintPalette_CustomTone(sDexNavSearchDataPtr->palBuffer, 16, 510, 510, 510);
        LoadPalette(sDexNavSearchDataPtr->palBuffer, 0x100 + index * 16, 32);
    }
    else
    {
        DrawSearchWindow(species, sDexNavSearchDataPtr->potential, FALSE);
        DrawDexNavSearchMonIcon(species, &sDexNavSearchDataPtr->iconSpriteId, GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT));
    }
    
    DexNavUpdateDirectionArrow();
}

static void Task_RevealHiddenMon(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    DexNavDrawRevealedMon();
    task->func = Task_DexNavSearch;
    task->tFrameCount = 0;  //restart search clock
}

// Gunk's: hide the search window and sprites under the START menu, and bring them back after
static void GunkDexNavShowSearch(bool8 show)
{
    u8 ids[8];
    u32 i;

    if (sDexNavSearchDataPtr->gfxHidden == !show)
        return;

    if (show)
    {
        // the START menu loads its own frame over this one's tiles, and hiding cleared the text: draw both again
        LoadDexNavWindowGfx(sDexNavSearchDataPtr->windowId, 0x1d5, 14 * 16);
        DrawStdFrameWithCustomTileAndPalette(sDexNavSearchDataPtr->windowId, FALSE, 0x214, 14);
        if (sDexNavSearchDataPtr->hiddenSearch && !sDexNavSearchDataPtr->revealed)
            DrawHiddenSearchText();
        else
            DexNavUpdateSearchWindow(sDexNavSearchDataPtr->proximity, sDexNavSearchDataPtr->searchLevel);
        CopyWindowToVram(sDexNavSearchDataPtr->windowId, COPYWIN_FULL);
    }
    else
    {
        ClearStdWindowAndFrameToTransparent(sDexNavSearchDataPtr->windowId, TRUE);
    }

    ids[0] = sDexNavSearchDataPtr->iconSpriteId;
    ids[1] = sDexNavSearchDataPtr->itemSpriteId;
    ids[2] = sDexNavSearchDataPtr->eyeSpriteId;
    ids[3] = sDexNavSearchDataPtr->starSpriteIds[0];
    ids[4] = sDexNavSearchDataPtr->starSpriteIds[1];
    ids[5] = sDexNavSearchDataPtr->starSpriteIds[2];
    ids[6] = sDexNavSearchDataPtr->ownedIconSpriteId;
    ids[7] = sDexNavSearchDataPtr->exclamationSpriteId;
    if (!show)
        sDexNavSearchDataPtr->shownSprites = 0;
    for (i = 0; i < NELEMS(ids); i++)
    {
        if (ids[i] == MAX_SPRITES)
            continue;
        if (show)
        {
            gSprites[ids[i]].invisible = !(sDexNavSearchDataPtr->shownSprites & (1 << i));
        }
        else
        {
            if (!gSprites[ids[i]].invisible)
                sDexNavSearchDataPtr->shownSprites |= 1 << i;
            gSprites[ids[i]].invisible = TRUE;
        }
    }
    sDexNavSearchDataPtr->gfxHidden = !show;
}

// Gunk's: a full-screen menu from the START menu (Bag, party, Pokédex...) resets the tasks and sprites. When the
// field comes back with the search still on, draw it again and carry on from the same frame count
void GunkDexNavTryResumeSearch(void)
{
    u8 taskId;
    u32 i;

    if (!FlagGet(FLAG_SYS_DEXNAV_SEARCH))
        return;
    if (sDexNavSearchDataPtr == NULL)
    { // loaded from a save made mid-search
        FlagClear(FLAG_SYS_DEXNAV_SEARCH);
        return;
    }
    if (FuncIsActiveTask(Task_DexNavSearch) || FuncIsActiveTask(Task_RevealHiddenMon))
        return;

    sDexNavSearchDataPtr->iconSpriteId = MAX_SPRITES;
    sDexNavSearchDataPtr->itemSpriteId = MAX_SPRITES;
    sDexNavSearchDataPtr->eyeSpriteId = MAX_SPRITES;
    for (i = 0; i < NELEMS(sDexNavSearchDataPtr->starSpriteIds); i++)
        sDexNavSearchDataPtr->starSpriteIds[i] = MAX_SPRITES;
    sDexNavSearchDataPtr->ownedIconSpriteId = MAX_SPRITES;
    sDexNavSearchDataPtr->exclamationSpriteId = MAX_SPRITES;
    sDexNavSearchDataPtr->fldEffSpriteId = MAX_SPRITES;
    sDexNavSearchDataPtr->gfxHidden = FALSE;

    LoadSearchIconData();
    if (sDexNavSearchDataPtr->hiddenSearch && !sDexNavSearchDataPtr->revealed)
    {
        DexNavDrawHiddenIcons();
    }
    else if (sDexNavSearchDataPtr->hiddenSearch)
    {
        DexNavDrawRevealedMon();
    }
    else
    {
        DexNavDrawIcons();
        DexNavUpdateSearchWindow(sDexNavSearchDataPtr->proximity, sDexNavSearchDataPtr->searchLevel);
    }
    GunkRestartPatchEffect();

    gPlayerAvatar.creeping = TRUE;
    taskId = CreateTask(Task_DexNavSearch, 0);
    gTasks[taskId].tProximity = sDexNavSearchDataPtr->proximity;
    gTasks[taskId].tFrameCount = sDexNavSearchDataPtr->frameCount;
    gTasks[taskId].tRevealed = sDexNavSearchDataPtr->revealed;
}

static void Task_DexNavSearch(u8 taskId)
{
    u16 species;
    s16 x, y;
    struct Task *task = &gTasks[taskId];

    // Gunk's: the START menu and what it opens (Bag, party...) pause the search, like Gunk's Platinum's menus;
    // a full-screen menu stops this task, and GunkDexNavTryResumeSearch starts it again from here
    sDexNavSearchDataPtr->frameCount = task->tFrameCount;
    sDexNavSearchDataPtr->revealed = task->tRevealed;
    if (IsStartMenuOpen() || gMain.callback2 != CB2_Overworld)
    {
        GunkDexNavShowSearch(FALSE);
        return;
    }
    GunkDexNavShowSearch(TRUE);
    
    if (sDexNavSearchDataPtr->proximity > MAX_PROXIMITY)
    { // out of range
        if (sDexNavSearchDataPtr->hiddenSearch && !task->tRevealed)
            EndDexNavSearch(taskId);
        else
            EndDexNavSearchSetupScript(EventScript_LostSignal, taskId);
        return;
    }
    
    if (sDexNavSearchDataPtr->proximity <= CREEPING_PROXIMITY && !gPlayerAvatar.creeping && task->tFrameCount > 60)
    { //should be creeping but player walks normally
        if (sDexNavSearchDataPtr->hiddenSearch && !task->tRevealed)
            EndDexNavSearch(taskId);
        else
            EndDexNavSearchSetupScript(EventScript_MovedTooFast, taskId);
        return;
    }
    
    if (sDexNavSearchDataPtr->proximity <= SNEAKING_PROXIMITY && TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_DASH | PLAYER_AVATAR_FLAG_BIKE)) 
    { // running/biking too close
        //always do event script, even if player hasn't revealed a hidden mon. It's assumed they would be creeping towards it
        EndDexNavSearchSetupScript(EventScript_MovedTooFast, taskId);
        return;
    }
    
    if (ArePlayerFieldControlsLocked() == TRUE)
    { // check if script just executed
        //gSaveBlock1Ptr->dexNavChain = 0;  //issue with reusable repels
        EndDexNavSearch(taskId);
        return;
    }
    
    if (gTasks[taskId].tFrameCount > DEXNAV_TIMEOUT * 60)
    { // player took too long
        if (sDexNavSearchDataPtr->hiddenSearch && !task->tRevealed)
            EndDexNavSearch(taskId);
        else
        {
            gSaveBlock1Ptr->dexNavChain = 0; // Gunk's: timeout resets the chain
            EndDexNavSearchSetupScript(EventScript_PokemonGotAway, taskId);
        }
        return;
    }
    
    if (sDexNavSearchDataPtr->proximity < 1)
    {
        CreateDexNavWildMon(sDexNavSearchDataPtr->species, sDexNavSearchDataPtr->potential, sDexNavSearchDataPtr->monLevel, 
          sDexNavSearchDataPtr->abilityNum, sDexNavSearchDataPtr->heldItem, sDexNavSearchDataPtr->moves);
        
        FlagClear(FLAG_SYS_DEXNAV_SEARCH);
        gDexnavBattle = TRUE;        
        ScriptContext_SetupScript(EventScript_StartDexNavBattle);
        sDexNavSearchDataPtr = NULL;
        DestroyTask(taskId);
        return;
    }
    
    if (sDexNavSearchDataPtr->hiddenSearch && !task->tRevealed &&
        (JOY_NEW(R_BUTTON) || (sDexNavSearchDataPtr->proximity < CREEPING_PROXIMITY)))
    {
        PlaySE(SE_DEX_SEARCH);
        ClearStdWindowAndFrameToTransparent(sDexNavSearchDataPtr->windowId, FALSE);
        CopyWindowToVram(sDexNavSearchDataPtr->windowId, 3);
        RemoveWindow(sDexNavSearchDataPtr->windowId);
        DestroySprite(&gSprites[sDexNavSearchDataPtr->iconSpriteId]);
        task->tRevealed = TRUE; //regular dexnav search
        //sDexNavSearchDataPtr->hiddenSearch = FALSE; //now its a regular dexnav search
        task->func = Task_RevealHiddenMon;
        return;
    }

    //Caves and water the pokemon moves around
    if ((sDexNavSearchDataPtr->environment == ENCOUNTER_TYPE_WATER || GetCurrentMapType() == MAP_TYPE_UNDERGROUND)
        && sDexNavSearchDataPtr->proximity < GetMovementProximityBySearchLevel() && sDexNavSearchDataPtr->movementCount < 2
        && task->tRevealed)
    {
        bool8 ret;
        
        GunkStopPatchEffect();
        while (1) {
            if (TryStartHiddenMonFieldEffect(sDexNavSearchDataPtr->environment, 10, 10, TRUE))
                break;
        }
        
        sDexNavSearchDataPtr->movementCount++;
    }

    DexNavProximityUpdate();
    if (task->tProximity != sDexNavSearchDataPtr->proximity)
    {
        //player has moved
        if (task->tRevealed)    //update search window info only if hidden mon has been revealed (always true for search mode)
            DexNavUpdateSearchWindow(sDexNavSearchDataPtr->proximity, sDexNavSearchDataPtr->searchLevel);
        
        task->tProximity = sDexNavSearchDataPtr->proximity;
    }

    if (task->tFrameCount % GUNK_DEXNAV_SHAKE_FRAMES == GUNK_DEXNAV_SHAKE_FRAMES - 1)
        GunkRestartPatchEffect();
    task->tFrameCount++;
}

/*static void DexNavSightUpdate(u8 index)
{
    u8 spriteId = sDexNavSearchDataPtr->eyeSpriteId;
    
    if (spriteId != MAX_SPRITES)
        StartSpriteAnim(&gSprites[spriteId], index);
}*/

static void DexNavUpdateSearchWindow(u8 proximity, u8 searchLevel)
{
    bool8 hideName = FALSE;

    if (sDexNavSearchDataPtr->hiddenSearch && !GetSetPokedexFlag(SpeciesToNationalPokedexNum(sDexNavSearchDataPtr->species), FLAG_GET_SEEN))
        hideName = TRUE;    //if a detector mode hidden search and player hasn't seen the mon, hide info
    
    FillWindowPixelBuffer(sDexNavSearchDataPtr->windowId, PIXEL_FILL(1));   //clear window
    AddSearchWindowText(sDexNavSearchDataPtr->species, proximity, searchLevel, hideName);
    
    DexNavUpdateDirectionArrow();
    
    //init hidden sprites
    if (sDexNavSearchDataPtr->itemSpriteId != MAX_SPRITES)
        gSprites[sDexNavSearchDataPtr->itemSpriteId].invisible = TRUE;
    if (sDexNavSearchDataPtr->starSpriteIds[0] != MAX_SPRITES)
        gSprites[sDexNavSearchDataPtr->starSpriteIds[0]].invisible = TRUE;
    if (sDexNavSearchDataPtr->starSpriteIds[1] != MAX_SPRITES)
        gSprites[sDexNavSearchDataPtr->starSpriteIds[1]].invisible = TRUE;
    if (sDexNavSearchDataPtr->starSpriteIds[2] != MAX_SPRITES)
        gSprites[sDexNavSearchDataPtr->starSpriteIds[2]].invisible = TRUE;
    
    if (proximity <= SNEAKING_PROXIMITY)
    {
        if (searchLevel > 2 && sDexNavSearchDataPtr->heldItem)
        {
            // toggle item view
            if (sDexNavSearchDataPtr->itemSpriteId != MAX_SPRITES)
                gSprites[sDexNavSearchDataPtr->itemSpriteId].invisible = FALSE;
        }
        
    }

    // Gunk's: the perfect-IV stars always show
    if (sDexNavSearchDataPtr->starSpriteIds[0] != MAX_SPRITES)
        gSprites[sDexNavSearchDataPtr->starSpriteIds[0]].invisible = FALSE;
    if (sDexNavSearchDataPtr->starSpriteIds[1] != MAX_SPRITES)
        gSprites[sDexNavSearchDataPtr->starSpriteIds[1]].invisible = FALSE;
    if (sDexNavSearchDataPtr->starSpriteIds[2] != MAX_SPRITES)
        gSprites[sDexNavSearchDataPtr->starSpriteIds[2]].invisible = FALSE;
}

//////////////////////////////
//// DEXNAV MON GENERATOR ////
//////////////////////////////
// Gunk's: every DexNav Pokemon is an ordinary wild encounter of that species on this map: encounter slot,
// level, nature, personality and IVs come from the same rolls a real encounter uses (GunkRollWildMonForDexNav),
// so it is exactly as legal as a Pokemon caught the normal way. Search level, chain and stars never edit it;
// they buy extra encounter rolls, and the best one is kept:
// - shiny: as many rolls as the DexNav's shiny odds are worth in normal 1/8192 rolls; the first shiny wins
// - stars: 1 = 8, 2 = 32, 3 = 128 rolls, keeping the one with the most perfect IVs, then the best total
#define GUNK_DEXNAV_MAX_SLOT_ROLLS 6000 // encounter rolls per search, most of them other species

static u32 GunkDexNavShinyRolls(void);

static u32 GunkCountPerfectIvs(const struct GunkWildRoll *roll, u32 *total)
{
    u32 i, iv, count = 0;

    *total = 0;
    for (i = 0; i < NUM_STATS; i++)
    {
        iv = (roll->ivs[i / 3] >> (5 * (i % 3))) & MAX_IV_MASK;
        *total += iv;
        if (iv == MAX_PER_STAT_IVS)
            count++;
    }
    return count;
}

static bool32 GunkIsRollShiny(const struct GunkWildRoll *roll)
{
    u32 otId = T1_READ_32(gSaveBlock2Ptr->playerTrainerId);

    return GET_SHINY_VALUE(otId, roll->personality) < SHINY_ODDS;
}

// Each roll is first only peeked at (its random numbers, no Pokemon made: making one per roll froze the game for
// seconds at high search levels). The random seed before the best roll is kept, and that roll is made for real at
// the end from the same seed, so it is the same encounter.
static void GunkDexNavRollMon(u16 species, u8 environment, u8 potential)
{
    static const u16 sIvRolls[] = {1, 8, 32, 128};
    struct GunkWildRoll roll;
    bool8 water = (environment == ENCOUNTER_TYPE_WATER);
    u32 ivRolls = sIvRolls[min(potential, 3)];
    u32 shinyRolls = GunkDexNavShinyRolls();
    u32 rolls = max(ivRolls, shinyRolls);
    u32 tries, i = 0, perfect, total, bestPerfect = 0, bestTotal = 0;
    u32 seed, bestSeed = 0, endSeed;
    bool32 haveBest = FALSE;

    // a VBlank in the middle of a peek would advance the random numbers there but not in the replay
    gGunkHoldVBlankRandom = TRUE;
    for (tries = 0; tries < GUNK_DEXNAV_MAX_SLOT_ROLLS && i < rolls; tries++)
    {
        seed = gRngValue;
        if (!GunkRollWildMonForDexNav(species, water, &roll))
            continue;
        if (i < shinyRolls && GunkIsRollShiny(&roll))
        {
            sDexNavSearchDataPtr->peeked = roll;
            bestSeed = seed;
            haveBest = TRUE;
            break;
        }
        if (i < ivRolls)
        {
            perfect = GunkCountPerfectIvs(&roll, &total);
            if (!haveBest || perfect > bestPerfect || (perfect == bestPerfect && total > bestTotal))
            {
                sDexNavSearchDataPtr->peeked = roll;
                bestSeed = seed;
                bestPerfect = perfect;
                bestTotal = total;
                haveBest = TRUE;
            }
        }
        i++;
    }
    if (!haveBest) // species too rare here to roll in time: keep rolling until one plain encounter of it
    {
        do
        {
            bestSeed = gRngValue;
        }
        while (!GunkRollWildMonForDexNav(species, water, &roll));
        sDexNavSearchDataPtr->peeked = roll;
    }
    endSeed = gRngValue;
    gRngValue = bestSeed;
    GunkRollWildMonForDexNav(species, water, NULL); // the kept roll, made for real
    gRngValue = endSeed;
    gGunkHoldVBlankRandom = FALSE;
    sDexNavSearchDataPtr->mon = gEnemyParty[0];
    ZeroEnemyPartyMons();
    sDexNavSearchDataPtr->monLevel = GetMonData(&sDexNavSearchDataPtr->mon, MON_DATA_LEVEL);
    for (i = 0; i < MAX_MON_MOVES; i++)
        sDexNavSearchDataPtr->moves[i] = GetMonData(&sDexNavSearchDataPtr->mon, MON_DATA_MOVE1 + i);
}

static void CreateDexNavWildMon(u16 species, u8 potential, u8 level, u8 abilityNum, u16 item, u16* moves)
{
    struct Pokemon* mon = &gEnemyParty[0];

    ZeroEnemyPartyMons();
    *mon = sDexNavSearchDataPtr->mon;

    // Set Held Item
    if (item)
        SetMonData(mon, MON_DATA_HELD_ITEM, &item);

    // its moves are the ones its encounter roll gave it (egg moves are off)
    CalculateMonStats(mon);
}

// gets a random level of the species based on map data.
//if it was a hidden encounter, updates the environment it is to be found from the wildheader encounterRate
static u8 DexNavTryGenerateMonLevel(u16 species, u8 environment)
{
    // Gunk's: no chain or random level bonus; the level comes from the encounter roll (GunkDexNavRollMon)
    return GetEncounterLevelFromMapData(species, environment);
}

static void DexNavGenerateMoveset(u16 species, u8 searchLevel, u8 encounterLevel, u16* moveDst)
{
    bool8 genMove = FALSE;
    u16 randVal = Random() % 100;
    u16 i;
    u16 eggMoveBuffer[EGG_MOVES_ARRAY_COUNT];

    // see if first move slot should be an egg move
    if (searchLevel < 5)
    {
        #if (SEARCHLEVEL0_MOVECHANCE != 0)
        if (randVal < SEARCHLEVEL0_MOVECHANCE)
            genMove = TRUE;
        #endif
    }
    else if (searchLevel < 10)
    {
        #if (SEARCHLEVEL5_MOVECHANCE != 0)
        if (randVal < SEARCHLEVEL5_MOVECHANCE)
            genMove = TRUE;
        #endif
    }
    else if (searchLevel < 25)
    {
        #if (SEARCHLEVEL10_MOVECHANCE != 0)
        if (randVal < SEARCHLEVEL10_MOVECHANCE)
            genMove = TRUE;
        #endif
    }
    else if (searchLevel < 50)
    {
        #if (SEARCHLEVEL25_MOVECHANCE != 0)
        if (randVal < SEARCHLEVEL25_MOVECHANCE)
            genMove = TRUE;
        #endif
    }
    else if (searchLevel < 100)
    {
        #if (SEARCHLEVEL50_MOVECHANCE != 0)
        if (randVal < SEARCHLEVEL50_MOVECHANCE)
            genMove = TRUE;
        #endif
    }
    else
    {
        #if (SEARCHLEVEL100_MOVECHANCE != 0)
        if (randVal < SEARCHLEVEL100_MOVECHANCE)
            genMove = TRUE;
        #endif
    }

    // Generate a wild mon just to get the initial moveset (later overwritten by CreateDexNavWildMon)
    CreateWildMon(species, encounterLevel);

    // Store generated mon moves into Dex Nav Struct
    for (i = 0; i < MAX_MON_MOVES; i++)
        moveDst[i] = GetMonData(&gEnemyParty[0], MON_DATA_MOVE1 + i, NULL);

    // set first move slot to a random egg move if search level is good enough    
    if (genMove)
    {
        u8 numEggMoves = GetEggMoves(&gEnemyParty[0], eggMoveBuffer);
        if (numEggMoves != 0)
            moveDst[0] = eggMoveBuffer[Random() % numEggMoves];
    }
}

static u16 DexNavGenerateHeldItem(u16 species, u8 searchLevel)
{
    u16 randVal = Random() % 100;
    u8 searchLevelInfluence = searchLevel >> 1;
    u16 item1 = gSpeciesInfo[species].itemCommon;
    u16 item2 = gSpeciesInfo[species].itemRare;
    // Gunk's: a Compound Eyes lead makes items more likely, as it does for other wild Pokemon (55% -> 80% to hold
    // one, 5% -> 20% for the rare one)
    bool8 compoundEyes = !GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG)
                      && GetMonAbility(&gPlayerParty[0]) == ABILITY_COMPOUND_EYES;
    u8 holdBonus = compoundEyes ? 25 : 0;
    u8 rareBonus = compoundEyes ? 15 : 0;

    // Gunk's: Altering Cave has its own wild item list; the battle rolls it the vanilla way (SetWildMonHeldItem)
    if (gMapHeader.mapLayoutId == LAYOUT_ALTERING_CAVE)
        return ITEM_NONE;
    
    // if both are the same, 100% to hold
    if (item1 == item2)
        return item1;

    // if no items can be held, then yeah...no items
    if (item2 == ITEM_NONE && item1 == ITEM_NONE)
        return ITEM_NONE;

    // if only one entry, 50% chance
    if (item2 == ITEM_NONE && item1 != ITEM_NONE)
        return (randVal < 50 + holdBonus) ? item1 : ITEM_NONE;

    // if both are distinct item1 = 50% + srclvl/2; item2 = 5% + srchlvl/2
    if (randVal < (50 + searchLevelInfluence + 5 + searchLevel + holdBonus))
        return (randVal > 5 + searchLevelInfluence + rareBonus) ? item1 : item2;
    else
        return ITEM_NONE;

    return ITEM_NONE;
}

static u8 DexNavGeneratePotential(u8 searchLevel)
{
    u8 genChance = 0;
    int randVal = Random() % 100;
    
    if (searchLevel < 5)
    {
        genChance = SEARCHLEVEL0_ONESTAR + SEARCHLEVEL0_TWOSTAR + SEARCHLEVEL0_THREESTAR;
        if (randVal < genChance)
        {
            // figure out which star it is
            if (randVal < SEARCHLEVEL0_ONESTAR)
                return 1;
            else if (randVal < (SEARCHLEVEL0_ONESTAR + SEARCHLEVEL0_TWOSTAR))
                return 2;
            else
                return 3;
        }
    }
    else if (searchLevel < 10)
    {
        genChance = SEARCHLEVEL5_ONESTAR + SEARCHLEVEL5_TWOSTAR + SEARCHLEVEL5_THREESTAR;
        if (randVal < genChance)
        {
            // figure out which star it is
            if (randVal < SEARCHLEVEL5_ONESTAR)
                return 1;
            else if (randVal < (SEARCHLEVEL5_ONESTAR + SEARCHLEVEL5_TWOSTAR))
                return 2;
            else
                return 3;
        }
    }
    else if (searchLevel < 25)
    {
        genChance = SEARCHLEVEL10_ONESTAR + SEARCHLEVEL10_TWOSTAR + SEARCHLEVEL10_THREESTAR;
        if (randVal < genChance)
        {
            // figure out which star it is
            if (randVal < SEARCHLEVEL10_ONESTAR)
                return 1;
            else if (randVal < (SEARCHLEVEL10_ONESTAR + SEARCHLEVEL10_TWOSTAR))
                return 2;
            else
                return 3;
        }
    }
    else if (searchLevel < 50)
    {
        genChance = SEARCHLEVEL25_ONESTAR + SEARCHLEVEL25_TWOSTAR + SEARCHLEVEL25_THREESTAR;
        if (randVal < genChance)
        {
            // figure out which star it is
            if (randVal < SEARCHLEVEL25_ONESTAR)
                return 1;
            else if (randVal < (SEARCHLEVEL25_ONESTAR + SEARCHLEVEL25_TWOSTAR))
                return 2;
            else
                return 3;
        }
    }
    else if (searchLevel < 100)
    {
        genChance = SEARCHLEVEL50_ONESTAR + SEARCHLEVEL50_TWOSTAR + SEARCHLEVEL50_THREESTAR;
        if (randVal < genChance)
        {
            // figure out which star it is
            if (randVal < SEARCHLEVEL50_ONESTAR)
                return 1;
            else if (randVal < (SEARCHLEVEL50_ONESTAR + SEARCHLEVEL50_TWOSTAR))
                return 2;
            else
                return 3;
        }
    }
    else
    {
        genChance = SEARCHLEVEL100_ONESTAR + SEARCHLEVEL100_TWOSTAR + SEARCHLEVEL100_THREESTAR;
        if (randVal < genChance)
        {
            // figure out which star it is
            if (randVal < SEARCHLEVEL100_ONESTAR)
                return 1;
            else if (randVal < (SEARCHLEVEL100_ONESTAR + SEARCHLEVEL100_TWOSTAR))
                return 2;
            else
                return 3;
        }
    }
    
    return 0;   // No potential
}

static u8 GetEncounterLevelFromMapData(u16 species, u8 environment)
{
    u16 headerId = GetCurrentMapWildMonHeaderId();
    const struct WildPokemonInfo *landMonsInfo = gWildMonHeaders[headerId].landMonsInfo;
    const struct WildPokemonInfo *waterMonsInfo = gWildMonHeaders[headerId].waterMonsInfo;
    const struct WildPokemonInfo *hiddenMonsInfo = gWildMonHeaders[headerId].hiddenMonsInfo;
    u8 min = 100;
    u8 max = 0;
    u8 i;
    
    switch (environment)
    {
    case ENCOUNTER_TYPE_LAND:    // grass
        if (landMonsInfo == NULL)
            return MON_LEVEL_NONEXISTENT; //Hidden pokemon should only appear on walkable tiles or surf tiles

        for (i = 0; i < LAND_WILD_COUNT; i++)
        {
            if (landMonsInfo->wildPokemon[i].species == species)
            {
                min = (min < landMonsInfo->wildPokemon[i].minLevel) ? min : landMonsInfo->wildPokemon[i].minLevel;
                max = (max > landMonsInfo->wildPokemon[i].maxLevel) ? max : landMonsInfo->wildPokemon[i].maxLevel;
            }
        }
        break;
    case ENCOUNTER_TYPE_WATER:    //water
        if (waterMonsInfo == NULL)
            return MON_LEVEL_NONEXISTENT; //Hidden pokemon should only appear on walkable tiles or surf tiles

        for (i = 0; i < WATER_WILD_COUNT; i++)
        {
            if (waterMonsInfo->wildPokemon[i].species == species)
            {
                min = (min < waterMonsInfo->wildPokemon[i].minLevel) ? min : waterMonsInfo->wildPokemon[i].minLevel;
                max = (max > waterMonsInfo->wildPokemon[i].maxLevel) ? max : waterMonsInfo->wildPokemon[i].maxLevel;
            }
        }
        break;
    case ENCOUNTER_TYPE_HIDDEN:
        if (hiddenMonsInfo == NULL)
            return MON_LEVEL_NONEXISTENT;
        
        for (i = 0; i < HIDDEN_WILD_COUNT; i++)
        {
            if (hiddenMonsInfo->wildPokemon[i].species == species)
            {
                min = (min < hiddenMonsInfo->wildPokemon[i].minLevel) ? min : hiddenMonsInfo->wildPokemon[i].minLevel;
                max = (max > hiddenMonsInfo->wildPokemon[i].maxLevel) ? max : hiddenMonsInfo->wildPokemon[i].maxLevel;
            }
        }
        
        // use encounter rate to signify is hidden pokemon are on land or in water
        if (hiddenMonsInfo->encounterRate == 1)
            sDexNavSearchDataPtr->environment = ENCOUNTER_TYPE_WATER;
        else
            sDexNavSearchDataPtr->environment = ENCOUNTER_TYPE_LAND;
        break;
    default:
        return MON_LEVEL_NONEXISTENT;
    }

    if (max == 0)
        return MON_LEVEL_NONEXISTENT;

    return RandRange(min, max);
}


///////////
/// GUI ///
///////////
static const struct BgTemplate sDexNavMenuBgTemplates[2] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .priority = 0
    }, 
    {
        .bg = 1,
        .charBaseIndex = 3,
        .mapBaseIndex = 30,
        .priority = 1
    }
};

static void DexNav_VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void DexNav_MainCB(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static bool8 DexNav_InitBgs(void)
{
    ResetVramOamAndBgCntRegs();
    ResetAllBgsCoordinates();
    sBg1TilemapBuffer = Alloc(0x800);
    if (sBg1TilemapBuffer == NULL)
        return FALSE;
    
    memset(sBg1TilemapBuffer, 0, 0x800);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sDexNavMenuBgTemplates, NELEMS(sDexNavMenuBgTemplates));
    SetBgTilemapBuffer(1, sBg1TilemapBuffer);
    ScheduleBgCopyTilemapToVram(1);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    SetGpuReg(REG_OFFSET_BLDCNT , 0);
    ShowBg(0);
    ShowBg(1);
    return TRUE;
}

// Gunk's: rearrange the screen's tilemap (gui_tilemap.bin) to Gunk's Platinum's layout. The R tab leaves the title
// bar, the SEARCH button and the five labelled bars become one info panel with a blank red bar on top, and the
// HIDDEN box moves left to make room for the button hints.
#define GUI_TILE(r, c) (((u16 *)sBg1TilemapBuffer)[(r) * 32 + (c)])
static void GunkDexNavRelayout(void)
{
    u16 hidden[5][11];
    u32 r, c;

    // title bar: black where the R tab was, and the WATER box's top edge gets its left corner back
    GUI_TILE(0, 1) = GUI_TILE(0, 2) = GUI_TILE(0, 3) = 0;
    GUI_TILE(1, 1) = 0x00B | 0x400; // the top right corner, flipped
    for (c = 2; c <= 4; c++)
        GUI_TILE(1, c) = 0x00A;

    // info panel: cols 20-29, a red bar on rows 2-3 and white down to the bottom edge on row 19
    for (c = 20; c <= 28; c++)
        GUI_TILE(1, c) = 0x005;
    for (c = 20; c <= 29; c++)
    {
        GUI_TILE(2, c) = (c == 29) ? 0x034 : 0x033;
        GUI_TILE(3, c) = (c == 29) ? 0x03B : 0x03A;
        for (r = 4; r <= 18; r++)
            GUI_TILE(r, c) = (c == 20 || c == 29) ? 0x043 : 0x02C;
    }

    // HIDDEN box: cols 4-14 -> 1-11, with its join on the LAND box's bottom edge
    for (r = 0; r < 5; r++)
        for (c = 0; c < 11; c++)
            hidden[r][c] = GUI_TILE(15 + r, 4 + c);
    for (c = 1; c <= 18; c++)
        GUI_TILE(14, c) = (c == 1) ? 0x077 : (c == 11) ? 0x079 : (c < 11) ? 0x078 : 0x076;
    for (r = 0; r < 5; r++)
    {
        for (c = 0; c < 20; c++)
            GUI_TILE(15 + r, c) = (r == 4) ? 0x0A1 : 0x00F;
        for (c = 0; c < 11; c++)
            GUI_TILE(15 + r, 1 + c) = hidden[r][c];
    }
}
#undef GUI_TILE

static bool8 DexNav_LoadGraphics(void)
{
    switch (sDexNavUiDataPtr->state)
    {
    case 0:
        ResetTempTileDataBuffers();
        DecompressAndCopyTileDataToVram(1, sDexNavGuiTiles, 0, 0, 0);
        sDexNavUiDataPtr->state++;
        break;
    case 1:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            LZDecompressWram(sDexNavGuiTilemap, sBg1TilemapBuffer);
            GunkDexNavRelayout();
            sDexNavUiDataPtr->state++;
        }
        break;
    case 2:
        LoadPalette(sDexNavGuiPal, 0, 32);
        sDexNavUiDataPtr->state++;
        break;
    default:
        sDexNavUiDataPtr->state = 0;
        return TRUE;
    }
    
    return FALSE;
}

static void UpdateCursorPosition(void)
{
    u16 x, y;
    
    switch (sDexNavUiDataPtr->cursorRow)
    {
    case ROW_WATER:
        x = ROW_WATER_ICON_X + (24 * sDexNavUiDataPtr->cursorCol);
        y = ROW_WATER_ICON_Y;
        sDexNavUiDataPtr->environment = ENCOUNTER_TYPE_WATER;
        break;
    case ROW_LAND_TOP: //land 1
        x = ROW_LAND_ICON_X + (24 * sDexNavUiDataPtr->cursorCol);
        y = ROW_LAND_TOP_ICON_Y;
        sDexNavUiDataPtr->environment = ENCOUNTER_TYPE_LAND;
        break;
    case ROW_LAND_BOT: //land 2
        x = ROW_LAND_ICON_X + (24 * sDexNavUiDataPtr->cursorCol);
        y = ROW_LAND_BOT_ICON_Y;
        sDexNavUiDataPtr->environment = ENCOUNTER_TYPE_LAND;
        break;
    case ROW_HIDDEN:
        x = ROW_HIDDEN_ICON_X + (24 * sDexNavUiDataPtr->cursorCol);
        y = ROW_HIDDEN_ICON_Y;
        sDexNavUiDataPtr->environment = ENCOUNTER_TYPE_HIDDEN;
        break;
    default:
        return;
    }
    
    gSprites[sDexNavUiDataPtr->cursorSpriteId].x = x;
    gSprites[sDexNavUiDataPtr->cursorSpriteId].y = y;
    
    PrintCurrentSpeciesInfo();
}

static void CreateSelectionCursor(void)
{
    u8 spriteId;
    struct CompressedSpriteSheet spriteSheet;
    
    spriteSheet.data = sSelectionCursorGfx;
    spriteSheet.size = 0x200;
    spriteSheet.tag = SELECTION_CURSOR_TAG;
    LoadCompressedSpriteSheet(&spriteSheet);
    
    LoadPalette(sSelectionCursorPal, (16 * sSelectionCursorOam.paletteNum) + 0x100, 32);
    
    spriteId = CreateSprite(&sSelectionCursorSpriteTemplate, 12, 32, 0);  
    //gSprites[spriteId].data[1] = -1;
    
    sDexNavUiDataPtr->cursorSpriteId = spriteId;
    UpdateCursorPosition();
}

static void CreateNoDataIcon(s16 x, s16 y)
{
    CreateSprite(&sNoDataIconTemplate, x, y, 0);
}

static bool8 CapturedAllLandMons(u16 headerId)
{
    u16 i, species;
    int count = 0;
    const struct WildPokemonInfo* landMonsInfo = gWildMonHeaders[headerId].landMonsInfo;
        
    if (landMonsInfo != NULL)
    {        
        for (i = 0; i < LAND_WILD_COUNT; ++i)
        {
            species = landMonsInfo->wildPokemon[i].species;
            if (species != SPECIES_NONE)
            {
                if (!GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
                    break;
                
                count++;
            }
        }

        if (i >= LAND_WILD_COUNT && count > 0) //All land mons caught
            return TRUE;
    }
    else
    {
        return TRUE;    //technically, no mon data means you caught them all
    }

    return FALSE;
}

//Checks if all Pokemon that can be encountered while surfing have been capture
static bool8 CapturedAllWaterMons(u16 headerId)
{
    u32 i;
    u16 species;
    u8 count = 0;
    const struct WildPokemonInfo* waterMonsInfo = gWildMonHeaders[headerId].waterMonsInfo;

    if (waterMonsInfo != NULL)
    {
        for (i = 0; i < WATER_WILD_COUNT; ++i)
        {
            species = waterMonsInfo->wildPokemon[i].species;
            if (species != SPECIES_NONE)
            {
                count++;
                if (!GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
                    break;
            }
        }

        if (i >= WATER_WILD_COUNT && count > 0)
            return TRUE;
    }
    else
    {
        return TRUE;    //technically, no mon data means you caught them all
    }

    return FALSE;
}

static bool8 CapturedAllHiddenMons(u16 headerId)
{
    u32 i;
    u16 species;
    u8 count = 0;
    const struct WildPokemonInfo* hiddenMonsInfo = gWildMonHeaders[headerId].hiddenMonsInfo;
    
    if (hiddenMonsInfo != NULL)
    {
        for (i = 0; i < HIDDEN_WILD_COUNT; ++i)
        {
            species = hiddenMonsInfo->wildPokemon[i].species;
            if (species != SPECIES_NONE)
            {
                count++;
                if (!GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
                    break;
            }
        }

        if (i >= HIDDEN_WILD_COUNT && count > 0)
            return TRUE;
    }
    else
    {
        return TRUE;    //technically, no mon data means you caught them all
    }

    return FALSE;
}

static void DexNavLoadCapturedAllSymbols(void)
{
    u16 headerId = GetCurrentMapWildMonHeaderId();
    

    if (CapturedAllLandMons(headerId))
        CreateSprite(&sCaptureAllMonsSpriteTemplate, 152, 58, 0);

    if (CapturedAllWaterMons(headerId))
        CreateSprite(&sCaptureAllMonsSpriteTemplate, 139, 17, 0);
    
    if (CapturedAllHiddenMons(headerId))
        CreateSprite(&sCaptureAllMonsSpriteTemplate, 90, 123, 0); // Gunk's: the HIDDEN box moved left (was 114)
}

//#define WIN_DETAILS_TILE        0x3a3
static void DexNav_InitWindows(void)
{
    InitWindows(sDexNavGuiWindowTemplates);
    DeactivateAllTextPrinters();
    ScheduleBgCopyTilemapToVram(0);
}

static void DexNavGuiFreeResources(void)
{
    FREE_AND_SET_NULL(sDexNavUiDataPtr); // Gunk's: NULL once closed (tests read it to tell the screen is open)
    FREE_AND_SET_NULL(sBg1TilemapBuffer);
    FreeAllWindowBuffers();
}

static void CB1_InitDexNavSearch(void)
{
    u8 taskId;
    
    if (!gPaletteFade.active && !ArePlayerFieldControlsLocked() && gMain.callback2 == CB2_Overworld)
    {
        SetMainCallback1(CB1_Overworld);
        taskId = CreateTask(Task_InitDexNavSearch, 0);
        gTasks[taskId].tSpecies = gSpecialVar_0x8000;
        gTasks[taskId].tEnvironment = gSpecialVar_0x8001;
    }
}

static void CB1_DexNavSearchCallback(void)
{
    CB1_InitDexNavSearch();
}

static void Task_DexNavExitAndSearch(u8 taskId)
{
    DexNavGuiFreeResources();
    DestroyTask(taskId);
    // Gunk's: a search paused by the START menu gives way to the new one (the screen reset its task)
    if (FlagGet(FLAG_SYS_DEXNAV_SEARCH))
        EndDexNavSearch(TASK_NONE);
    SetMainCallback1(CB1_DexNavSearchCallback);
    SetMainCallback2(CB2_ReturnToField);
}

static void Task_DexNavFadeAndExit(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetMainCallback2(sDexNavUiDataPtr->savedCallback);
        DexNavGuiFreeResources();
        DestroyTask(taskId);
    }
}

static void DexNavFadeAndExit(void)
{
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
    CreateTask(Task_DexNavFadeAndExit, 0);
    SetVBlankCallback(DexNav_VBlankCB);
    SetMainCallback2(DexNav_MainCB);
}

static bool8 SpeciesInArray(u16 species, u8 section)
{
    u32 i;
    u16 dexNum = SpeciesToNationalPokedexNum(species);
    
    switch (section)
    {
    case 0: //land
        for (i = 0; i < LAND_WILD_COUNT; i++)
        {
            if (SpeciesToNationalPokedexNum(sDexNavUiDataPtr->landSpecies[i]) == dexNum)
                return TRUE;
        }
        break;
    case 1: //water
        for (i = 0; i < WATER_WILD_COUNT; i++)
        {
            if (SpeciesToNationalPokedexNum(sDexNavUiDataPtr->waterSpecies[i]) == dexNum)
                return TRUE;
        }
        break;
    case 2: //hidden
        for (i = 0; i < HIDDEN_WILD_COUNT; i++)
        {
            if (SpeciesToNationalPokedexNum(sDexNavUiDataPtr->hiddenSpecies[i]) == dexNum)
                return TRUE;
        }
        break;
    default:
        break;
    }
    
    return FALSE;
}

// get unique wild encounters on current map
static void DexNavLoadEncounterData(void)
{
    u8 grassIndex = 0;
    u8 waterIndex = 0;
    u8 hiddenIndex = 0;
    u16 species;
    u32 i;
    u16 headerId = GetCurrentMapWildMonHeaderId();
    const struct WildPokemonInfo* landMonsInfo = gWildMonHeaders[headerId].landMonsInfo;
    const struct WildPokemonInfo* waterMonsInfo = gWildMonHeaders[headerId].waterMonsInfo;
    const struct WildPokemonInfo* hiddenMonsInfo = gWildMonHeaders[headerId].hiddenMonsInfo;
    
    // nop struct data
    memset(sDexNavUiDataPtr->landSpecies, 0, sizeof(sDexNavUiDataPtr->landSpecies));
    memset(sDexNavUiDataPtr->waterSpecies, 0, sizeof(sDexNavUiDataPtr->waterSpecies));
    memset(sDexNavUiDataPtr->hiddenSpecies, 0, sizeof(sDexNavUiDataPtr->hiddenSpecies));
    
    // land mons
    if (landMonsInfo != NULL && landMonsInfo->encounterRate != 0)
    {
        for (i = 0; i < LAND_WILD_COUNT; i++)
        {
            species = landMonsInfo->wildPokemon[i].species;
            if (species != SPECIES_NONE && !SpeciesInArray(species, 0))
                sDexNavUiDataPtr->landSpecies[grassIndex++] = landMonsInfo->wildPokemon[i].species;
        }
    }

    // water mons
    if (waterMonsInfo != NULL && waterMonsInfo->encounterRate != 0)
    {
        for (i = 0; i < WATER_WILD_COUNT; i++)
        {
            species = waterMonsInfo->wildPokemon[i].species;
            if (species != SPECIES_NONE && !SpeciesInArray(species, 1))
                sDexNavUiDataPtr->waterSpecies[waterIndex++] = waterMonsInfo->wildPokemon[i].species;
        }
    }
    
    // hidden mons
    if (hiddenMonsInfo != NULL) // no encounter rate check since 0 means land, 1 means water encounters
    {
        for (i = 0; i < HIDDEN_WILD_COUNT; i++)
        {
            species = hiddenMonsInfo->wildPokemon[i].species;
            if (species != SPECIES_NONE && !SpeciesInArray(species, 2))
                sDexNavUiDataPtr->hiddenSpecies[hiddenIndex++] = hiddenMonsInfo->wildPokemon[i].species;
        }
    }
}

// Gunk's: seen but not caught species show as a dark silhouette, like Gunk's Platinum's DexNav
#define SILHOUETTE_PAL_TAG 0x4007
static const u16 sSilhouettePal[16] =
{
    RGB(0, 0, 0), RGB(6, 7, 9), RGB(6, 7, 9), RGB(6, 7, 9), RGB(6, 7, 9), RGB(6, 7, 9), RGB(6, 7, 9), RGB(6, 7, 9),
    RGB(6, 7, 9), RGB(6, 7, 9), RGB(6, 7, 9), RGB(6, 7, 9), RGB(6, 7, 9), RGB(6, 7, 9), RGB(6, 7, 9), RGB(6, 7, 9),
};
static const struct SpritePalette sSilhouetteSpritePalette = {sSilhouettePal, SILHOUETTE_PAL_TAG};

static void TryDrawIconInSlot(u16 species, s16 x, s16 y)
{
    u8 spriteId;

    if (species == SPECIES_NONE || species > NUM_SPECIES)
    {
        CreateNoDataIcon(x, y);   //'X' in slot
    }
    else if (!GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_SEEN))
    {
        CreateMonIcon(SPECIES_NONE, SpriteCB_MonIcon, x, y, 1, 0xFFFFFFFF, 0); //question mark
    }
    else
    {
        spriteId = CreateMonIcon(species, SpriteCB_MonIcon, x, y, 1, 0xFFFFFFFF, 0);
        if (!GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
            gSprites[spriteId].oam.paletteNum = LoadSpritePalette(&sSilhouetteSpritePalette);
    }

    // Gunk's: a caught species gets a small Poke Ball in its cell's top left corner, in front of its icon
    if (species != SPECIES_NONE && species <= NUM_SPECIES
     && GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
        CreateSprite(&sCaptureAllMonsSpriteTemplate, x - 8, y - 2, 0);
}

static void DrawSpeciesIcons(void)
{
    s16 x, y;
    u32 i;
    u16 species;
    
    LoadCompressedSpriteSheetUsingHeap(&sNoDataIconSpriteSheet);
    LoadCompressedSpriteSheetUsingHeap(&sCapturedAllPokemonSpriteSheet); // Gunk's: also the caught marks
    for (i = 0; i < LAND_WILD_COUNT; i++)
    {
        species = sDexNavUiDataPtr->landSpecies[i];
        x = 20 + (24 * (i % 6));
        y = ROW_LAND_TOP_ICON_Y + (i > 5 ? 28 : 0);
        TryDrawIconInSlot(species, x, y);
    }
    
    for (i = 0; i < WATER_WILD_COUNT; i++)
    {
        species = sDexNavUiDataPtr->waterSpecies[i];
        x = 30 + 24 * i;
        y = ROW_WATER_ICON_Y;
        TryDrawIconInSlot(species, x, y);
    }
    
    for (i = 0; i < HIDDEN_WILD_COUNT; i++)
    {
        species = sDexNavUiDataPtr->hiddenSpecies[i];
        x = ROW_HIDDEN_ICON_X + 24 * i;
        y = ROW_HIDDEN_ICON_Y;
        if (FlagGet(FLAG_SYS_DETECTOR_MODE))
            TryDrawIconInSlot(species, x, y);
       else if (species == SPECIES_NONE || species > NUM_SPECIES)
            CreateNoDataIcon(x, y);
        else
            CreateMonIcon(SPECIES_NONE, SpriteCB_MonIcon, x, y, 0, 0xFFFFFFFF, 0); //question mark if detector mode inactive
    }
}

// Gunk's: the species in the highlighted cell, seen or not (SPECIES_NONE for an empty cell, or a HIDDEN cell
// without the detector)
static u16 DexNavGetSlotSpecies(void)
{
    switch (sDexNavUiDataPtr->cursorRow)
    {
    case ROW_WATER:
        return sDexNavUiDataPtr->waterSpecies[sDexNavUiDataPtr->cursorCol];
    case ROW_LAND_TOP:
        return sDexNavUiDataPtr->landSpecies[sDexNavUiDataPtr->cursorCol];
    case ROW_LAND_BOT:
        return sDexNavUiDataPtr->landSpecies[sDexNavUiDataPtr->cursorCol + COL_LAND_COUNT];
    case ROW_HIDDEN:
        if (!FlagGet(FLAG_SYS_DETECTOR_MODE))
            return SPECIES_NONE;
        return sDexNavUiDataPtr->hiddenSpecies[sDexNavUiDataPtr->cursorCol];
    default:
        return SPECIES_NONE;
    }
}

static u16 DexNavGetSpecies(void)
{
    u16 species = DexNavGetSlotSpecies();

    if (!GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_SEEN))
        return SPECIES_NONE;

    return species;
}

static void SetSpriteInvisibility(u8 spriteArrayId, bool8 invisible)
{
    gSprites[sDexNavUiDataPtr->typeIconSpriteIds[spriteArrayId]].invisible = invisible;
}

// different from pokemon_summary_screen
#define TYPE_ICON_PAL_NUM_0     13
#define TYPE_ICON_PAL_NUM_1     14
#define TYPE_ICON_PAL_NUM_2     15
static const u8 sMoveTypeToOamPaletteNum[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL] = TYPE_ICON_PAL_NUM_0,
    [TYPE_FIGHTING] = TYPE_ICON_PAL_NUM_0,
    [TYPE_FLYING] = TYPE_ICON_PAL_NUM_1,
    [TYPE_POISON] = TYPE_ICON_PAL_NUM_1,
    [TYPE_GROUND] = TYPE_ICON_PAL_NUM_0,
    [TYPE_ROCK] = TYPE_ICON_PAL_NUM_0,
    [TYPE_BUG] = TYPE_ICON_PAL_NUM_2,
    [TYPE_GHOST] = TYPE_ICON_PAL_NUM_1,
    [TYPE_STEEL] = TYPE_ICON_PAL_NUM_0,
    [TYPE_MYSTERY] = TYPE_ICON_PAL_NUM_2,
    [TYPE_FIRE] = TYPE_ICON_PAL_NUM_0,
    [TYPE_WATER] = TYPE_ICON_PAL_NUM_1,
    [TYPE_GRASS] = TYPE_ICON_PAL_NUM_2,
    [TYPE_ELECTRIC] = TYPE_ICON_PAL_NUM_0,
    [TYPE_PSYCHIC] = TYPE_ICON_PAL_NUM_1,
    [TYPE_ICE] = TYPE_ICON_PAL_NUM_1,
    [TYPE_DRAGON] = TYPE_ICON_PAL_NUM_2,
    [TYPE_DARK] = TYPE_ICON_PAL_NUM_0,
    #ifdef TYPE_FAIRY
    [TYPE_FAIRY] = TYPE_ICON_PAL_NUM_1, //based on battle_engine
    #endif
};
static void SetTypeIconPosAndPal(u8 typeId, u8 x, u8 y, u8 spriteArrayId)
{
    struct Sprite *sprite;
        
    sprite = &gSprites[sDexNavUiDataPtr->typeIconSpriteIds[spriteArrayId]];
    StartSpriteAnim(sprite, typeId);
    sprite->oam.paletteNum = sMoveTypeToOamPaletteNum[typeId];
    sprite->x = x + 16;
    sprite->y = y + 8;
    SetSpriteInvisibility(spriteArrayId, FALSE);
}

// Gunk's: the lowest and highest level a species has in this area's slots of one kind (FALSE if it has none)
static bool8 DexNavGetLevelRange(u16 species, u8 environment, u8 *minLevel, u8 *maxLevel)
{
    u16 headerId = GetCurrentMapWildMonHeaderId();
    const struct WildPokemonInfo *info;
    u32 i, count;

    *minLevel = MAX_LEVEL;
    *maxLevel = 0;
    if (headerId == 0xFFFF) // HEADER_NONE
        return FALSE;

    switch (environment)
    {
    case ENCOUNTER_TYPE_LAND:
        info = gWildMonHeaders[headerId].landMonsInfo;
        count = LAND_WILD_COUNT;
        break;
    case ENCOUNTER_TYPE_WATER:
        info = gWildMonHeaders[headerId].waterMonsInfo;
        count = WATER_WILD_COUNT;
        break;
    default:
        info = gWildMonHeaders[headerId].hiddenMonsInfo;
        count = HIDDEN_WILD_COUNT;
        break;
    }

    if (info == NULL)
        return FALSE;
    for (i = 0; i < count; i++)
    {
        if (info->wildPokemon[i].species == species)
        {
            *minLevel = min(*minLevel, info->wildPokemon[i].minLevel);
            *maxLevel = max(*maxLevel, info->wildPokemon[i].maxLevel);
        }
    }
    return *maxLevel != 0;
}

// Gunk's: the info panel, laid out like Gunk's Platinum's: the name on the red bar, then status, where it's found,
// its level range here, search level, chain, types and (once caught) abilities
#define INFO_TEXT_X     4
#define INFO_VALUE_X    32
#define INFO_NAME_Y     2
#define INFO_LINE_Y(n)  (16 + 12 * (n))
#define INFO_TYPES_Y    (16 + INFO_LINE_Y(5) + 1)  // screen y of the type icons: the window starts on row 2
#define INFO_ABILITY_Y  (INFO_LINE_Y(5) + 19)

static void PrintInfoLine(u8 line, const u8 *label, const u8 *value)
{
    if (label != NULL)
        AddTextPrinterParameterized3(WINDOW_INFO, FONT_SMALL, INFO_TEXT_X, INFO_LINE_Y(line), sFontColor_Black, 0, label);
    AddTextPrinterParameterized3(WINDOW_INFO, FONT_SMALL, label != NULL ? INFO_VALUE_X : INFO_TEXT_X, INFO_LINE_Y(line),
                                 sFontColor_Black, 0, value);
}

static void PrintCurrentSpeciesInfo(void)
{
    u16 slotSpecies = DexNavGetSlotSpecies();
    u16 species = DexNavGetSpecies();
    u16 dexNum = SpeciesToNationalPokedexNum(species);
    u8 environment = sDexNavUiDataPtr->environment;
    u8 minLevel, maxLevel;
    u8 type1, type2;
    const u8 *text;

    FillWindowPixelBuffer(WINDOW_INFO, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    SetSpriteInvisibility(0, TRUE);
    SetSpriteInvisibility(1, TRUE);

    if (slotSpecies == SPECIES_NONE || slotSpecies > NUM_SPECIES)
    {
        // an empty cell, or a HIDDEN cell without the detector
        AddTextPrinterParameterized3(WINDOW_INFO, FONT_SMALL, INFO_TEXT_X, INFO_NAME_Y, sFontColor_White, 0, sText_DexNav_NoInfo);
        CopyWindowToVram(WINDOW_INFO, 3);
        PutWindowTilemap(WINDOW_INFO);
        return;
    }

    // name and status
    AddTextPrinterParameterized3(WINDOW_INFO, FONT_SMALL, INFO_TEXT_X, INFO_NAME_Y, sFontColor_White, 0,
                                 species == SPECIES_NONE ? sText_DexNav_Unknown : gSpeciesNames[species]);
    if (species == SPECIES_NONE)
        text = sText_DexNav_StatusNotSeen;
    else if (GetSetPokedexFlag(dexNum, FLAG_GET_CAUGHT))
        text = sText_DexNav_StatusCaught;
    else
        text = sText_DexNav_StatusSeen;
    PrintInfoLine(0, NULL, text);

    // where and at what levels
    if (environment == ENCOUNTER_TYPE_WATER)
        text = sText_DexNav_FoundSurfing;
    else if (environment == ENCOUNTER_TYPE_HIDDEN)
        text = sText_DexNav_FoundHidden;
    else if (GetCurrentMapType() == MAP_TYPE_UNDERGROUND)
        text = sText_DexNav_FoundCave;
    else
        text = sText_DexNav_FoundGrass;
    PrintInfoLine(1, sText_DexNav_Found, text);

    if (DexNavGetLevelRange(slotSpecies, environment, &minLevel, &maxLevel))
    {
        ConvertIntToDecimalStringN(gStringVar1, minLevel, STR_CONV_MODE_LEFT_ALIGN, 3);
        ConvertIntToDecimalStringN(gStringVar2, maxLevel, STR_CONV_MODE_LEFT_ALIGN, 3);
        StringExpandPlaceholders(gStringVar4, minLevel == maxLevel ? sText_DexNav_LevelOne : sText_DexNav_LevelRange);
        PrintInfoLine(2, sText_DexNav_Level, gStringVar4);
    }

    // search level and chain
    ConvertIntToDecimalStringN(gStringVar1, species == SPECIES_NONE ? 0 : GetSearchLevel(dexNum), STR_CONV_MODE_LEFT_ALIGN, 4);
    StringExpandPlaceholders(gStringVar4, sText_DexNav_SearchLv);
    PrintInfoLine(3, NULL, gStringVar4);
    ConvertIntToDecimalStringN(gStringVar4, gSaveBlock1Ptr->dexNavChain, STR_CONV_MODE_LEFT_ALIGN, 3);
    PrintInfoLine(4, sText_DexNav_ChainLabel, gStringVar4);

    // types (??? until seen)
    type1 = gSpeciesInfo[species].types[0];
    type2 = gSpeciesInfo[species].types[1];
    if (species == SPECIES_NONE)
        type1 = type2 = TYPE_MYSTERY;
    SetTypeIconPosAndPal(type1, 164, INFO_TYPES_Y, 0);
    if (type1 != type2)
        SetTypeIconPosAndPal(type2, 164 + 36, INFO_TYPES_Y, 1);

    // Gunk's: the species' abilities, once caught (Gen 3 has no hidden abilities)
    if (species == SPECIES_NONE)
    {
        AddTextPrinterParameterized3(WINDOW_INFO, FONT_SMALL, INFO_TEXT_X, INFO_ABILITY_Y, sFontColor_Black, 0, sText_DexNav_NoInfo);
    }
    else if (GetSetPokedexFlag(dexNum, FLAG_GET_CAUGHT))
    {
        AddTextPrinterParameterized3(WINDOW_INFO, FONT_SMALL, INFO_TEXT_X, INFO_ABILITY_Y, sFontColor_Black, 0,
                                     gAbilityNames[gSpeciesInfo[species].abilities[0]]);
        if (gSpeciesInfo[species].abilities[1] != ABILITY_NONE)
            AddTextPrinterParameterized3(WINDOW_INFO, FONT_SMALL, INFO_TEXT_X, INFO_ABILITY_Y + 12, sFontColor_Black, 0,
                                         gAbilityNames[gSpeciesInfo[species].abilities[1]]);
    }
    else
    {
        AddTextPrinterParameterized3(WINDOW_INFO, FONT_SMALL, INFO_TEXT_X, INFO_ABILITY_Y, sFontColor_Black, 0, sText_DexNav_CaptureToSee);
    }

    CopyWindowToVram(WINDOW_INFO, 3);
    PutWindowTilemap(WINDOW_INFO);
}

// Gunk's: the title bar: the area's name, and how many of its species are caught (HIDDEN ones only with the
// detector, the same as the grid shows them)
static void PrintMapName(void)
{
    u16 counted[LAND_WILD_COUNT + WATER_WILD_COUNT + HIDDEN_WILD_COUNT];
    u32 i, j, total = 0, caught = 0;
    u16 species;

    for (i = 0; i < NELEMS(counted); i++)
    {
        if (i < LAND_WILD_COUNT)
            species = sDexNavUiDataPtr->landSpecies[i];
        else if (i < LAND_WILD_COUNT + WATER_WILD_COUNT)
            species = sDexNavUiDataPtr->waterSpecies[i - LAND_WILD_COUNT];
        else if (FlagGet(FLAG_SYS_DETECTOR_MODE))
            species = sDexNavUiDataPtr->hiddenSpecies[i - LAND_WILD_COUNT - WATER_WILD_COUNT];
        else
            species = SPECIES_NONE;
        if (species == SPECIES_NONE || species > NUM_SPECIES)
            continue;
        for (j = 0; j < total && counted[j] != species; j++)
            ;
        if (j < total)
            continue;
        counted[total++] = species;
        if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
            caught++;
    }

    FillWindowPixelBuffer(WINDOW_REGISTERED, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    // Gunk's: a swarm (the TV's mass outbreak) shows here every other 3 s: which Pokémon, and where
    if (gSaveBlock1Ptr->outbreakPokemonSpecies != SPECIES_NONE && (sDexNavUiDataPtr->titleTimer / GUNK_SWARM_TITLE_FRAMES) % 2)
    {
        StringCopy(gStringVar1, gSpeciesNames[gSaveBlock1Ptr->outbreakPokemonSpecies]);
        GetMapName(gStringVar2, Overworld_GetMapHeaderByGroupAndId(gSaveBlock1Ptr->outbreakLocationMapGroup,
                                                                   gSaveBlock1Ptr->outbreakLocationMapNum)->regionMapSectionId, 0);
        StringExpandPlaceholders(gStringVar4, sText_DexNav_Swarm);
        AddTextPrinterParameterized3(WINDOW_REGISTERED, FONT_NORMAL, 4, 0, sFontColor_White, 0, gStringVar4);
        PutWindowTilemap(WINDOW_REGISTERED);
        CopyWindowToVram(WINDOW_REGISTERED, 3);
        return;
    }
    GetMapName(gStringVar3, GetCurrentRegionMapSectionId(), 0);
    AddTextPrinterParameterized3(WINDOW_REGISTERED, FONT_NORMAL, 4, 0, sFontColor_White, 0, gStringVar3);
    ConvertIntToDecimalStringN(gStringVar1, caught, STR_CONV_MODE_LEFT_ALIGN, 2);
    ConvertIntToDecimalStringN(gStringVar2, total, STR_CONV_MODE_LEFT_ALIGN, 2);
    StringExpandPlaceholders(gStringVar4, sText_DexNav_Caught);
    AddTextPrinterParameterized3(WINDOW_REGISTERED, FONT_NORMAL, 236 - GetStringWidth(FONT_NORMAL, gStringVar4, 0), 0,
                                 sFontColor_White, 0, gStringVar4);
    PutWindowTilemap(WINDOW_REGISTERED);
    CopyWindowToVram(WINDOW_REGISTERED, 3);
}

// Gunk's: the search panel under the grids: what R searches for in the field, and the buttons
static void PrintSearchableSpecies(u16 species)
{
    FillWindowPixelBuffer(WINDOW_HINTS, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    if (species == SPECIES_NONE)
    {
        AddTextPrinterParameterized3(WINDOW_HINTS, FONT_SMALL, 0, 0, sFontColor_White, TEXT_SKIP_DRAW, sText_DexNav_HintR);
    }
    else
    {
        StringCopy(gStringVar1, gSpeciesNames[species]);
        StringExpandPlaceholders(gStringVar4, sText_DexNav_SearchForRegisteredSpecies);
        AddTextPrinterParameterized3(WINDOW_HINTS, FONT_SMALL, 0, 0, sFontColor_White, TEXT_SKIP_DRAW, gStringVar4);
    }
    AddTextPrinterParameterized3(WINDOW_HINTS, FONT_SMALL, 0, 10, sFontColor_White, TEXT_SKIP_DRAW, sText_DexNav_HintA);
    AddTextPrinterParameterized3(WINDOW_HINTS, FONT_SMALL, 0, 20, sFontColor_White, TEXT_SKIP_DRAW, sText_DexNav_HintB);
    PutWindowTilemap(WINDOW_HINTS);
    CopyWindowToVram(WINDOW_HINTS, 3);
}

static void CreateTypeIconSprites(void)
{
    u8 i;

    LoadCompressedSpriteSheet(&gSpriteSheet_MoveTypes);
    LoadCompressedPalette(gMoveTypes_Pal, 0x1D0, 0x60);
    for (i = 0; i < 2; i++)
    {
        if (sDexNavUiDataPtr->typeIconSpriteIds[i] == 0xFF)
            sDexNavUiDataPtr->typeIconSpriteIds[i] = CreateSprite(&gSpriteTemplate_MoveTypes, 10, 10, 2);    
    
        SetSpriteInvisibility(i, TRUE);
    }
}

static bool8 DexNav_DoGfxSetup(void)
{
    u8 taskId;
    
    switch (gMain.state)
    {
    case 0:
        SetVBlankHBlankCallbacksToNull();
        ClearScheduledBgCopiesToVram();
        gMain.state++;
        break;
    case 1:
        ScanlineEffect_Stop();
        gMain.state++;
        break;
    case 2:
        FreeAllSpritePalettes();
        gMain.state++;
        break;
    case 3:
        ResetPaletteFade();
        ResetSpriteData();
        ResetTasks();
        gMain.state++;
        break;
    case 4:
        if (DexNav_InitBgs())
        {
            sDexNavUiDataPtr->state = 0;
            gMain.state++;
        }
        else
        {
            DexNavFadeAndExit();
            return TRUE;
        }
        break;
    case 5:
        if (DexNav_LoadGraphics() == TRUE)
            gMain.state++;
        break;
    case 6:
        DexNav_InitWindows();
        sDexNavUiDataPtr->cursorRow = ROW_LAND_TOP;
        sDexNavUiDataPtr->cursorCol = 0;
        sDexNavUiDataPtr->environment = ENCOUNTER_TYPE_LAND;
        gMain.state++;
        break;
    case 7:
        DexNavLoadEncounterData();
        PrintMapName();
        PrintSearchableSpecies(VarGet(VAR_DEXNAV_SPECIES) & MASK_SPECIES);
        gMain.state++;
        break;
    case 8:
        taskId = CreateTask(Task_DexNavWaitFadeIn, 0);
        gTasks[taskId].tSpecies = 0;
        gTasks[taskId].tEnvironment = sDexNavUiDataPtr->environment;
        gMain.state++;
        break;
    case 9:
        sDexNavUiDataPtr->typeIconSpriteIds[0] = 0xFF;
        sDexNavUiDataPtr->typeIconSpriteIds[1] = 0xFF;
        CreateTypeIconSprites();
        gMain.state++;
        break;
    case 10:
        LoadMonIconPalettes();
        DrawSpeciesIcons();
        CreateSelectionCursor();
        DexNavLoadCapturedAllSymbols();
        gMain.state++;
        break;
    case 11:
        BlendPalettes(0xFFFFFFFF, 16, RGB_BLACK);
        gMain.state++;
        break;
    case 12:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    default:
        SetVBlankCallback(DexNav_VBlankCB);
        SetMainCallback2(DexNav_MainCB);
        return TRUE;
    }
    
    return FALSE;
}

static void DexNav_RunSetup(void)
{
    while (!DexNav_DoGfxSetup()) {}
}

static void DexNavGuiInit(MainCallback callback)
{
    if ((sDexNavUiDataPtr = AllocZeroed(sizeof(struct DexNavGUI))) == NULL)
    {
        SetMainCallback2(callback);
        return;
    }
    
    sDexNavUiDataPtr->state = 0;
    sDexNavUiDataPtr->savedCallback = callback;
    SetMainCallback2(DexNav_RunSetup);
}

// Gunk's: from the DEXNAV key item in the bag
void CB2_OpenDexNavFromBag(void)
{
    DexNavGuiInit(CB2_ReturnToBagMenuPocket);
}

// Gunk's: from the DEXNAV key item registered to SELECT; B goes straight back to the field
void Task_OpenDexNavFromField(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        DexNavGuiInit(CB2_ReturnToField);
        DestroyTask(taskId);
    }
}

void Task_OpenDexNavFromStartMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        DexNavGuiInit(CB2_ReturnToFieldWithOpenMenu);
        DestroyTask(taskId);
    }
}

static void Task_DexNavWaitFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_DexNavMain;
}

static void Task_DexNavMain(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    u16 species;

    if (gSaveBlock1Ptr->outbreakPokemonSpecies != SPECIES_NONE
     && ++sDexNavUiDataPtr->titleTimer % GUNK_SWARM_TITLE_FRAMES == 0)
        PrintMapName();
    
    if (IsSEPlaying())
        return;
    
    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_POKENAV_OFF);
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
        task->func = Task_DexNavFadeAndExit;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (sDexNavUiDataPtr->cursorRow == ROW_WATER)
        {
            sDexNavUiDataPtr->cursorRow = ROW_HIDDEN;
            if (sDexNavUiDataPtr->cursorCol >= COL_HIDDEN_COUNT)
                sDexNavUiDataPtr->cursorCol = COL_HIDDEN_MAX;
        }
        else
        {
            if (sDexNavUiDataPtr->cursorRow == ROW_LAND_TOP && sDexNavUiDataPtr->cursorCol == COL_LAND_MAX)
                sDexNavUiDataPtr->cursorCol = COL_WATER_MAX;
            
            sDexNavUiDataPtr->cursorRow--;
        }
        
        PlaySE(SE_RG_BAG_CURSOR);
        UpdateCursorPosition();
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (sDexNavUiDataPtr->cursorRow == ROW_HIDDEN)
        {
            sDexNavUiDataPtr->cursorRow = ROW_WATER;
        }
        else if (sDexNavUiDataPtr->cursorRow == ROW_LAND_BOT)
        {
            if (sDexNavUiDataPtr->cursorCol >= COL_HIDDEN_COUNT)
                sDexNavUiDataPtr->cursorCol = COL_HIDDEN_MAX;
            
            sDexNavUiDataPtr->cursorRow++;
        }
        else
        {
            sDexNavUiDataPtr->cursorRow++;
        }
        
        PlaySE(SE_RG_BAG_CURSOR);
        UpdateCursorPosition();
    }
    else if (JOY_NEW(DPAD_LEFT))
    {
        if (sDexNavUiDataPtr->cursorCol == 0)
        {
            switch (sDexNavUiDataPtr->cursorRow)
            {
            case ROW_WATER:
                sDexNavUiDataPtr->cursorCol = COL_WATER_MAX;
                break;
            case ROW_HIDDEN:
                sDexNavUiDataPtr->cursorCol = COL_HIDDEN_MAX;
                break;
            default:
                sDexNavUiDataPtr->cursorCol = COL_LAND_MAX;
                break;
            }
        }
        else
        {
            sDexNavUiDataPtr->cursorCol--;
        }
        
        PlaySE(SE_RG_BAG_CURSOR);
        UpdateCursorPosition();
    }
    else if (JOY_NEW(DPAD_RIGHT))
    {
        switch (sDexNavUiDataPtr->cursorRow)
        {
        case ROW_WATER:
            if (sDexNavUiDataPtr->cursorCol == COL_WATER_MAX)
                sDexNavUiDataPtr->cursorCol = 0;
            else
                sDexNavUiDataPtr->cursorCol++;
            break;
        case ROW_HIDDEN:
            if (sDexNavUiDataPtr->cursorCol == COL_HIDDEN_MAX)
                sDexNavUiDataPtr->cursorCol = 0;
            else
                sDexNavUiDataPtr->cursorCol++;
            break;
        default:
            if (sDexNavUiDataPtr->cursorCol == COL_LAND_MAX)
                sDexNavUiDataPtr->cursorCol = 0;
            else
                sDexNavUiDataPtr->cursorCol++;
            break;
        }
        
        PlaySE(SE_RG_BAG_CURSOR);
        UpdateCursorPosition();
    }
    else if (JOY_NEW(R_BUTTON))
    {
        // check selection is valid. Play sound if invalid
        species = DexNavGetSpecies();
        
        if (species != SPECIES_NONE)
        {            
            PrintSearchableSpecies(species);
            //PlaySE(SE_DEX_SEARCH);
            PlayCry_Script(species, 0);
            
            // create value to store in a var
            VarSet(VAR_DEXNAV_SPECIES, ((sDexNavUiDataPtr->environment << 14) | species));
        }
        else
        {
            PlaySE(SE_FAILURE);
        }
    }
    else if (JOY_NEW(A_BUTTON))
    {
        species = DexNavGetSpecies();
        if (species == SPECIES_NONE)
        {
            PlaySE(SE_FAILURE);
        }
        else
        {
            gSpecialVar_0x8000 = species;
            gSpecialVar_0x8001 = sDexNavUiDataPtr->environment;
            gSpecialVar_0x8002 = (sDexNavUiDataPtr->cursorRow == ROW_HIDDEN) ? TRUE : FALSE;
            PlaySE(SE_DEX_SEARCH);
            BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
            task->func = Task_DexNavExitAndSearch;
        }
    }
}

/////////////////////////
//// HIDDEN POKEMON /////
/////////////////////////
bool8 TryFindHiddenPokemon(void)
{
    u16 *stepPtr = GetVarPointer(VAR_DEXNAV_STEP_COUNTER);
    u32 attempts = 0;
    u16 currSteps;

    if (!FlagGet(FLAG_SYS_DETECTOR_MODE) || FlagGet(FLAG_SYS_DEXNAV_SEARCH) || GetFlashLevel() > 0)
    {
        (*stepPtr) = 0;
        return FALSE;
    }
    
    (*stepPtr)++;
    (*stepPtr) %= HIDDEN_MON_STEP_COUNT;
    if ((*stepPtr) == 0 && (Random() % 100 < HIDDEN_MON_SEARCH_RATE))
    {
        // hidden pokemon
        u16 headerId = GetCurrentMapWildMonHeaderId();
        u8 index;
        u16 species;
        u8 environment;
        u8 taskId;
        u8 total;
        const struct WildPokemonInfo* hiddenMonsInfo = gWildMonHeaders[headerId].hiddenMonsInfo;
        bool8 isHiddenMon = FALSE;
        
        //while you can still technically find hidden pokemon if there are not hidden-only pokemon on a map,
        // this prevents any potential lagging on maps you dont want hidden pokemon to appear on
        if (hiddenMonsInfo == NULL)
            return FALSE;
        
        //encounter rate signifies surfing (1) or land mons (0)!
        // again, for simplicity
        switch (hiddenMonsInfo->encounterRate)
        {
        case 0: //land
            // there are surely better ways to do this, but this allows greatest flexibility
            if (Random() % 100 < HIDDEN_MON_PROBABILTY)
            {
                index = ChooseHiddenMonIndex();
                if (index == 0xFF)
                    return FALSE;//no hidden info
                species = hiddenMonsInfo->wildPokemon[index].species;
                isHiddenMon = TRUE;
                environment = ENCOUNTER_TYPE_HIDDEN;
            }
            else
            {
                species = gWildMonHeaders[headerId].landMonsInfo->wildPokemon[ChooseWildMonIndex_Land()].species;
                environment = ENCOUNTER_TYPE_LAND;
            }
            break;
        case 1: //water
            if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
            {
                if (Random() % 100 < HIDDEN_MON_PROBABILTY)
                {
                    index = ChooseHiddenMonIndex();
                    if (index == 0xFF)
                        return FALSE;//no hidden info
                    species = hiddenMonsInfo->wildPokemon[index].species;
                    isHiddenMon = TRUE;
                    environment = ENCOUNTER_TYPE_HIDDEN;
                }
                else
                {
                    species = gWildMonHeaders[headerId].waterMonsInfo->wildPokemon[ChooseWildMonIndex_WaterRock()].species;
                    environment = ENCOUNTER_TYPE_WATER;

                }
            }
            else
            {
                return FALSE;   //not surfing -> cant find hidden water mons
            }
            break;
        default:
            return FALSE;
        }
        
        if (species == SPECIES_NONE)
            return FALSE;
        
        sDexNavSearchDataPtr = &sDexNavSearchData;
        memset(sDexNavSearchDataPtr, 0, sizeof(*sDexNavSearchDataPtr));
                
        // init search data
        sDexNavSearchDataPtr->isHiddenMon = isHiddenMon;
        sDexNavSearchDataPtr->species = species;
        sDexNavSearchDataPtr->hiddenSearch = TRUE;
        sDexNavSearchDataPtr->environment = environment;  //updated in DexNavTryGenerateMonLevel if hidden mon
        sDexNavSearchDataPtr->monLevel = DexNavTryGenerateMonLevel(species, environment);
        if (sDexNavSearchDataPtr->monLevel == MON_LEVEL_NONEXISTENT)
        {
            sDexNavSearchDataPtr = NULL;
            return FALSE;
        }

        // find tile for hidden mon and start effect if possible
        while (1) {
            if (TryStartHiddenMonFieldEffect(sDexNavSearchDataPtr->environment, 8, 8, TRUE))
                break;
            if (++attempts > 20)
                return FALSE;   //cannot find suitable tile
        }

        // exclamation mark over player
        gFieldEffectArguments[0] = gSaveBlock1Ptr->pos.x;
        gFieldEffectArguments[1] = gSaveBlock1Ptr->pos.y;
        gFieldEffectArguments[2] = gSprites[gPlayerAvatar.spriteId].subpriority - 1;
        gFieldEffectArguments[3] = 2;
        ObjectEventGetLocalIdAndMap(&gObjectEvents[gPlayerAvatar.objectEventId], &gFieldEffectArguments[0], &gFieldEffectArguments[1], &gFieldEffectArguments[2]);
        FieldEffectStart(FLDEFF_EXCLAMATION_MARK_ICON);
        
        PlayCry_Script(species, 0);
        taskId = CreateTask(Task_SetUpDexNavSearch, 0);
        gTasks[taskId].tSpecies = sDexNavSearchDataPtr->species;
        gTasks[taskId].tEnvironment = sDexNavSearchDataPtr->environment;
        gTasks[taskId].tRevealed = FALSE;
        HideMapNamePopUpWindow();
        ChangeBgY_ScreenOff(0, 0, 0);
        return FALSE;   //we dont actually want to enable the script context or the game will freeze
    }
    
    return FALSE;
}

static void DrawSearchIcon(void)
{
    struct CompressedSpriteSheet spriteSheet;       
    
    spriteSheet.data = sHiddenSearchIconGfx;
    spriteSheet.size = 0x200;
    spriteSheet.tag = SELECTION_CURSOR_TAG;
    LoadCompressedSpriteSheet(&spriteSheet);
    sDexNavSearchDataPtr->iconSpriteId = CreateSprite(&sSearchIconSpriteTemplate, 18, GetSearchWindowY() + 12, 0);
}

// the initial hidden icon window ONLY shows search icon, ??? instead of name, and the search level (and pokeball icon if owned)
//  if the player presses R or moves close enough, the full search window will be created
//  this way, if the player is not interested in hidden pokemon it will not be too intrusive
static void DrawHiddenSearchText(void)
{
    AddTextPrinterParameterized3(sDexNavSearchDataPtr->windowId, 0, SPECIES_ICON_X + 4, 0, sSearchFontColor, TEXT_SKIP_DRAW, sText_ThreeQmarks);
    
    ConvertIntToDecimalStringN(gStringVar1, sDexNavSearchDataPtr->searchLevel, STR_CONV_MODE_LEFT_ALIGN, 2);
    StringExpandPlaceholders(gStringVar4, sText_SearchLevel);
    AddTextPrinterParameterized3(sDexNavSearchDataPtr->windowId, 0, SPECIES_ICON_X + 4, 12, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar4);
}

static void DrawHiddenSearchWindow(u8 width)
{
    AddSearchWindow(width);
    DrawHiddenSearchText();
    CopyWindowToVram(sDexNavSearchDataPtr->windowId, 2);
}

static void DexNavDrawHiddenIcons(void)
{
    u8 searchLevel = sDexNavSearchDataPtr->searchLevel;
    u16 species = sDexNavSearchDataPtr->species;
    
    DrawHiddenSearchWindow(12);
    DrawSearchIcon();
    
    if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
        sDexNavSearchDataPtr->ownedIconSpriteId = CreateSprite(&sOwnedIconTemplate, SPECIES_ICON_X + 6, GetSearchWindowY() + 2, 0);
    
    if (sDexNavSearchDataPtr->isHiddenMon)
        sDexNavSearchDataPtr->exclamationSpriteId = CreateSprite(&sHiddenMonIconTemplate, SPECIES_ICON_X + 34, GetSearchWindowY() + 8, 0);
}

/////////////////////////
//// GENERAL UTILITY ////
/////////////////////////
static u32 GunkDexNavShinyRolls(void)
{
    u32 shinyRolls, chainBonus, rndBonus;
    u32 shinyRate = 0;
    u32 charmBonus = 0;
    u8 searchLevel = sDexNavSearchDataPtr->searchLevel;
    u8 chain = gSaveBlock1Ptr->dexNavChain;
    
    #ifdef ITEM_SHINY_CHARM
    charmBonus = (CheckBagHasItem(ITEM_SHINY_CHARM, 1) > 0) ? 2 : 0;
    #endif
    
    chainBonus = (chain == 50) ? 5 : (chain == 100) ? 10 : 0;
    rndBonus = (Random() % 100 < 4 ? 4 : 0);
    shinyRolls = 1 + charmBonus + chainBonus + rndBonus;

    if (searchLevel > 200)
    {
        shinyRate += searchLevel - 200;
        searchLevel = 200;
    }
    if (searchLevel > 100)
    {
        shinyRate += (searchLevel * 2) - 200;
        searchLevel = 100;
    }
    if (searchLevel > 0)
    {
        shinyRate += searchLevel * 6;
    }
    
    // Gunk's: shinyRate / 100 per 10000 per roll; a normal roll is shiny 1 time in 8192 (1.22 in 10000)
    return 1 + (shinyRolls * shinyRate * 8192) / (100 * 10000);
}

void TryIncrementSpeciesSearchLevel(u16 dexNum)
{
#if USE_DEXNAV_SEARCH_LEVELS == TRUE
    if (gMapHeader.regionMapSectionId != MAPSEC_BATTLE_FRONTIER && gPokemonStoragePtr->dexNavSearchLevels[dexNum] < 255)
        gPokemonStoragePtr->dexNavSearchLevels[dexNum]++;
#endif
}

void ResetDexNavSearch(void)
{
    // Gunk's: the chain survives map changes (it resets on a timeout or a wild encounter of another species)
    VarSet(VAR_DEXNAV_STEP_COUNTER, 0); //reset hidden pokemon step counter
    if (FlagGet(FLAG_SYS_DEXNAV_SEARCH))
        EndDexNavSearch(FindTaskIdByFunc(Task_DexNavSearch));   //moving to new map ends dexnav search
}

void IncrementDexNavChain(void)
{
    if (gSaveBlock1Ptr->dexNavChain < DEXNAV_CHAIN_MAX)
        gSaveBlock1Ptr->dexNavChain++;
}

// Gunk's: after any battle. A won or caught DexNav battle adds one to the chain and any other end (fleeing, the
// Pokemon fleeing, losing) resets it; any other wild battle keeps the chain if it was the registered species and
// resets it if not. Trainer battles leave it alone.
void GunkDexNavUpdateChainAfterBattle(void)
{
    if (gDexnavBattle)
    {
        if (gBattleOutcome == B_OUTCOME_WON || gBattleOutcome == B_OUTCOME_CAUGHT)
            IncrementDexNavChain();
        else
            gSaveBlock1Ptr->dexNavChain = 0;
        return;
    }
    if (gBattleTypeFlags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_SAFARI))
        return;
    if (GetMonData(&gEnemyParty[0], MON_DATA_SPECIES) != (VarGet(VAR_DEXNAV_SPECIES) & MASK_SPECIES))
        gSaveBlock1Ptr->dexNavChain = 0;
}
