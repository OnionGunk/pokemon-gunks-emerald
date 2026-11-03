EventScript_RepelWoreOff::
	msgbox Text_RepelWoreOff, MSGBOX_SIGN
	end

@ Gunk's: when a Repel wears off, offer another of the same kind if the bag has one.
@ VAR_GUNK_LAST_REPEL is the Repel the player used last (set in item_use.c).
EventScript_GunkRepelWoreOff::
	lockall
	checkitem VAR_GUNK_LAST_REPEL
	goto_if_eq VAR_RESULT, FALSE, EventScript_GunkRepelNoneLeft
	bufferitemname STR_VAR_2, VAR_GUNK_LAST_REPEL
	msgbox Text_GunkRepelUseAnother, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_GunkRepelEnd
	removeitem VAR_GUNK_LAST_REPEL
	special GunkRestartRepel
	playse SE_REPEL
	waitse
	msgbox gText_PlayerUsedVar2, MSGBOX_DEFAULT
	goto EventScript_GunkRepelEnd

EventScript_GunkRepelNoneLeft:
	msgbox Text_RepelWoreOff, MSGBOX_DEFAULT
EventScript_GunkRepelEnd:
	releaseall
	end

Text_RepelWoreOff:
	.string "REPEL's effect wore off…$"

Text_GunkRepelUseAnother:
	.string "REPEL's effect wore off…\n"
	.string "Use another {STR_VAR_2}?$"
