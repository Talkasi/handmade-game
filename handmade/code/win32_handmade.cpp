#include <windows.h>
#include <stdint.h>

#define internal static
#define global_variable static
#define local_persist static

global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable LPVOID BitmapMemory;
global_variable LONG BitmapHeight;
global_variable LONG BitmapWidth;


internal void
RenderWeirdGradient(uint8_t Xoffset, uint8_t Yoffset) {
    uint32_t *Pitch = (uint32_t *)BitmapMemory;
    for (LONG i = 0; i < BitmapHeight; ++i) {
        for (LONG j = 0; j < BitmapWidth; ++j) {
            uint8_t Green = Xoffset + i;
            uint8_t Blue = Yoffset + j;
            uint8_t Red = i - j;
            *Pitch++ = (Red << 16) | (Green << 8) | Blue;
        }
    }
}


internal void
Win32ResizeDIBSection(LONG Width, LONG Height) {
    if (BitmapMemory)
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    BitmapHeight = Height;
    BitmapWidth = Width;

    int BytesPerPixel = 4;
    SIZE_T BitmapMemorySize = BytesPerPixel * Width * Height;
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void
Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect, LONG X, LONG Y, LONG Width, LONG Height) {
    /*
        RR GG BB xx
        LE: 0xxxBBGGRR
        So:
        BB GG RR xx
    */
    LONG ClientWidth = ClientRect->right - ClientRect->left;
    LONG ClientHeight = ClientRect->bottom - ClientRect->top;

    //RenderWeirdGradient();

    StretchDIBits(DeviceContext, 
//                  X, Y, Width, Height, 
//                  X, Y, Width, Height, 
                  0, 0, ClientWidth, ClientHeight,
                  0, 0, BitmapWidth, BitmapHeight,
                  BitmapMemory, &BitmapInfo, 
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT Wndproc(HWND Window, UINT Message,
  WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = 0;
    switch(Message){
    case WM_CLOSE:
        {
            Running = false;
        } break;

    case WM_DESTROY:
        {
            Running = false;
        } break;

    case WM_SIZE:
        {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(Width, Height);
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
            LONG Width = Paint.rcPaint.right - Paint.rcPaint.left;
            LONG Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
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
    WindowClass.style = CS_OWNDC|CS_VREDRAW|CS_HREDRAW;
    WindowClass.lpfnWndProc = Wndproc;
    WindowClass.hInstance = Instance;
//    WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeGameWindowClass";

    if (RegisterClassA(&WindowClass)) {
        HWND Window = CreateWindowEx(0, WindowClass.lpszClassName, "HandmadeHero", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                  CW_USEDEFAULT, CW_USEDEFAULT,
                  CW_USEDEFAULT, CW_USEDEFAULT, 
                  0, 0, Instance, 0);

        if (Window) {

            MSG Message;
            Running = true;
            uint8_t Xo = 0;
            uint8_t Yo = 0;
            while (Running) {
                ++Xo;
                ++Yo;
                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                    if (Message.message == WM_QUIT)
                        Running = false;

                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                
                HDC DeviceContext = GetDC(Window);
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);

                RenderWeirdGradient(Xo, Yo);
                LONG WindowWidth = ClientRect.right - ClientRect.left;
                LONG WindowHeight = ClientRect.top - ClientRect.bottom;

                Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
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
