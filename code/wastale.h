#ifndef WASTALE_H
#define WASTALE_H

#include "wastale_platform.h"
#include "wastale_math.h"
#include "wastale_intrinsic.h"

enum entity_type
{
    EntityType_Null,
    EntityType_Player,
    EntityType_Wall,
};

struct entity
{
    b32 Exist;

    entity_type Type;

    v2 ddP;
    v2 dP;
    v2 P;

    v2 WidthHeight;

    b32 Collide;
};

struct game_state
{
    u32 ControllerToEntityIndex[5];

    u32 EntityCount;
    entity Entities[256];

    u32 MeterPerPixel;

    r32 ToneHz;
    r32 Volume;
    r32 tSine;
};

#endif
