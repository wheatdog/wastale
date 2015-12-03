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
            u8 G = 255;
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

internal void
DrawRect(game_offscreen_buffer *Buffer,
         r32 Real32X, r32 Real32Y, r32 Real32Width, r32 Real32Height,
         r32 Real32R, r32 Real32G, r32 Real32B)
{
    i32 X = RoundReal32ToInt32(Real32X);
    i32 Y = RoundReal32ToInt32(Real32Y);
    i32 Width = RoundReal32ToInt32(Real32Width);
    i32 Height = RoundReal32ToInt32(Real32Height);

    i32 MinX = (X < 0)? 0 : X;
    i32 MaxX = X + Width;
    if (MaxX < 0) MaxX = 0;
    if (MaxX > Buffer->Width) MaxX = Buffer->Width;

    i32 MinY = (Y < 0)? 0 : Y;
    i32 MaxY = Y + Height;
    if (MaxY < 0) MaxY = 0;
    if (MaxY > Buffer->Height) MaxY = Buffer->Height;

    u8 R = (u8)(Real32R*255.0f);
    u8 G = (u8)(Real32G*255.0f);
    u8 B = (u8)(Real32B*255.0f);
    u32 Color = B|(G << 8)|(R << 16);

    u8 *Start = (u8 *)Buffer->Memory + MinY*Buffer->PitchInByte + MinX*Buffer->BytePerPixel;
    for (i32 YIndex = MinY; YIndex < MaxY; ++YIndex)
    {
        u32 *Pixel = (u32 *)Start;
        for (i32 XIndex = MinX; XIndex < MaxX; ++XIndex)
        {
            *Pixel++ = Color;
        }
        Start += Buffer->PitchInByte;
    }
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    game_state *GameState = (game_state *)GameMemory->PermanentStorage;
    Assert(sizeof(game_state) <= GameMemory->PermanentStorageSize);
    if (!GameMemory->IsInitial)
    {
        GameState->MeterPerPixel = 30;

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

        player_info *PlayerInfo = GameState->PlayerInfo + ControllerIndex;

        if (!PlayerInfo->Exist)
        {
            if (!Controller->Start.EndedDown)
            {
                continue;
            }
            PlayerInfo->Exist = true;
        }

        r32 PlayerAccelerate = 100.0f; // m/s^2
        r32 Drag = 5.0f;
        PlayerInfo->ddP = V2(Controller->LeftStick.X, Controller->LeftStick.Y)*PlayerAccelerate - PlayerInfo->dP*Drag;
        PlayerInfo->dP += PlayerInfo->ddP*GameInput->dtForFrame;
        PlayerInfo->P += PlayerInfo->dP*GameInput->dtForFrame + 0.5*PlayerInfo->ddP*Square(GameInput->dtForFrame);

    }

    // NOTE(wheatdog): Clear screen to black
    DrawRect(Buffer, 0, 0, (r32)Buffer->Width, (r32)Buffer->Height, 0.0f, 0.0f, 0.0f);

    r32 PlatformX = 5.0f;
    r32 PlatformY = 5.0f;
    r32 PlatformWidth = 10.0f;
    r32 PlatformHeight = 1.5f;
    DrawRect(Buffer,
             (PlatformX*GameState->MeterPerPixel),
             (PlatformY*GameState->MeterPerPixel),
             (PlatformWidth*GameState->MeterPerPixel),
             (PlatformHeight*GameState->MeterPerPixel),
             1.0f, 1.0f, 0.0f);

    r32 GroundX = 0.0f;
    r32 GroundY = 0.0f;
    r32 GroundWidth = 30.0f;
    r32 GroundHeight = 1.5f;
    DrawRect(Buffer,
             (GroundX*GameState->MeterPerPixel),
             (GroundY*GameState->MeterPerPixel),
             (GroundWidth*GameState->MeterPerPixel),
             (GroundHeight*GameState->MeterPerPixel),
             1.0f, 1.0f, 0.0f);

    for (u32 ControllerIndex = 0;
         ControllerIndex < ArrayCount(GameInput->Controllers);
         ++ControllerIndex)
    {
        player_info *PlayerInfo = GameState->PlayerInfo + ControllerIndex;

        if (!PlayerInfo->Exist)
        {
            continue;
        }

        r32 PlayerWidth = 1.0f;
        r32 PlayerHeight = 1.5f;
        DrawRect(Buffer,
                 (PlayerInfo->P.X*GameState->MeterPerPixel),
                 (PlayerInfo->P.Y*GameState->MeterPerPixel),
                 (PlayerWidth*GameState->MeterPerPixel),
                 (PlayerHeight*GameState->MeterPerPixel),
                 1.0f, 0.0f, 0.0f);
    }
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
