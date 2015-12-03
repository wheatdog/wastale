#ifndef WASTALE_INTRINSIC_H
#define WASTALE_INTRINSIC_H

inline i32
RoundReal32ToInt32(r32 Real32)
{
    i32 Result = (i32)roundf(Real32);
    return Result;
}

inline u32
RoundReal32ToUIint32(r32 Real32)
{
    u32 Result = (u32)roundf(Real32);
    return Result;
}

inline i32
CeilReal32ToInt32(r32 Real32)
{
    i32 Result = (i32)ceilf(Real32);
    return Result;
}

inline i32
FloorReal32ToInt32(r32 Real32)
{
    i32 Result = (i32)floorf(Real32);
    return Result;
}

#endif
