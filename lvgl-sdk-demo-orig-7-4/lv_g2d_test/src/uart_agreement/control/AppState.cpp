#include <stdio.h>
#include "Control.h"
#include "AppState.h"

static AppControl __GsAppCtrl;

void asSet (appstate state, int num)
{
    __GsAppCtrl.Set(state, num);
}

void asClear (appstate state)
{
    __GsAppCtrl.Clear(state);
}

int asGet (appstate state)
{
    return __GsAppCtrl.Get(state);
}

void asClearDisplayDelay (int iDelayMs)
{
    __GsAppCtrl.ClearDisplyDelay(iDelayMs);
}
