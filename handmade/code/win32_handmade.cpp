#include <windows.h>
#include <stdint.h>
#include <xinput.h>

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

internal void 
Win32LoadXInput(void) {
    HMODULE XInputLib = LoadLibraryA("xinput1_4.dll");
    if (XInputLib)
        XInputLib = LoadLibraryA("xinput1_3.dll");
    
    if (XInputLib) {
        XInputGetState = (xinput_get_state *)GetProcAddress(XInputLib, "XInputGetState");
        XInputSetState = (xinput_set_state *)GetProcAddress(XInputLib, "XInputSetState");
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
    GlobalBitmap->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
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
  WPARAM wParam, LPARAM lParam)
{
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
            case 'a':
                {
                    Yo += 10;
                } break;

            case VK_DOWN:
            case 'S':
            case 's':
                {
                    Xo -= 10;
                } break;

            case VK_RIGHT:
            case 'D':
            case 'd':
                {
                    Yo -= 10;
                } break;

            case VK_UP:
            case 'W':
            case 'w':
                {
                    Xo += 10;
                } break;

            case VK_ESCAPE:
                {

                } break;

            case VK_SPACE:
                {

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
                     LPSTR CommandLine, int ShowCode)
{   
    WNDCLASS WindowClass = {};

    Win32ResizeDIBSection(&GlobalBitmap, 1280, 720);

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
            GlobalRunning = true;
            while (GlobalRunning) {
                //++Xo;
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

                RenderWeirdGradient(&GlobalBitmap, Xo, Yo);

                HDC DeviceContext = GetDC(Window);

                window_dimension Dimension = GetWindowDimention(Window);
                Win32UpdateWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBitmap, 
                                  0, 0, Dimension.Width, Dimension.Height);
                ReleaseDC(Window, DeviceContext);
            }
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
