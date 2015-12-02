#include "wastale_platform.h"

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
global_variable WINDOWPLACEMENT GlobalWindowPlacement;

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    if (Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result Result = {};

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0,
                                    OPEN_EXISTING, 0, 0);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        // TODO(wheatdog): Logging
        Assert(!"Error");
    }

    LARGE_INTEGER FileSize;
    if (!GetFileSizeEx(FileHandle, &FileSize))
    {
        // TODO(wheatdog): Logging
        Assert(!"Error");
    }

    // TODO(wheatdog): Define for maximum values.
    u32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
    Result.Content = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (!Result.Content)
    {
        // TODO(wheatdog): Logging
        Assert(!"Error");
    }

    DWORD ByteRead;
    if (ReadFile(FileHandle, Result.Content, FileSize32, &ByteRead, 0) &&
        (ByteRead == FileSize32))
    {
        // NOTE(wheatdog): Read file successfully
        Result.Size = ByteRead;
    }
    else
    {
        DEBUGPlatformFreeFileMemory(Thread, Result.Content);
        Result.Content = 0;
        // TODO(wheatdog): Logging
    }

    return Result;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    b32 Result = false;

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        // TODO(wheatdog): Logging
        Assert(!"Error");
    }

    DWORD ByteWritten;
    if (!WriteFile(FileHandle, Memory, FileSize, &ByteWritten, 0))
    {
        // TODO(wheatdog): Loggin
    }
    Result = (ByteWritten == FileSize);

    CloseHandle(FileHandle);

    return Result;
}

internal void
Win32UnloadGameCode(win32_game_code *Game)
{
    if (Game->DLL)
    {
        FreeLibrary(Game->DLL);
        Game->DLL = 0;
    }

    Game->IsValid = 0;
    Game->UpdateAndRender = 0;
    Game->FillSound = 0;
}

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

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            Assert(!"Keyboard input came in through a non-dispatch message!");
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
    Result.OutputWaveFormat.nChannels = (WORD)ChannelCount;
    Result.OutputWaveFormat.nSamplesPerSec = SamplePerSecond;
    Result.OutputWaveFormat.wBitsPerSample = (WORD)BytePerSample*8;
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
Win32FillSoundBuffer(game_sound *GameSound, win32_xaudio *Win32Audio)
{
    u32 SampleToWrite[2];
    XAUDIO2_BUFFER XAudioBuffer[2] = {};
    u32 ByteToWrite = GameSound->SampleCount*GameSound->ChannelCount*GameSound->BytePerSample;

    SampleToWrite[0] = GameSound->SampleCount;
    XAudioBuffer[0].AudioBytes = ByteToWrite;
    XAudioBuffer[0].pAudioData = (BYTE *)((u8 *)Win32Audio->SoundBuffer + Win32Audio->WriteCursor*GameSound->ChannelCount*GameSound->BytePerSample);
    if (Win32Audio->WriteCursor + GameSound->SampleCount > Win32Audio->BufferSizeInSample)
    {
        SampleToWrite[0] = (Win32Audio->BufferSizeInSample - Win32Audio->WriteCursor);
        XAudioBuffer[0].AudioBytes = SampleToWrite[0]*GameSound->ChannelCount*GameSound->BytePerSample;
    }
    SampleToWrite[1] = GameSound->SampleCount - SampleToWrite[0];
    XAudioBuffer[1].AudioBytes = ByteToWrite - XAudioBuffer[0].AudioBytes;
    XAudioBuffer[1].pAudioData = (BYTE *)Win32Audio->SoundBuffer;

    Win32Audio->WriteCursor = ((Win32Audio->WriteCursor + GameSound->SampleCount) %
                                Win32Audio->BufferSizeInSample);

    i16* SourceSample = (i16 *)GameSound->Buffer;
    for (i32 BufferIndex = 0; BufferIndex < 2; ++BufferIndex)
    {
        if (XAudioBuffer[BufferIndex].AudioBytes == 0)
        {
            continue;
        }

        i16* DestSample = (i16 *)XAudioBuffer[BufferIndex].pAudioData;
        for(u32 SampleIndex = 0;
            SampleIndex < SampleToWrite[BufferIndex];
            ++SampleIndex)
        {
            for (u32 ChannelIndex = 0;
                 ChannelIndex < GameSound->ChannelCount;
                 ++ChannelIndex)
            {
                *DestSample++ = *SourceSample++;
            }
        }

        Win32Audio->SourceVoice->SubmitSourceBuffer(&XAudioBuffer[BufferIndex]);
    }
}

internal r32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZone)
{
    r32 Result = 0;

    if (Value < -DeadZone)
    {
        Result = (r32)(Value + DeadZone) / (32768.0f - DeadZone);
    }
    else if (Value > DeadZone)
    {
        Result = (r32)(Value - DeadZone) / (32767.0f - DeadZone);
    }

    return Result;
}

internal void
Win32ProcessKeyboardButton(b32 IsDown, game_button_state *NewButtonState)
{
    if (IsDown != NewButtonState->EndedDown)
    {
        NewButtonState->EndedDown = IsDown;
        ++NewButtonState->HalfTransitionCount;
    }
}

internal void
Win32ProcessXInputDigitalButton(game_button_state *NewButton, game_button_state *OldButton,
                                WORD GamePadButtons, WORD ButtonBit)
{
    NewButton->EndedDown = ((GamePadButtons & ButtonBit) == ButtonBit);
    NewButton->HalfTransitionCount = (OldButton->EndedDown != NewButton->EndedDown)? 1 : 0;
}

internal void
Win32ToggleFullscreen(HWND Window)
{
    // NOTE(wheatdog): This follow Raymond Chen's prescription
    // for fullscreen toggle, see:
    // http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if (GetWindowPlacement(Window, &GlobalWindowPlacement) &&
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLongA(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPlacement);
        SetWindowPos(Window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal void
Win32ProcessPendingMessage(game_controller_input *KeyboardController)
{
    MSG Message;

    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;

            case WM_KEYUP:
            case WM_KEYDOWN:
            case WM_SYSKEYUP:
            case WM_SYSKEYDOWN:
            {
                WPARAM VKCode = Message.wParam;
                bool WasDown = ((Message.lParam & (1 << 30)) != 0);
                bool IsDown = ((Message.lParam & (1 << 31)) == 0);

                if (IsDown != WasDown)
                {
                    if (VKCode == 'W')
                    {
                        Win32ProcessKeyboardButton(IsDown, &KeyboardController->MoveUp);
                    }
                    else if (VKCode == 'A')
                    {
                        Win32ProcessKeyboardButton(IsDown, &KeyboardController->MoveLeft);
                    }
                    else if (VKCode == 'S')
                    {
                        Win32ProcessKeyboardButton(IsDown, &KeyboardController->MoveDown);
                    }
                    else if (VKCode == 'D')
                    {
                        Win32ProcessKeyboardButton(IsDown, &KeyboardController->MoveRight);
                    }
                    else if (VKCode == 'Q')
                    {
                        Win32ProcessKeyboardButton(IsDown, &KeyboardController->LeftSoulder);
                    }
                    else if (VKCode == 'E')
                    {
                        Win32ProcessKeyboardButton(IsDown, &KeyboardController->RightSoulder);
                    }
                    else if (VKCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardButton(IsDown, &KeyboardController->Back);
                    }
                    else if (VKCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardButton(IsDown, &KeyboardController->Start);
                    }
                }

                b32 AltKeyIsDown = (Message.lParam & (1 << 29));
                if (IsDown)
                {
                    if ((VKCode == VK_F4) && AltKeyIsDown)
                    {
                        GlobalRunning = false;
                    }

                    if ((VKCode == VK_RETURN) && AltKeyIsDown)
                    {
                        Win32ToggleFullscreen(Message.hwnd);
                    }
                }

            } break;

            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }
}

internal FILETIME
Win32GetFileLastWritedTime(char *FileName)
{
    FILETIME Result = {};
    WIN32_FILE_ATTRIBUTE_DATA FileAttribute;
    if (!GetFileAttributesEx(FileName, GetFileExInfoStandard, &FileAttribute))
    {
        return Result;
    }

    Result = FileAttribute.ftLastWriteTime;
    return Result;
}


internal void
Win32GetExeFullPath(win32_state *Win32State)
{
    // NOTE(wheatdog): Never use MAX_PATH in code that is user-facing because
    // it can be dangerous and lead to bad results.
    DWORD ExeNameLength = GetModuleFileName(0, Win32State->ExeFullPath,
                                            sizeof(Win32State->ExeFullPath));

    Win32State->OnePastExeFullPathLastSlash = Win32State->ExeFullPath;
    for (char *Scan = Win32State->ExeFullPath;
         *Scan != '\0';
         ++Scan)
    {
        if (*Scan == '\\')
        {
            Win32State->OnePastExeFullPathLastSlash = Scan + 1;
        }
    }
}

internal u32
StrLength(char *A)
{
    u32 Length = 0;
    while (*A++)
    {
        ++Length;
    }

    return Length;
}

internal void
CatStrings(char *SourceA, size_t SourceACount, char *SourceB, size_t SourceBCount,
           char *Dest, size_t DestSize)
{
    // TODO(wheatdog): Handle buffer overload
    Assert(DestSize > SourceACount + SourceBCount);
    for (u32 Index = 0; Index < SourceACount; ++Index)
    {
        *Dest++ = *SourceA++;
    }

    for (u32 Index = 0; Index < SourceBCount; ++Index)
    {
        *Dest++ = *SourceB++;
    }

    *Dest = '\0';
}

internal void
Win32GenerateFullPathInBuildDir(win32_state *Win32State, char *FileName, char *Dest, u32 DestSize)
{
    CatStrings(Win32State->ExeFullPath,
               Win32State->OnePastExeFullPathLastSlash - Win32State->ExeFullPath,
               FileName, StrLength(FileName),
               Dest, DestSize);
}

internal win32_game_code
Win32LoadGameCode(char *SourceDLLName, char *TempDLLName, char *LockFileName)
{
    win32_game_code Result = {};

    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    if (GetFileAttributesEx(LockFileName, GetFileExInfoStandard, &Ignored))
    {
        return Result;
    }

    CopyFile(SourceDLLName, TempDLLName, FALSE);
    Result.DLL = LoadLibrary(TempDLLName);
    if (!Result.DLL)
    {
        return Result;
    }

    Result.UpdateAndRender = (game_update_and_render *) GetProcAddress(Result.DLL, "GameUpdateAndRender");
    Result.FillSound = (game_fill_sound *) GetProcAddress(Result.DLL, "GameFillSound");
    Result.LastWriteTime = Win32GetFileLastWritedTime(SourceDLLName);
    Result.IsValid = Result.UpdateAndRender && Result.FillSound;

    if (!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
        Result.FillSound = 0;
    }

    return Result;
}


int CALLBACK
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR Commandline, int ShowCode)
{
    win32_state Win32State;
    Win32GetExeFullPath(&Win32State);

    char GameCodeDLLFullPath[MAX_PATH];
    Win32GenerateFullPathInBuildDir(&Win32State, "wastale.dll", GameCodeDLLFullPath, sizeof(GameCodeDLLFullPath));

    char TempCodeDLLFullPath[MAX_PATH];
    Win32GenerateFullPathInBuildDir(&Win32State, "wastale_temp.dll", TempCodeDLLFullPath, sizeof(TempCodeDLLFullPath));

    char LockFileFullPath[MAX_PATH];
    Win32GenerateFullPathInBuildDir(&Win32State, "lock.tmp", LockFileFullPath, sizeof(LockFileFullPath));

    LARGE_INTEGER PerfCountFreq;
    if (QueryPerformanceFrequency(&PerfCountFreq) == 0)
    {
        // TODO(wheatdog): Logging
    }
    GlobalPerfCountFreq = PerfCountFreq.QuadPart;

    Win32ResizeDIBSection(&GlobalScreenBuffer, 960, 540);
    Win32LoadXInput();
    win32_game_code Game = Win32LoadGameCode(GameCodeDLLFullPath, TempCodeDLLFullPath, LockFileFullPath);

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

    // TODO(wheatdog): How do we reliably query this on Windows?
    int MonitorRefreshHz = 60;
    HDC RefreshDC = GetDC(Window);
    int Win32RefreshHz = GetDeviceCaps(RefreshDC, VREFRESH);
    ReleaseDC(Window, RefreshDC);
    if (Win32RefreshHz > 1)
    {
        MonitorRefreshHz = Win32RefreshHz;
    }
    r32 GameRefreshHz = (MonitorRefreshHz / 2.0f);
    r32 TargetSecondElapsed = 1.0f / GameRefreshHz;

    // TODO(wheatdog): Thread
    thread_context Thread = {};

    // TODO(wheatdog): Handle various memory footprints
    game_memory GameMemory = {};
    GameMemory.PermanentStorageSize = MegaBytes(512);
    GameMemory.TransientStorageSize = GigaBytes(1);
    u64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
    GameMemory.PermanentStorage = VirtualAlloc(0, TotalSize, MEM_RESERVE|MEM_COMMIT,
                                               PAGE_READWRITE);
    GameMemory.TransientStorage = ((u8 *)GameMemory.PermanentStorage +
                                   GameMemory.PermanentStorageSize);
    GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
    GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
    GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

    if (!GameMemory.PermanentStorage || !GameMemory.TransientStorage)
    {
        //TODO(wheatdog): Logging
        return -1;
    }

    // TODO(wheatdog): Need more robust test here. This value seems to be the
    // maximum latency I will get.
    r32 MaxAudioLantencyInFrames = 1.5f;
    u32 SamplePerSecond = 48000;
    u32 ChannelCount = 2;
    u32 BytePerSample = sizeof(i16);
    u32 LatencyInSample = (u32)(MaxAudioLantencyInFrames*(SamplePerSecond / GameRefreshHz));
    win32_xaudio Win32Audio = Win32InitXAudio2(SamplePerSecond, SamplePerSecond,
                                                ChannelCount, BytePerSample, LatencyInSample);

    game_sound GameSound = {};
    GameSound.SamplePerSecond = SamplePerSecond;
    GameSound.Buffer = VirtualAlloc(0, Win32Audio.BufferSizeInByte,
                                     MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    GameSound.ChannelCount = ChannelCount;
    GameSound.BytePerSample = BytePerSample;

    if (!GameSound.Buffer)
    {
        //TODO(wheatdog): Logging
        return -1;
    }

    game_input GameInput[2] = {};
    game_input *OldInput = &GameInput[0];
    game_input *NewInput = &GameInput[1];

    GlobalRunning = true;
    i64 LastCycleCount = __rdtsc();
    LARGE_INTEGER LastCounter = Win32GetPerfCount();
    while(GlobalRunning)
    {
        FILETIME ThisWriteTime = Win32GetFileLastWritedTime(GameCodeDLLFullPath);
        if (CompareFileTime(&ThisWriteTime, &Game.LastWriteTime) != 0)
        {
            Win32UnloadGameCode(&Game);
            Game = Win32LoadGameCode(GameCodeDLLFullPath, TempCodeDLLFullPath, LockFileFullPath);
        }

        NewInput->dtForFrame = TargetSecondElapsed;

        // TODO(wheatdog): Zeroing marcro
        // TODO(wheatdog): We can't zero everything because the up/down state
        // will be wrong.
        game_controller_input *OldKeyboardController = OldInput->Controllers;
        game_controller_input *NewKeyboardController = NewInput->Controllers;

        *NewKeyboardController = {};
        NewKeyboardController->IsConnected = true;
        for (int ButtonIndex = 0;
             ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
             ++ButtonIndex)
        {
            NewKeyboardController->Buttons[ButtonIndex]
                = OldKeyboardController->Buttons[ButtonIndex];
        }

        Win32ProcessPendingMessage(NewKeyboardController);

        if (NewKeyboardController->MoveUp.EndedDown)
        {
            NewKeyboardController->LeftStick.Y = 1.0f;
        }
        if (NewKeyboardController->MoveLeft.EndedDown)
        {
            NewKeyboardController->LeftStick.X = -1.0f;
        }
        if (NewKeyboardController->MoveDown.EndedDown)
        {
            NewKeyboardController->LeftStick.Y = -1.0f;
        }
        if (NewKeyboardController->MoveRight.EndedDown)
        {
            NewKeyboardController->LeftStick.X = 1.0f;
        }

        DWORD ControllerMax = XUSER_MAX_COUNT;
        if (ControllerMax > (ArrayCount(NewInput->Controllers) - 1))
        {
            ControllerMax = (ArrayCount(NewInput->Controllers) - 1);
        }

        // TODO(wheatdog): Poll this more frequently.
        for (DWORD ControllerIndex = 0;
             ControllerIndex < ControllerMax;
             ++ControllerIndex)
        {
            u32 ArrayIndex = ControllerIndex + 1;
            game_controller_input *OldController = OldInput->Controllers + ArrayIndex;
            game_controller_input *NewController = NewInput->Controllers + ArrayIndex;

            XINPUT_STATE ControllerState;
            if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
            {
                // NOTE(wheatdog): Controller is connected.
                NewController->IsConnected = true;
                NewController->IsAnalog = OldController->IsAnalog;

                // TODO(wheatdog): This is a square deadzone now, check XInput
                // to verify the "round" deadzone, and make sure how to deal with
                // it.
                // TODO(wheatdog): Need to not poll disconnected controller to
                // avoid xinput frame rate hit on older libraries...
                // TODO(wheatdog): See if dwPacketNumber increments too rapidly
                XINPUT_GAMEPAD *GamePad = &ControllerState.Gamepad;

                NewController->LeftStick.X = Win32ProcessXInputStickValue(GamePad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                NewController->LeftStick.Y = Win32ProcessXInputStickValue(GamePad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                NewController->RightStick.X = Win32ProcessXInputStickValue(GamePad->sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
                NewController->RightStick.Y = Win32ProcessXInputStickValue(GamePad->sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

                if ((NewController->LeftStick.X != 0.0f) ||
                    (NewController->LeftStick.Y != 0.0f) ||
                    (NewController->RightStick.X != 0.0f) ||
                    (NewController->RightStick.Y != 0.0f))
                {
                    NewController->IsAnalog = true;
                }

                Win32ProcessXInputDigitalButton(&NewController->MoveUp, &OldController->MoveUp, GamePad->wButtons, XINPUT_GAMEPAD_DPAD_UP);
                Win32ProcessXInputDigitalButton(&NewController->MoveDown, &OldController->MoveDown, GamePad->wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
                Win32ProcessXInputDigitalButton(&NewController->MoveLeft, &OldController->MoveLeft, GamePad->wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
                Win32ProcessXInputDigitalButton(&NewController->MoveRight, &OldController->MoveRight, GamePad->wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);

                if (NewController->MoveUp.EndedDown)
                {
                    NewController->LeftStick.Y = 1.0f;
                    NewController->IsAnalog = false;
                }

                if (NewController->MoveDown.EndedDown)
                {
                    NewController->LeftStick.Y = -1.0f;
                    NewController->IsAnalog = false;
                }

                if (NewController->MoveLeft.EndedDown)
                {
                    NewController->LeftStick.X = -1.0f;
                    NewController->IsAnalog = false;
                }

                if (NewController->MoveRight.EndedDown)
                {
                    NewController->LeftStick.X = 1.0f;
                    NewController->IsAnalog = false;
                }

                Win32ProcessXInputDigitalButton(&NewController->YButton, &OldController->YButton, GamePad->wButtons, XINPUT_GAMEPAD_Y);
                Win32ProcessXInputDigitalButton(&NewController->XButton, &OldController->XButton, GamePad->wButtons, XINPUT_GAMEPAD_X);
                Win32ProcessXInputDigitalButton(&NewController->AButton, &OldController->AButton, GamePad->wButtons, XINPUT_GAMEPAD_A);
                Win32ProcessXInputDigitalButton(&NewController->BButton, &OldController->BButton, GamePad->wButtons, XINPUT_GAMEPAD_B);

                Win32ProcessXInputDigitalButton(&NewController->Start, &OldController->Start, GamePad->wButtons, XINPUT_GAMEPAD_START);
                Win32ProcessXInputDigitalButton(&NewController->Back, &OldController->Back, GamePad->wButtons, XINPUT_GAMEPAD_BACK);

                Win32ProcessXInputDigitalButton(&NewController->LeftSoulder, &OldController->LeftSoulder, GamePad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
                Win32ProcessXInputDigitalButton(&NewController->RightSoulder, &OldController->RightSoulder, GamePad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
            }
            else
            {
                // NOTE(wheatdog): NOT connected.
                NewController->IsConnected = false;
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
        Win32Audio.SourceVoice->GetState(&VoiceState);
        u32 PlayCursor = (u32)(VoiceState.SamplesPlayed % Win32Audio.BufferSizeInSample);
        u32 RemainSample = (Win32Audio.WriteCursor - PlayCursor);
        if (Win32Audio.WriteCursor < PlayCursor)
        {
            RemainSample += Win32Audio.BufferSizeInSample;
        }
        GameSound.SampleCount = Win32Audio.LatencyInSample - RemainSample;

        char ShowBuffer[128];
        snprintf(ShowBuffer, sizeof(ShowBuffer), "PC: %d, WC: %d, RS: %d, Fill: %d\n",
                 PlayCursor, Win32Audio.WriteCursor, RemainSample, GameSound.SampleCount);
        OutputDebugStringA(ShowBuffer);

        if (Game.IsValid)
        {
            Game.FillSound(&Thread, &GameMemory, &GameSound);
        }

        Win32FillSoundBuffer(&GameSound, &Win32Audio);

        if (Game.IsValid)
        {
            Game.UpdateAndRender(&Thread, &GameMemory, &GameScreenBuffer, NewInput);
        }

        LARGE_INTEGER WorkCounter = Win32GetPerfCount();
        r32 SecondElapseInFrame = Win32GetSecondsElapse(LastCounter, WorkCounter);
        r32 Duration = SecondElapseInFrame;

        if (SecondElapseInFrame < TargetSecondElapsed)
        {
#if 0
            if (SleepIsGranular)
            {
                DWORD SleepMS = (DWORD)(1000.0f*(TargetSecondElapsed - SecondElapseInFrame));
                if (SleepMS > 0)
                {
                    Sleep(SleepMS);
                }
            }
#endif

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
        r32 WaitingSecond = Win32GetSecondsElapse(WorkCounter, EndCounter);
        r32 SecondsElapse = Win32GetSecondsElapse(LastCounter, EndCounter);
        r32 MSElapse = 1000.0f*SecondsElapse;
        r32 FPS = 1.0f / SecondsElapse;
        LastCounter = EndCounter;

        // char Buffer[128];
        // snprintf(Buffer, sizeof(Buffer), "D:%0.2fms/f, W:%0.2fms/f, %0.2fms/f, %0.2f/s, %0.2fMc/f\n",
        //          Duration*1000, WaitingSecond*1000, MSElapse, FPS, MCPerFrame);
        // OutputDebugStringA(Buffer);

        HDC DeviceContext = GetDC(Window);
        win32_screen_dimension ScreenDim = Win32GetScreenDimension(Window);
        Win32DisplayBufferToScreen(&GlobalScreenBuffer, DeviceContext,
                                   ScreenDim.Width, ScreenDim.Height);
        ReleaseDC(Window, DeviceContext);

        game_input *Temp = NewInput;
        NewInput = OldInput;
        OldInput = Temp;
    }

    return 0;
}
