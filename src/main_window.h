#pragma once

#include "globals.h"
#include "config.h"

#if defined(ANTIALIASING) && defined(PBL_COLOR)
#include "include/antialiasing.h"
#endif

void main_window_push();
