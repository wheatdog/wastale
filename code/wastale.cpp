#include <math.h>
#include "wastale.h"

internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, i32 XOffset, i32 YOffset)
{
    u8 *Row = (u8 *)Buffer->Memory;
    for (i32 Y = 0; Y < Buffer->Height; ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        // u8 *Pixel = Row;
        for (i32 X = 0; X < Buffer->Width; ++X)
        {
            // 0xAARRGGBB
            u8 R = (u8)(X + XOffset);
            u8 G = 0;
            u8 B = (u8)(Y + YOffset);
            *Pixel++ = B|(G << 8)|(R << 16);
        }
        Row += Buffer->PitchInByte;
    }
}

internal void
FillSoundOutput(game_sound *GameSound, game_state *GameState)
{
    u32 WavePeriod = (u32)(GameSound->SamplePerSecond / GameState->ToneHz);
    i16 *SampleOut = (i16 *)GameSound->Buffer;
    for (u32 SampleIndex = 0;
         SampleIndex < GameSound->SampleCount;
         ++SampleIndex)
    {
        i16 SampleValue = (i16)(GameState->Volume * sinf(GameState->tSine));

        for (u32 ChannelIndex = 0;
             ChannelIndex < GameSound->ChannelCount;
             ++ChannelIndex)
        {
            *SampleOut++ = SampleValue;
        }

        GameState->tSine += 2.0f*Pi32*(1.0f / (r32)WavePeriod);
        if (GameState->tSine > 2.0f*Pi32)
        {
            GameState->tSine -= 2.0f*Pi32;
        }
    }
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    game_state *GameState = (game_state *)GameMemory->PermanentStorage;
    Assert(sizeof(game_state) <= GameMemory->PermanentStorageSize);
    if (!GameMemory->IsInitial)
    {
        GameMemory->IsInitial = true;
    }

    for (u32 ControllerIndex = 0;
         ControllerIndex < ArrayCount(GameInput->Controllers);
         ++ControllerIndex)
    {
        game_controller_input *Controller = GameInput->Controllers + ControllerIndex;

        if (!Controller->IsConnected)
        {
            continue;
        }

        GameState->X += (u32)(Controller->LeftStick.X*10.0f);
        GameState->Y += (u32)(Controller->LeftStick.Y*10.0f);
    }

    RenderWeirdGradient(Buffer, GameState->X, GameState->Y);
}

//NOTE(wheatdog): This must be fast. ~1ms
GAME_FILL_SOUND(GameFillSound)
{
    game_state *GameState = (game_state *)GameMemory->PermanentStorage;
    if (!GameMemory->IsInitial)
    {
        GameState->ToneHz = 512;
        GameState->Volume = 0;
    }

    FillSoundOutput(GameSound, GameState);
}
