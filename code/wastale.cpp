#include <math.h>

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
FillSoundOutput(game_sound *GameSound)
{
    u32 WavePeriod = GameSound->SamplePerSecond / GameSound->ToneHz;
    i16 *SampleOut = (i16 *)GameSound->Buffer;
    for (int SampleIndex = 0;
         SampleIndex < GameSound->SampleCount;
         ++SampleIndex)
    {
        i16 SampleValue = (i16)(GameSound->Volume * sinf(GameSound->tSine));

        for (int ChannelIndex = 0;
             ChannelIndex < GameSound->ChannelCount;
             ++ChannelIndex)
        {
            *SampleOut++ = SampleValue;
        }

        GameSound->tSine += 2.0f*Pi32*(1.0f / (r32)WavePeriod);
        if (GameSound->tSine > 2.0f*Pi32)
        {
            GameSound->tSine -= 2.0f*Pi32;
        }
    }
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    RenderWeirdGradient(Buffer, 0, 0);
    FillSoundOutput(GameSound);
}
