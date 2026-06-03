#ifndef __APPSTATE_H
#define __APPSTATE_H
#include <stdio.h>
#include "Control.h"

extern void asSet(appstate state, int num = 1);
extern void asClear(appstate state);
extern int asGet(appstate state);
extern void asClearDisplayDelay(int iDelayMs);

#endif

