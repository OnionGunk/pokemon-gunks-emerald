EventScript_StartDexNavBattle::
	lock
	playse 0x15
	applymovement OBJ_EVENT_ID_PLAYER Common_Movement_ExclamationMark
	waitmovement 0
	waitse
	dowildbattle    
	release
	end


EventScript_NotFoundNearby::
    msgbox sText_NotFoundNearby, MSGBOX_SIGN
    end
sText_NotFoundNearby:
    .string "It couldn't be found nearby.\n"
    .string "Try looking in a different area!$"


EventScript_MovedTooFast::
    msgbox sText_TryMovingSlower, MSGBOX_SIGN
    end
sText_TryMovingSlower:
    .string "The Pokémon got away!\n"
    .string "Try moving more slowly.$"


EventScript_PokemonGotAway::
    msgbox sText_PokemonGotAway, MSGBOX_SIGN
    end
sText_PokemonGotAway:
    .string "The Pokémon got away!$"


EventScript_LostSignal::
    msgbox sText_LostSignal, MSGBOX_SIGN
    end
sText_LostSignal:
    .string "There is no reaction.\n"
    .string "The signal was lost!$"


@ Gunk's: clearer messages when R can't start a search, like Gunk's Platinum's
EventScript_DexNavNothingChosen::
    msgbox sText_DexNavNothingChosen, MSGBOX_SIGN
    end
sText_DexNavNothingChosen:
    .string "Pick a POKéMON in the DEXNAV, then\n"
    .string "press R in the grass or on water.$"

EventScript_DexNavNotFoundHere::
    msgbox sText_DexNavNotFoundHere, MSGBOX_SIGN
    end
sText_DexNavNotFoundHere:
    .string "That POKéMON can't be found here.$"

EventScript_DexNavSearchWhileSurfing::
    msgbox sText_DexNavSearchWhileSurfing, MSGBOX_SIGN
    end
sText_DexNavSearchWhileSurfing:
    .string "That POKéMON lives on the water.\n"
    .string "Search for it while surfing!$"

EventScript_DexNavSearchInGrass::
    msgbox sText_DexNavSearchInGrass, MSGBOX_SIGN
    end
sText_DexNavSearchInGrass:
    .string "That POKéMON lives on land.\n"
    .string "Search for it off the water!$"

EventScript_TooDark::
    msgbox sText_TooDark, MSGBOX_SIGN
    end
sText_TooDark:
    .string "It's too dark to search\nfor a Pokémon!$"
