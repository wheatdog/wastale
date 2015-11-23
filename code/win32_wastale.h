#ifndef WIN32_WASTALE_H
#define WIN32_WASTALE_H

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int BytePerPixel;
    u32 PitchInByte;
};

struct win32_screen_dimension
{
    int Width;
    int Height;
};

struct win32_xaudio
{
    IXAudio2 *XAudio;
    IXAudio2MasteringVoice *MasterVoice;
    IXAudio2SourceVoice *SourceVoice;
    WAVEFORMATEX OutputWaveFormat;

    u32 BufferSizeInSample;
    u32 BufferSizeInByte;
    void *SoundBuffer;

    u32 WriteCursor;

    u32 LatencyInSample;
};

#endif
