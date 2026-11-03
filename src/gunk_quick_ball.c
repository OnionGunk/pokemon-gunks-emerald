#include "global.h"
#include "battle.h"
#include "event_data.h"
#include "gunk_quick_ball.h"
#include "item.h"
#include "item_icon.h"
#include "item_menu.h"
#include "menu.h"
#include "pokemon.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "text.h"
#include "window.h"
#include "constants/items.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// Gunk's: Quick Run and Quick Ball on the action menu.
// R runs exactly as RUN does. In a wild battle where balls can be thrown, a widget at the bottom left shows the
// selected ball and how many are in the bag; tapping L throws it through the normal item path (the bag's own
// room check, then PlayerHandleChooseItem hands the ball back without opening the bag), and L + Left/Right
// picks another ball. The ball is remembered in VAR_GUNK_LAST_BALL. Not in L=A button mode, where L is A.
#define TAG_GUNK_BALL_ICON 0xD7A0
#define TAG_GUNK_BALL_TEXT 0xD7A1
#define GUNK_BALL_WIDGET_X 16
#define GUNK_BALL_WIDGET_Y 88

static const struct OamData sGunkBallTextOam =
{
    .shape = SPRITE_SHAPE(32x16),
    .size = SPRITE_SIZE(32x16),
    .priority = 0,
};

static const struct SpriteTemplate sGunkBallTextTemplate =
{
    .tileTag = TAG_GUNK_BALL_TEXT,
    .paletteTag = TAG_GUNK_BALL_TEXT,
    .oam = &sGunkBallTextOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const u16 sGunkBallTextPal[16] = {RGB_BLACK, RGB_WHITE, RGB(7, 7, 7)};
static const u8 sGunkBallTextFormat[] = _("L x{STR_VAR_1}");

// sprite id + 1, 0 = not shown (EWRAM starts zeroed: initializers don't apply)
EWRAM_DATA static u8 sGunkBallIconSprite = 0;
EWRAM_DATA static u8 sGunkBallTextSprite = 0;
EWRAM_DATA static bool8 sGunkQuickBallPending = FALSE;
EWRAM_DATA static bool8 sGunkLHeld = FALSE;
EWRAM_DATA static bool8 sGunkLCycled = FALSE;

static bool32 GunkQuickBallBattle(void)
{
    if (gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_L_EQUALS_A)
        return FALSE;
    if (gBattleTypeFlags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_SAFARI | BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED
                          | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_FIRST_BATTLE | BATTLE_TYPE_WALLY_TUTORIAL
                          | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_TRAINER_HILL))
        return FALSE;
    return TRUE;
}

// Quick Ball's ball order: the everyday balls first, the Master Ball last; the Safari Ball isn't thrown this way
static const u16 sGunkQuickBalls[] =
{
    ITEM_POKE_BALL, ITEM_GREAT_BALL, ITEM_ULTRA_BALL, ITEM_NET_BALL, ITEM_DIVE_BALL, ITEM_NEST_BALL,
    ITEM_REPEAT_BALL, ITEM_TIMER_BALL, ITEM_LUXURY_BALL, ITEM_PREMIER_BALL, ITEM_MASTER_BALL,
};

// The ball to show: the remembered one if the bag still has it, else the first in sGunkQuickBalls the bag has
static u16 GunkQuickBallItem(void)
{
    u16 item = VarGet(VAR_GUNK_LAST_BALL);
    u32 i;

    if (item >= FIRST_BALL && item <= LAST_BALL && item != ITEM_SAFARI_BALL && CheckBagHasItem(item, 1))
        return item;
    for (i = 0; i < ARRAY_COUNT(sGunkQuickBalls); i++)
    {
        if (CheckBagHasItem(sGunkQuickBalls[i], 1))
            return sGunkQuickBalls[i];
    }
    return ITEM_NONE;
}

void Gunk_QuickBall_Hide(void)
{
    if (sGunkBallIconSprite != 0)
    {
        DestroySprite(&gSprites[sGunkBallIconSprite - 1]);
        FreeSpriteTilesByTag(TAG_GUNK_BALL_ICON);
        FreeSpritePaletteByTag(TAG_GUNK_BALL_ICON);
        sGunkBallIconSprite = 0;
    }
    if (sGunkBallTextSprite != 0)
    {
        DestroySprite(&gSprites[sGunkBallTextSprite - 1]);
        FreeSpriteTilesByTag(TAG_GUNK_BALL_TEXT);
        FreeSpritePaletteByTag(TAG_GUNK_BALL_TEXT);
        sGunkBallTextSprite = 0;
    }
}

void Gunk_QuickBall_Show(void)
{
    static const struct WindowTemplate sTextWindow = {.bg = 0, .width = 4, .height = 2, .paletteNum = 0};
    static const u8 sColors[3] = {0, 1, 2};
    struct SpriteSheet sheet = {.size = 4 * 2 * TILE_SIZE_4BPP, .tag = TAG_GUNK_BALL_TEXT};
    struct SpritePalette palette = {.data = sGunkBallTextPal, .tag = TAG_GUNK_BALL_TEXT};
    u16 item;
    u8 windowId, spriteId;

    Gunk_QuickBall_Hide();
    sGunkLHeld = FALSE;
    if (!GunkQuickBallBattle() || (item = GunkQuickBallItem()) == ITEM_NONE)
        return;

    spriteId = AddItemIconSprite(TAG_GUNK_BALL_ICON, TAG_GUNK_BALL_ICON, item);
    if (spriteId == MAX_SPRITES)
        return;
    gSprites[spriteId].x = GUNK_BALL_WIDGET_X;
    gSprites[spriteId].y = GUNK_BALL_WIDGET_Y;
    gSprites[spriteId].oam.priority = 0;
    gSprites[spriteId].subpriority = 0;
    sGunkBallIconSprite = spriteId + 1;

    // "L x99": text drawn in a window buffer, then copied into a 32x16 sprite's tiles
    windowId = AddWindow(&sTextWindow);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
    ConvertIntToDecimalStringN(gStringVar1, min(CountTotalItemQuantityInBag(item), 99), STR_CONV_MODE_LEFT_ALIGN, 2);
    StringExpandPlaceholders(gStringVar4, sGunkBallTextFormat);
    AddTextPrinterParameterized4(windowId, FONT_SMALL, 0, 2, 0, 0, sColors, TEXT_SKIP_DRAW, gStringVar4);
    sheet.data = (const void *)GetWindowAttribute(windowId, WINDOW_TILE_DATA);
    LoadSpriteSheet(&sheet);
    RemoveWindow(windowId);
    LoadSpritePalette(&palette);
    spriteId = CreateSprite(&sGunkBallTextTemplate, GUNK_BALL_WIDGET_X + 26, GUNK_BALL_WIDGET_Y + 4, 0);
    if (spriteId != MAX_SPRITES)
        sGunkBallTextSprite = spriteId + 1;
}

static void GunkCycleQuickBall(s8 delta)
{
    u16 item = GunkQuickBallItem();
    s32 i, pos = 0, count = ARRAY_COUNT(sGunkQuickBalls);

    if (item == ITEM_NONE)
        return;
    for (i = 0; i < count; i++)
    {
        if (sGunkQuickBalls[i] == item)
            pos = i;
    }
    for (i = 0; i < count; i++)
    {
        pos = (pos + delta + count) % count;
        if (CheckBagHasItem(sGunkQuickBalls[pos], 1))
            break;
    }
    VarSet(VAR_GUNK_LAST_BALL, sGunkQuickBalls[pos]);
    PlaySE(SE_SELECT);
    Gunk_QuickBall_Show();
    sGunkLHeld = TRUE;
    sGunkLCycled = TRUE;
}

u8 Gunk_QuickBall_HandleInput(void)
{
    if (JOY_NEW(R_BUTTON))
    {
        PlaySE(SE_SELECT);
        Gunk_QuickBall_Hide();
        return GUNK_QUICK_BALL_INPUT_RUN;
    }
    if (sGunkBallIconSprite == 0)
        return GUNK_QUICK_BALL_INPUT_NONE;
    if (JOY_NEW(L_BUTTON))
    {
        sGunkLHeld = TRUE;
        sGunkLCycled = FALSE;
    }
    if (sGunkLHeld && JOY_HELD(L_BUTTON))
    {
        if (JOY_NEW(DPAD_LEFT))
            GunkCycleQuickBall(-1);
        else if (JOY_NEW(DPAD_RIGHT))
            GunkCycleQuickBall(1);
        return GUNK_QUICK_BALL_INPUT_USED;
    }
    if (sGunkLHeld) // L released
    {
        sGunkLHeld = FALSE;
        if (sGunkLCycled)
            return GUNK_QUICK_BALL_INPUT_USED;
        if (IsPlayerPartyAndPokemonStorageFull()) // the bag's own check for balls
        {
            PlaySE(SE_FAILURE);
            return GUNK_QUICK_BALL_INPUT_USED;
        }
        PlaySE(SE_SELECT);
        VarSet(VAR_GUNK_LAST_BALL, GunkQuickBallItem());
        Gunk_QuickBall_Hide();
        sGunkQuickBallPending = TRUE;
        return GUNK_QUICK_BALL_INPUT_THROW;
    }
    return GUNK_QUICK_BALL_INPUT_NONE;
}

bool32 Gunk_QuickBall_TakePending(u16 *ball)
{
    if (!sGunkQuickBallPending)
        return FALSE;
    sGunkQuickBallPending = FALSE;
    *ball = VarGet(VAR_GUNK_LAST_BALL);
    RemoveBagItem(*ball, 1);
    gSpecialVar_ItemId = *ball;
    return TRUE;
}

void Gunk_QuickBall_NoteItemUsed(u16 item)
{
    if (item >= FIRST_BALL && item <= LAST_BALL && item != ITEM_SAFARI_BALL)
        VarSet(VAR_GUNK_LAST_BALL, item);
}
