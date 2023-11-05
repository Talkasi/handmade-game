#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

#define internal static
#define global_variable static
#define local_persist static

struct 
bitmap_buff {
    BITMAPINFO Info;
    LPVOID Memory;
    LONG Height;
    LONG Width;
};

struct
window_dimension {
    LONG Width;
    LONG Height;
};

global_variable bool GlobalRunning;
global_variable bitmap_buff GlobalBitmap;
global_variable LPDIRECTSOUNDBUFFER GlobalSoundBuffer;

uint8_t Xo = 0;
uint8_t Yo = 0;

#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XINPUT_GET_STATE(xinput_get_state);
XINPUT_GET_STATE(XInputGetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable xinput_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef XINPUT_SET_STATE(xinput_set_state);
XINPUT_SET_STATE(XInputSetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable xinput_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void 
Win32LoadXInput(void) {
    HMODULE XInputLib = LoadLibraryA("xinput1_4.dll");
    if (!XInputLib)
        XInputLib = LoadLibraryA("xinput1_3.dll");
    
    if (XInputLib) {
        XInputGetState = (xinput_get_state *)GetProcAddress(XInputLib, "XInputGetState");
        XInputSetState = (xinput_set_state *)GetProcAddress(XInputLib, "XInputSetState");
    }
}

internal void   
Win32InitSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize) {
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    if (DSoundLibrary) {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)
            GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))) {
                    if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))) {
                        OutputDebugStringA("Buffer1 set\n");
                    } else {

                    }
                } else {

                }
            } else {
                // Diagnostic
            }

            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSoundBuffer, 0))) {
                OutputDebugStringA("Buffer2 set\n");
            } else {

            }

        } else {

        }
    } else {

    }
}

internal window_dimension
GetWindowDimention(HWND Window) {
    window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void
RenderWeirdGradient(bitmap_buff *GlobalBitmap, uint8_t Xoffset, uint8_t Yoffset) {
    uint32_t *Pitch = (uint32_t *)GlobalBitmap->Memory;
    for (LONG i = 0; i < GlobalBitmap->Height; ++i) {
        for (LONG j = 0; j < GlobalBitmap->Width; ++j) {
            uint8_t Green = Xoffset + i;
            uint8_t Blue = Yoffset + j;
            uint8_t Red = Xoffset - Yoffset + i - j;
            *Pitch++ = (Red << 16) | (Green << 8) | Blue;
        }
    }
}


internal void
Win32ResizeDIBSection(bitmap_buff *GlobalBitmap, LONG Width, LONG Height) {
    if (GlobalBitmap->Memory)
        VirtualFree(GlobalBitmap->Memory, 0, MEM_RELEASE);

    GlobalBitmap->Width = Width;
    GlobalBitmap->Height = Height;

    GlobalBitmap->Info.bmiHeader.biSize = sizeof(GlobalBitmap->Info.bmiHeader);
    GlobalBitmap->Info.bmiHeader.biWidth = GlobalBitmap->Width;
    GlobalBitmap->Info.bmiHeader.biHeight = -GlobalBitmap->Height;
    GlobalBitmap->Info.bmiHeader.biPlanes = 1;
    GlobalBitmap->Info.bmiHeader.biBitCount = 32;
    GlobalBitmap->Info.bmiHeader.biCompression = BI_RGB;

    int BytesPerPixel = 4;
    SIZE_T BitmapMemorySize = BytesPerPixel * GlobalBitmap->Width * GlobalBitmap->Height;
    GlobalBitmap->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

internal void
Win32UpdateWindow(HDC DeviceContext, LONG WindowWidth, LONG WindowHeight, 
                  bitmap_buff *GlobalBitmap, LONG X, LONG Y, LONG Width, LONG Height) {
    StretchDIBits(DeviceContext, 
//                  X, Y, Width, Height, 
//                  X, Y, Width, Height, 
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, GlobalBitmap->Width, GlobalBitmap->Height,
                  GlobalBitmap->Memory, &GlobalBitmap->Info, 
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT Wndproc(HWND Window, UINT Message,
  WPARAM wParam, LPARAM lParam) {
    LRESULT Result = 0;
    switch(Message) {
    case WM_CLOSE:
        {
            GlobalRunning = false;
        } break;

    case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
        {
            uint32_t VKCode = wParam;
            bool WasDown = ((lParam & (1 << 30)) != 0);
            bool IsDown = ((lParam & (1 << 31)) == 0);

            switch (VKCode) {
            case VK_LEFT:
            case 'A':
                {
                    Yo += 10;
                } break;

            case VK_DOWN:
            case 'S':
                {
                    Xo -= 10;
                } break;

            case VK_RIGHT:
            case 'D':
                {
                    Yo -= 10;
                } break;

            case VK_UP:
            case 'W':
                {
                    Xo += 10;
                } break;

            case VK_ESCAPE:
                {

                } break;

            case VK_SPACE:
                {

                } break;

            case VK_F4:
                {
                    bool IsAltKeyPressed = ((lParam & (1 << 29)) != 0);
                    if (IsAltKeyPressed)
                        GlobalRunning = false;
                } break;
            }
        } break;


    case WM_SIZE:
        {
        } break;

    case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

    case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            LONG X = Paint.rcPaint.left;
            LONG Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            
            window_dimension Dimension = GetWindowDimention(Window);
            Win32UpdateWindow(DeviceContext, Dimension.Width, Dimension.Height,
                              &GlobalBitmap, X, Y, Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;

    default:
        {
            Result = DefWindowProcA(Window, Message, wParam, lParam);
        }
    }

    return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
                     LPSTR CommandLine, int ShowCode) {   
    WNDCLASS WindowClass = {};

    Win32ResizeDIBSection(&GlobalBitmap, 1280, 720);
    Win32LoadXInput();

    WindowClass.style = CS_OWNDC|CS_VREDRAW|CS_HREDRAW;
    WindowClass.lpfnWndProc = Wndproc;
    WindowClass.hInstance = Instance;
//    WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeGameWindowClass";

    if (RegisterClassA(&WindowClass)) {
        HWND Window = CreateWindowEx(0, WindowClass.lpszClassName, "HandmadeHero", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                  0, 0, Instance, 0);

        if (Window) {
            MSG Message;
            HDC DeviceContext = GetDC(Window);

            bool SoundPlaying = false;
            int32_t SamplesPerSecond = 48000;
            int Hz = 256;
            int ToneSound = 3000;
            uint32_t RunningSampleIndex = 0;
            int SquareWavePeriod = SamplesPerSecond / Hz;
            int HalfSquareWacwPeriod = SquareWavePeriod / 2;
            int BytesPerSample = sizeof(int16_t) * 2;
            int SoundBufferSize = SamplesPerSecond * BytesPerSample;

            Win32InitSound(Window, SamplesPerSecond, SamplesPerSecond * sizeof(int16_t) * 2);

            GlobalRunning = true;
            while (GlobalRunning) {
                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                    if (Message.message == WM_QUIT)
                        GlobalRunning = false;

                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                for (DWORD ControlI = 0; ControlI < XUSER_MAX_COUNT; ++ControlI) {
                    XINPUT_STATE ControllerState;
                    if (XInputGetState(ControlI, &ControllerState) == ERROR_SUCCESS)
                    {
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
                        bool Up = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        bool Down = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        bool Left = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        bool Right = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        bool Start = Pad->wButtons & XINPUT_GAMEPAD_START;
                        bool Back = Pad->wButtons & XINPUT_GAMEPAD_BACK;
                        bool LeftShoulder = Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        bool RightShoukder = Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                        bool AButton = Pad->wButtons & XINPUT_GAMEPAD_A;
                        bool BButton = Pad->wButtons & XINPUT_GAMEPAD_B;
                        bool XButton = Pad->wButtons & XINPUT_GAMEPAD_X;
                        bool YButton = Pad->wButtons & XINPUT_GAMEPAD_Y;

                        int16_t StickX = Pad->sThumbLX;
                        int16_t StickY = Pad->sThumbLY;
                    }
                    else
                    {
                        // Controller is not connected
                    }
                }

                // Sound test
                if (!SoundPlaying) {
                    GlobalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
                    SoundPlaying = true;
                }

                DWORD PlayCursor;
                DWORD WriteCursor;
                if (SUCCEEDED(GlobalSoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
                    DWORD ByteToLock = RunningSampleIndex * BytesPerSample % SoundBufferSize;
                    DWORD BytesToWrite;
                    if (ByteToLock == PlayCursor)
                        BytesToWrite = SoundBufferSize;
                    else if (ByteToLock > PlayCursor) {
                        BytesToWrite = (SoundBufferSize - ByteToLock);
                        BytesToWrite += PlayCursor;
                    } else
                        BytesToWrite = PlayCursor - ByteToLock;

                    VOID *AudioPtr1;
                    DWORD Region1Size;
                    VOID *AudioPtr2;
                    DWORD Region2Size;
                    if (SUCCEEDED(GlobalSoundBuffer->Lock(ByteToLock, BytesToWrite, 
                                            &AudioPtr1, &Region1Size, &AudioPtr2, &Region2Size, 0))) {
                        int16_t *SampleOut = (int16_t *)AudioPtr1;
                        DWORD Region1SampleCount = Region1Size / BytesPerSample;
                        DWORD Region2SampleCount = Region2Size / BytesPerSample;
                        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {
                            int16_t SampleValue = ((RunningSampleIndex / HalfSquareWacwPeriod) % 2) ? ToneSound : -ToneSound;
                            *SampleOut++ = SampleValue;
                            *SampleOut++ = SampleValue;
                            ++RunningSampleIndex;
                        }

                        SampleOut = (int16_t *)AudioPtr2;
                        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {
                            int16_t SampleValue = ((RunningSampleIndex / HalfSquareWacwPeriod) % 2) ? ToneSound : -ToneSound;
                            *SampleOut++ = SampleValue;
                            *SampleOut++ = SampleValue;
                            ++RunningSampleIndex;
                        }

                        GlobalSoundBuffer->Unlock(&AudioPtr1, Region1Size, &AudioPtr2, Region2Size);
                    }
                }

                // Rendering test
                RenderWeirdGradient(&GlobalBitmap, Xo, Yo);
                window_dimension Dimension = GetWindowDimention(Window);
                Win32UpdateWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBitmap, 
                                  0, 0, Dimension.Width, Dimension.Height);
            }
            ReleaseDC(Window, DeviceContext);
        }
        else
        {
            // TODO(Talkasi): Loggining
        }
    }
    else 
    {
        // TODO(Talkasi): loggining
    }

    return 0;
}
