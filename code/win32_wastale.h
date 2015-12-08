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

struct win32_playback
{
    void *PermanentMemory;
    u32 Max;
    u32 Count;
    game_input *Inputs;
};

struct win32_state
{
    char ExeFullPath[MAX_PATH];
    char *OnePastExeFullPathLastSlash;

    u32 PlayingIndex;
    u32 RecordingIndex;
    win32_playback Playbacks[5];
};

struct win32_game_code
{
    b32 IsValid;
    HMODULE DLL;
    FILETIME LastWriteTime;
    game_update_and_render *UpdateAndRender;
    game_fill_sound *FillSound;
};

#endif
