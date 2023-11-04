#include <windows.h>
#include <stdint.h>

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
dimensions {
    LONG Width;
    LONG Height;
};

global_variable bool GlobalRunning;
global_variable bitmap_buff GlobalBitmap;


internal void
CalculateDimentions(RECT Src, dimensions *Dst) {
    Dst->Width = Src.right - Src.left;
    Dst->Height = Src.bottom - Src.top;
}

internal void
RenderWeirdGradient(bitmap_buff *GlobalBitmap, uint8_t Xoffset, uint8_t Yoffset) {
    uint32_t *Pitch = (uint32_t *)GlobalBitmap->Memory;
    for (LONG i = 0; i < GlobalBitmap->Height; ++i) {
        for (LONG j = 0; j < GlobalBitmap->Width; ++j) {
            uint8_t Green = Xoffset + i;
            uint8_t Blue = Yoffset + j;
            uint8_t Red = i - j;
            *Pitch++ = (Red << 16) | (Green << 8) | Blue;
        }
    }
}


internal void
Win32ResizeDIBSection(bitmap_buff *GlobalBitmap, LONG Width, LONG Height) {
    if (GlobalBitmap->Memory)
        VirtualFree(GlobalBitmap->Memory, 0, MEM_RELEASE);

    GlobalBitmap->Info.bmiHeader.biSize = sizeof(GlobalBitmap->Info.bmiHeader);
    GlobalBitmap->Info.bmiHeader.biWidth = Width;
    GlobalBitmap->Info.bmiHeader.biHeight = Height;
    GlobalBitmap->Info.bmiHeader.biPlanes = 1;
    GlobalBitmap->Info.bmiHeader.biBitCount = 32;
    GlobalBitmap->Info.bmiHeader.biCompression = BI_RGB;

    GlobalBitmap->Height = Height;
    GlobalBitmap->Width = Width;

    int BytesPerPixel = 4;
    SIZE_T BitmapMemorySize = BytesPerPixel * Width * Height;
    GlobalBitmap->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void
Win32UpdateWindow(HDC DeviceContext, RECT ClientRect, bitmap_buff *GlobalBitmap, LONG X, LONG Y, LONG Width, LONG Height) {
    dimensions ClientDim;
    CalculateDimentions(ClientRect, &ClientDim);
    StretchDIBits(DeviceContext, 
//                  X, Y, Width, Height, 
//                  X, Y, Width, Height, 
                  0, 0, ClientDim.Width, ClientDim.Height,
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

    case WM_SIZE:
        {
            RECT ClientRect;
            dimensions BitmapDim;
            GetClientRect(Window, &ClientRect);
            CalculateDimentions(ClientRect, &BitmapDim);
            Win32ResizeDIBSection(&GlobalBitmap, BitmapDim.Width, BitmapDim.Height);
        } break;

    case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

    case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            dimensions PaintDim;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            LONG X = Paint.rcPaint.left;
            LONG Y = Paint.rcPaint.top;
            CalculateDimentions(Paint.rcPaint, &PaintDim);
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            Win32UpdateWindow(DeviceContext, ClientRect, &GlobalBitmap, X, Y, PaintDim.Width, PaintDim.Height);
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
            GlobalRunning = true;
            uint8_t Xo = 0;
            uint8_t Yo = 0;
            while (GlobalRunning) {
                ++Xo;
                ++Yo;
                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                    if (Message.message == WM_QUIT)
                        GlobalRunning = false;

                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                
                HDC DeviceContext = GetDC(Window);
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);

                RenderWeirdGradient(&GlobalBitmap, Xo, Yo);
                LONG WindowWidth = ClientRect.right - ClientRect.left;
                LONG WindowHeight = ClientRect.top - ClientRect.bottom;

                Win32UpdateWindow(DeviceContext, ClientRect, &GlobalBitmap, 0, 0, WindowWidth, WindowHeight);
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
