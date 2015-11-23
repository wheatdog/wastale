#ifndef WASTALE_PLATFORM_H
#define WASTALE_PLATFORM_H

#define internal static
#define global_variable static
#define local_persist static

#define Pi32 3.14159265359

#define KiloBytes(Value) (Value * 1024LL)
#define MegaBytes(Value) (KiloBytes(Value) * 1024LL)
#define GigaBytes(Value) (MegaBytes(Value) * 1024LL)
#define TeraBytes(Value) (GigaBytes(Value) * 1024LL)

#include <assert.h>

#define Assert(Expression) assert(Expression)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef i32 b32;

typedef float r32;
typedef double r64;


/*
  NOTE(wheatdog): Services that the game provides for the platform.
*/

struct game_memory
{
    b32 IsInitial;
    u64 PermanentStorageSize;
    void *PermanentStorage; // NOTE(wheatdog): REQUIRE to be cleared to zero at startup

    u64 TransientStorageSize;
    void *TransientStorage; // NOTE(wheatdog): REQUIRE to be cleared to zero at startup
};

struct game_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int BytePerPixel;
    u32 PitchInByte;
};

struct game_sound
{
    u32 SamplePerSecond;
    u32 SampleCount;
    void *Buffer;

    u32 ChannelCount;
    u32 BytePerSample;

    r32 ToneHz;
    r32 Volume;
    r32 tSine;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *GameMemory, game_offscreen_buffer *Buffer, game_sound *GameSound)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

/*
  NOTE(wheatdog): Services that the platform provides for the game.
*/


#endif
