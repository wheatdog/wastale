#ifndef WASTALE_PLATFORM_H
#define WASTALE_PLATFORM_H

/*
  NOTE(wheatdog):

  - WASTALE_INTERNAL
  - WASTALE_SLOW
*/

#define internal static
#define global_variable static
#define local_persist static

#define Pi32 3.14159265359f

#define KiloBytes(Value) (Value * 1024LL)
#define MegaBytes(Value) (KiloBytes(Value) * 1024LL)
#define GigaBytes(Value) (MegaBytes(Value) * 1024LL)
#define TeraBytes(Value) (GigaBytes(Value) * 1024LL)


#ifdef WASTALE_INTERNAL
#define Assert(Expression) if (!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

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

//
// NOTE(wheatdog): Utility
//

inline u32
SafeTruncateUInt64(u64 Value)
{
    Assert(Value <= 0xFFFFFFFF);
    u32 Result = (u32)Value;
    return Result;
}

//
// NOTE(wheatdog): Services that the platform provides for the game.
//

struct thread_context
{
    int Placeholder;
};

struct debug_read_file_result
{
    u32 Size;
    void *Content;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void* Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, char* Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(thread_context *Thread, char* Filename, u32 FileSize, void* Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

//
// NOTE(wheatdog): Services that the game provides for the platform.
//

struct game_memory
{
    b32 IsInitial;
    u64 PermanentStorageSize;
    void *PermanentStorage; // NOTE(wheatdog): REQUIRE to be cleared to zero at startup

    u64 TransientStorageSize;
    void *TransientStorage; // NOTE(wheatdog): REQUIRE to be cleared to zero at startup

    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
};

struct game_offscreen_buffer
{
    void *Memory;
    i32 Width;
    i32 Height;
    u32 BytePerPixel;
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

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *GameMemory, game_offscreen_buffer *Buffer, game_input *GameInput)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_FILL_SOUND(name) void name(thread_context *Thread, game_memory *GameMemory, game_sound *GameSound)
typedef GAME_FILL_SOUND(game_fill_sound);

#endif
