#ifndef WASTALE_H
#define WASTALE_H

#include "wastale_platform.h"
#include "wastale_math.h"
#include "wastale_intrinsic.h"

struct player_info
{
    b32 Exist;

    v2 ddP;
    v2 dP;
    v2 P;
};

struct game_state
{
    player_info PlayerInfo[5];

    u32 MeterPerPixel;

    r32 ToneHz;
    r32 Volume;
    r32 tSine;
};

#endif
