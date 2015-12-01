#ifndef WASTALE_PLATFORM_H
#define WASTALE_PLATFORM_H

#define internal static
#define global_variable static
#define local_persist static

#define Pi32 3.14159265359f

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

struct game_button_state
{
    u32 HalfTransitionCount;
    b32 EndedDown;
};

struct game_controller_stick
{
    r32 X;
    r32 Y;
};

struct game_controller_input
{
    b32 IsConnected;
    b32 IsAnalog;
    game_controller_stick LeftStick;
    game_controller_stick RightStick;

    union
    {
        game_button_state Buttons[12];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state YButton;
            game_button_state XButton;
            game_button_state AButton;
            game_button_state BButton;

            game_button_state LeftSoulder;
            game_button_state RightSoulder;

            game_button_state Back;
            game_button_state Start;

            // NOTE(wheatdog): All button must be added before this line

            game_button_state Terminator;
        };
    };
};

struct game_input
{
    r32 dtForFrame;
    game_controller_input Controllers[5];
};

struct game_sound
{
    u32 SamplePerSecond;
    u32 SampleCount;
    u32 ChannelCount;
    u32 BytePerSample;
    void *Buffer;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *GameMemory, game_offscreen_buffer *Buffer, game_sound *GameSound, game_input *GameInput)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

/*
  NOTE(wheatdog): Services that the platform provides for the game.
*/


#endif
