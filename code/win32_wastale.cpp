#include "wastale_platform.h"
#include "wastale.cpp"

#include <windows.h>
#include <Xaudio2.h>
#include <xinput.h>
#include <stdio.h>
#include "win32_wastale.h"

// NOTE(wheatdog): XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE(wheatdog): XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define X_AUDIO2_CREATE(name) HRESULT name(IXAudio2 **ppXAudio2, UINT32 Flags, XAUDIO2_PROCESSOR XAudio2Processor)
typedef X_AUDIO2_CREATE(x_audio2_create);

global_variable b32 GlobalRunning;
global_variable win32_offscreen_buffer GlobalScreenBuffer;
global_variable i64 GlobalPerfCountFreq;

internal void
Win32LoadXInput()
{
    HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibrary("xinput1_3.dll");
    }
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibrary("xinput9_1_0.dll");
    }

    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
    else
    {
        // TODO(wheatdog): Logging
    }
}

internal win32_screen_dimension
Win32GetScreenDimension(HWND Window)
{
    win32_screen_dimension Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    // NOTE(wheatdog): Make the origin be the upper-left corner.
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    Buffer->BytePerPixel = 4;
    SIZE_T BitmapMemorySize = Buffer->Width*Buffer->Height*Buffer->BytePerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->PitchInByte = Buffer->Width*Buffer->BytePerPixel;
}

internal void
Win32DisplayBufferToScreen(win32_offscreen_buffer *Buffer,HDC DeviceContext,
                           int DestWidth, int DestHeight)
{
    // TODO(wheatdog): Aspect ratio correction
    StretchDIBits(DeviceContext,
                  /*
                  X, Y, Width, Height, // NOTE(wheatdog): Dest
                  X, Y, Width, Height, // NOTE(wheatdog): Source
                  */
                  0, 0, DestWidth, DestHeight,
                  0, 0, Buffer->Width, Buffer->Height,
                  Buffer->Memory, &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK
Win32MainWindowCallback(HWND   Window,
                        UINT   Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_DESTROY:
        {
            // TODO(wheatdog): Handle this with a message to the player.
            GlobalRunning = false;
        } break;

        case WM_CLOSE:
        {
            // TODO(wheatdog): Recreate window.
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT PaintStruct;
            HDC DeviceContext = BeginPaint(Window, &PaintStruct);
            win32_screen_dimension ScreenDim = Win32GetScreenDimension(Window);
            Win32DisplayBufferToScreen(&GlobalScreenBuffer, DeviceContext,
                                       ScreenDim.Width, ScreenDim.Height);
            EndPaint(Window, &PaintStruct);
        } break;

        case WM_KEYUP:
        case WM_KEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
        {
            WPARAM VKCode = WParam;
            bool WasDown = ((LParam & (1 << 30)) != 0);
            bool IsDown = ((LParam & (1 << 31)) == 0);

            if (IsDown != WasDown)
            {
                if (VKCode == 'W')
                {
                    OutputDebugStringA("W:");
                    if (IsDown)
                        OutputDebugStringA("IsDown ");
                    if (WasDown)
                        OutputDebugStringA("WasDown ");
                    OutputDebugStringA("\n");
                }
                else if (VKCode == 'A')
                {
                }
                else if (VKCode == 'S')
                {
                }
                else if (VKCode == 'D')
                {
                }
                else if (VKCode == 'Q')
                {
                }
                else if (VKCode == 'E')
                {
                }
                else if (VKCode == VK_UP)
                {
                }
                else if (VKCode == VK_DOWN)
                {
                }
                else if (VKCode == VK_LEFT)
                {
                }
                else if (VKCode == VK_RIGHT)
                {
                }
                else if (VKCode == VK_ESCAPE)
                {
                }
                else if (VKCode == VK_SPACE)
                {
                }
            }

            b32 AltKeyIsDown = (LParam & (1 << 29));
            if (IsDown)
            {
                if (VKCode == VK_F4)
                {
                    if (AltKeyIsDown)
                    {
                        GlobalRunning = false;
                    }
                }
            }

        } break;

        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

internal win32_xaudio
Win32InitXAudio2(u32 SamplePerSecond, u32 BufferSizeInSample,
                 u32 ChannelCount, u32 BytePerSample, u32 LatencyInSample)
{
    // NOTE(wheatdog): Windows 10
    HMODULE XAudio2Library = LoadLibrary("xaudio2_9.dll");
    if (!XAudio2Library)
    {
        // NOTE(wheatdog): Windows 8
        XAudio2Library = LoadLibrary("xaudio2_8.dll");
    }
    if (!XAudio2Library)
    {
        // NOTE(wheatdog): Windows 7
        XAudio2Library = LoadLibrary("xaudio2_7.dll");
    }
    // TODO(wheatdog): Windows XP?

    win32_xaudio Result = {};
    if (!XAudio2Library)
    {
        // TODO(wheatdog): Logging
        return Result;
    }

    x_audio2_create *XAudio2Create = (x_audio2_create *)GetProcAddress(XAudio2Library, "XAudio2Create");
    if (FAILED(XAudio2Create(&Result.XAudio, 0, XAUDIO2_DEFAULT_PROCESSOR)))
    {
        // TODO(wheatdog): Logging
        Result = {};
        return Result;
    }

    // TODO(wheatdog): If I don't call CoInitialize, it will fail. I really
    // don't know why, figure it out later. And maybe find a better place to call
    // CoInitialize.
    if (FAILED(CoInitializeEx(0, COINIT_APARTMENTTHREADED)))
    {
        // TODO(wheatdog): Logging
        Result = {};
        return Result;
    }

    if (FAILED(Result.XAudio->CreateMasteringVoice(&Result.MasterVoice)))
    {
        // TODO(wheatdog): Logging
        Result = {};
        return Result;
    }

    Result.OutputWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    Result.OutputWaveFormat.nChannels = ChannelCount;
    Result.OutputWaveFormat.nSamplesPerSec = SamplePerSecond;
    Result.OutputWaveFormat.wBitsPerSample = BytePerSample*8;
    Result.OutputWaveFormat.nBlockAlign = (Result.OutputWaveFormat.nChannels*Result.OutputWaveFormat.wBitsPerSample)/8;
    Result.OutputWaveFormat.nAvgBytesPerSec = Result.OutputWaveFormat.nSamplesPerSec*Result.OutputWaveFormat.nBlockAlign;

    OutputDebugStringA("Initialize XAudio2 MasterVoice successfully\n");

    Result.LatencyInSample = LatencyInSample;
    Result.BufferSizeInSample = BufferSizeInSample;
    Result.BufferSizeInByte = BufferSizeInSample*ChannelCount*BytePerSample;
    Result.SoundBuffer = VirtualAlloc(0, Result.BufferSizeInByte, MEM_RESERVE|MEM_COMMIT,
                                      PAGE_READWRITE);

    if(FAILED(Result.XAudio->CreateSourceVoice(&Result.SourceVoice, &Result.OutputWaveFormat)))
    {
        // TODO(wheatdog): Logging
        Result = {};
        return Result;
    }
    OutputDebugStringA("Create XAudio2 SourceVoice successfully\n");

#if 1
    if (FAILED(Result.SourceVoice->Start(0,0)))
    {
        // TODO(wheatdog): Logging
        Result = {};
        return Result;
    }
    OutputDebugStringA("Start SourceVoice successfully\n");
#endif

    return Result;
}

internal LARGE_INTEGER
Win32GetPerfCount()
{
    LARGE_INTEGER CurrentPerfCount;
    if (QueryPerformanceCounter(&CurrentPerfCount) == 0)
    {
        // TODO(wheatdog): Logging
    }

    return CurrentPerfCount;
}

internal r32
Win32GetSecondsElapse(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    r32 Result = ((r32)(End.QuadPart - Start.QuadPart)/ (r32)GlobalPerfCountFreq);
    return Result;
}

internal void
Win32FillSoundBuffer(game_sound *GameSound, win32_xaudio *Win32XAudio)
{
    u32 SampleToWrite[2];
    XAUDIO2_BUFFER XAudioBuffer[2] = {};
    u32 ByteToWrite = GameSound->SampleCount*GameSound->ChannelCount*GameSound->BytePerSample;

    SampleToWrite[0] = GameSound->SampleCount;
    XAudioBuffer[0].AudioBytes = ByteToWrite;
    XAudioBuffer[0].pAudioData = (BYTE *)((u8 *)Win32XAudio->SoundBuffer + Win32XAudio->WriteCursor*GameSound->ChannelCount*GameSound->BytePerSample);
    if (Win32XAudio->WriteCursor + GameSound->SampleCount > Win32XAudio->BufferSizeInSample)
    {
        SampleToWrite[0] = (Win32XAudio->BufferSizeInSample - Win32XAudio->WriteCursor);
        XAudioBuffer[0].AudioBytes = SampleToWrite[0]*GameSound->ChannelCount*GameSound->BytePerSample;
    }
    SampleToWrite[1] = GameSound->SampleCount - SampleToWrite[0];
    XAudioBuffer[1].AudioBytes = ByteToWrite - XAudioBuffer[0].AudioBytes;
    XAudioBuffer[1].pAudioData = (BYTE *)Win32XAudio->SoundBuffer;

    Win32XAudio->WriteCursor = ((Win32XAudio->WriteCursor + GameSound->SampleCount) %
                                Win32XAudio->BufferSizeInSample);

    i16* SourceSample = (i16 *)GameSound->Buffer;
    for (i32 BufferIndex = 0; BufferIndex < 2; ++BufferIndex)
    {
        if (XAudioBuffer[BufferIndex].AudioBytes == 0)
        {
            continue;
        }

        i16* DestSample = (i16 *)XAudioBuffer[BufferIndex].pAudioData;
        for(i32 SampleIndex = 0;
            SampleIndex < SampleToWrite[BufferIndex];
            ++SampleIndex)
        {
            for (i32 ChannelIndex = 0;
                 ChannelIndex < GameSound->ChannelCount;
                 ++ChannelIndex)
            {
                *DestSample++ = *SourceSample++;
            }
        }

        Win32XAudio->SourceVoice->SubmitSourceBuffer(&XAudioBuffer[BufferIndex]);
    }
}

int CALLBACK
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR Commandline, int ShowCode)
{
    LARGE_INTEGER PerfCountFreq;
    if (QueryPerformanceFrequency(&PerfCountFreq) == 0)
    {
        // TODO(wheatdog): Logging
    }
    GlobalPerfCountFreq = PerfCountFreq.QuadPart;

    Win32ResizeDIBSection(&GlobalScreenBuffer, 1280, 720);
    Win32LoadXInput();

    // NOTE(wheatdog): Set Windows Scheduler granularity to 1ms
    // so that our sleep() can be more granular.
    UINT DesiredSchedulerMS = 1;
    b32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_OWNDC|CS_VREDRAW|CS_HREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "WastaleWindowClass";
    if (!RegisterClass(&WindowClass))
    {
        // TODO(wheatdog): Logging
        return -1;
    }

    HWND Window = CreateWindowEx(0, WindowClass.lpszClassName,
                                 "Wastale", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 0, 0, Instance, 0);

    if (!Window)
    {
        // TODO(wheatdog): Logging
        return -1;
    }

    // TODO(wheatdog): Handle various memory footprints
    game_memory GameMemory = {};
    GameMemory.PermanentStorageSize = MegaBytes(512);
    GameMemory.TransientStorageSize = GigaBytes(1);
    u32 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
    GameMemory.PermanentStorage = VirtualAlloc(0, TotalSize, MEM_RESERVE|MEM_COMMIT,
                                               PAGE_READWRITE);
    GameMemory.TransientStorage = ((u8 *)GameMemory.PermanentStorage +
                                   GameMemory.PermanentStorageSize);

#define GAME_UPDATE_HZ 60
#define FRAME_OF_AUDIO_LANTENCY 2.5f
    r32 TargetSecondElapsed = 1.0f / GAME_UPDATE_HZ;
    u32 SamplePerSecond = 48000;
    u32 ChannelCount = 2;
    u32 BytePerSample = sizeof(i16);
    u32 LatencyInSample = FRAME_OF_AUDIO_LANTENCY*(SamplePerSecond / GAME_UPDATE_HZ);
    win32_xaudio Win32XAudio = Win32InitXAudio2(SamplePerSecond, SamplePerSecond,
                                                ChannelCount, BytePerSample, LatencyInSample);

    game_sound GameSound = {};
    GameSound.SamplePerSecond = SamplePerSecond;
    GameSound.Buffer = VirtualAlloc(0, Win32XAudio.BufferSizeInByte,
                                     MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    GameSound.ChannelCount = ChannelCount;
    GameSound.BytePerSample = BytePerSample;
    GameSound.ToneHz = 256;
    GameSound.Volume = 10000;

    b32 AudioIsPlaying = false;
    GlobalRunning = true;
    i64 LastCycleCount = __rdtsc();
    LARGE_INTEGER LastCounter = Win32GetPerfCount();
    while(GlobalRunning)
    {
        MSG Message;
        while (PeekMessage(&Message, Window,  0, 0, PM_REMOVE))
        {
            if (Message.message == WM_QUIT)
            {
                GlobalRunning = false;
            }

            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }

        // TODO(wheatdog): Poll this more frequently.
        for (DWORD ControllerIndex = 0;
             ControllerIndex < XUSER_MAX_COUNT;
             ++ControllerIndex)
        {
            XINPUT_STATE ControllerState;
            if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
            {
                // NOTE(wheatdog): Controller is connected.

                XINPUT_GAMEPAD *GamePad = &ControllerState.Gamepad;

                b32 Up = (GamePad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                b32 Down = (GamePad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                b32 Left = (GamePad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                b32 Right = (GamePad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                b32 Start = (GamePad->wButtons & XINPUT_GAMEPAD_START);
                b32 Back = (GamePad->wButtons & XINPUT_GAMEPAD_BACK);
                b32 AButton = (GamePad->wButtons & XINPUT_GAMEPAD_A);
                b32 BButton = (GamePad->wButtons & XINPUT_GAMEPAD_B);
                b32 XButton = (GamePad->wButtons & XINPUT_GAMEPAD_X);
                b32 YButton = (GamePad->wButtons & XINPUT_GAMEPAD_Y);
                b32 LeftSoulder = (GamePad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                b32 RightSoulder = (GamePad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

                i16 LeftStickX = GamePad->sThumbLX;
                i16 LeftStickY = GamePad->sThumbLY;

                i16 RightStickX = GamePad->sThumbRX;
                i16 RightStickY = GamePad->sThumbRY;

                GameSound.ToneHz = 512 + 256.0f*((r32)LeftStickY / 32768.0f);
                GameSound.Volume = 3000 + 3000.0f*((r32)RightStickY / 32768.0f);

            }
            else
            {
                // NOTE(wheatdog): NOT connected.
            }

            // XINPUT_VIBRATION Vibration;
            // Vibration.wLeftMotorSpeed = ;
            // Vibration.wRightMotorSpeed = ;
            // XInputSetState(ControllerIndex, &Vibration);
        }

        game_offscreen_buffer GameScreenBuffer;
        GameScreenBuffer.Memory = GlobalScreenBuffer.Memory;
        GameScreenBuffer.Width = GlobalScreenBuffer.Width;
        GameScreenBuffer.Height = GlobalScreenBuffer.Height;
        GameScreenBuffer.BytePerPixel = GlobalScreenBuffer.BytePerPixel;
        GameScreenBuffer.PitchInByte = GlobalScreenBuffer.PitchInByte;

        XAUDIO2_VOICE_STATE VoiceState;
        Win32XAudio.SourceVoice->GetState(&VoiceState);
        u32 PlayCursor = (u32)(VoiceState.SamplesPlayed % Win32XAudio.BufferSizeInSample);
        u32 RemainSample = (Win32XAudio.WriteCursor - PlayCursor);
        if (Win32XAudio.WriteCursor < PlayCursor)
        {
            RemainSample += Win32XAudio.BufferSizeInSample;
        }
        GameSound.SampleCount = Win32XAudio.LatencyInSample - RemainSample;

        char ShowBuffer[128];
        snprintf(ShowBuffer, sizeof(ShowBuffer), "PC: %d, WC: %d, RemainSample: %d\n",
                 PlayCursor, Win32XAudio.WriteCursor, RemainSample);
        OutputDebugStringA(ShowBuffer);

        LARGE_INTEGER BeforeCounter = Win32GetPerfCount();

        GameUpdateAndRender(&GameMemory, &GameScreenBuffer, &GameSound);

        Win32XAudio.SourceVoice->GetState(&VoiceState);
        PlayCursor = (u32)(VoiceState.SamplesPlayed % Win32XAudio.BufferSizeInSample);
        RemainSample = (Win32XAudio.WriteCursor - PlayCursor);
        if (Win32XAudio.WriteCursor < PlayCursor)
        {
            RemainSample += Win32XAudio.BufferSizeInSample;
        }
        snprintf(ShowBuffer, sizeof(ShowBuffer), "PC: %d, WC: %d, RemainSample: %d\n",
                 PlayCursor, Win32XAudio.WriteCursor, RemainSample);
        OutputDebugStringA(ShowBuffer);

        Win32FillSoundBuffer(&GameSound, &Win32XAudio);

        LARGE_INTEGER WorkCounter = Win32GetPerfCount();
        r32 SecondElapseInFrame = Win32GetSecondsElapse(LastCounter, WorkCounter);
        r32 Duration = Win32GetSecondsElapse(BeforeCounter, WorkCounter);
        r32 WorkingSecond = SecondElapseInFrame;

        if (SecondElapseInFrame < TargetSecondElapsed)
        {
            if (SleepIsGranular)
            {
                DWORD SleepMS = (DWORD)(1000.0f*(TargetSecondElapsed - SecondElapseInFrame));
                if (SleepMS > 0)
                {
                    Sleep(SleepMS);
                }
            }

            r32 TestSecondElapseForFrame = Win32GetSecondsElapse(LastCounter, Win32GetPerfCount());
            if (TestSecondElapseForFrame > TargetSecondElapsed)
            {
                // TODO(wheatdog): MISS SLEEP, Logging
            }

            while (SecondElapseInFrame < TargetSecondElapsed)
            {
                SecondElapseInFrame = Win32GetSecondsElapse(LastCounter,
                                                            Win32GetPerfCount());
            }
        }
        else
        {
            // TODO(wheatdog): MISS FRAME RATE!
            // TODO(wheatdog): Logging
        }

        i64 EndCycleCount = __rdtsc();
        r32 MCPerFrame = (EndCycleCount - LastCycleCount) / (1000.0f*1000.0f);
        LastCycleCount = EndCycleCount;

        LARGE_INTEGER EndCounter = Win32GetPerfCount();
        r32 SecondsElapse = Win32GetSecondsElapse(LastCounter, EndCounter);
        r32 MSElapse = 1000.0f*SecondsElapse;
        r32 FPS = 1.0f / SecondsElapse;
        LastCounter = EndCounter;

        char Buffer[128];
        snprintf(Buffer, sizeof(Buffer), "D:%0.2fms/f, W:%0.2fms/f, %0.2fms/f, %0.2f/s, %0.2fMc/f\n",
                 Duration*1000, WorkingSecond*1000, MSElapse, FPS, MCPerFrame);
        OutputDebugStringA(Buffer);

        if (!AudioIsPlaying)
        {
#if 0
            if (FAILED(Win32XAudio.SourceVoice->Start(0,0)))
            {
                // TODO(wheatdog): Logging
                return -1;
            }
            OutputDebugStringA("Start SourceVoice successfully\n");
#endif
            AudioIsPlaying = true;
        }

        HDC DeviceContext = GetDC(Window);
        win32_screen_dimension ScreenDim = Win32GetScreenDimension(Window);
        Win32DisplayBufferToScreen(&GlobalScreenBuffer, DeviceContext,
                                   ScreenDim.Width, ScreenDim.Height);
        ReleaseDC(Window, DeviceContext);

    }

    return 0;
}
