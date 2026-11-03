#ifndef GUARD_GUNK_QUICK_BALL_H
#define GUARD_GUNK_QUICK_BALL_H

// Gunk's Quick Run (R) and Quick Ball (L) on the battle action menu; see src/gunk_quick_ball.c

enum {
    GUNK_QUICK_BALL_INPUT_NONE,  // not ours: the action menu handles the input
    GUNK_QUICK_BALL_INPUT_USED,  // ours, nothing for the controller to do
    GUNK_QUICK_BALL_INPUT_RUN,   // R: the controller chooses RUN
    GUNK_QUICK_BALL_INPUT_THROW, // L: the controller chooses BAG; Gunk_QuickBall_TakePending gives the ball
};

void Gunk_QuickBall_Show(void);
void Gunk_QuickBall_Hide(void);
u8 Gunk_QuickBall_HandleInput(void);
bool32 Gunk_QuickBall_TakePending(u16 *ball);
void Gunk_QuickBall_NoteItemUsed(u16 item);

#endif // GUARD_GUNK_QUICK_BALL_H
