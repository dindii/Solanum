#include "stdio.h"
#include "stdint.h"
#include "Windows.h"

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

#define BIT(x) (1UL << (x))

#if SOL_DEBUG
void SolAssert(uint64 expr)
{
    if (expr)
    {
        return;
    }
    
    DebugBreak();
    ExitProcess(-1);
}

#define ASSERT(EXPR) SolAssert((uint64)(EXPR))
#else
#define ASSERT(EXPR)
#endif

enum WindowFlags : uint16
{
    WINDOW_FLAGS_NONE   = 0,
    WINDOW_FLAGS_CLOSE  = BIT(0),
};
DEFINE_ENUM_FLAG_OPERATORS(WindowFlags);

struct Window
{
    HWND mWinHandle = NULL;
    uint32 mWidth = 0;
    uint32 mHeight = 0;
    WindowFlags mFlags = WINDOW_FLAGS_NONE;

    void Show()
    {
        ShowWindow(mWinHandle, SW_SHOWNORMAL);
        SetActiveWindow(mWinHandle);
    }

    void Destroy()
    {
        DestroyWindow(mWinHandle);
    }

    void Close()
    {
        mFlags |= WINDOW_FLAGS_CLOSE;
    }

    bool IsClosed()
    {
        return mFlags & WINDOW_FLAGS_CLOSE;
    }
};

const char* gAppName = "Solanum";
Window gWindow = {};

LRESULT CALLBACK Win32WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    //if (ImGui_ImplWin32_WndProcHandler(hwnd, umsg, wparam, lparam)) return TRUE;

    switch (umsg)
    {
    case WM_CLOSE:
    {
        gWindow.Close();
    } break;
    case WM_SIZE:
    {
        gWindow.mWidth  = LOWORD(lparam);
        gWindow.mHeight = HIWORD(lparam);
    } break;
    default: break;
    }

    return DefWindowProc(hwnd, umsg, wparam, lparam);
}

void Win32MessageLoop()
{
    MSG msg = {};
    while (true)
    {
        int32 ret = PeekMessage(&msg, gWindow.mWinHandle, 0, 0, PM_REMOVE);
        ASSERT(ret >= 0);
        if (!ret)
        {
            break;
        }

        DispatchMessage(&msg);
    }
}

int main()
{
    printf("Hello World!\n");

    // Init window
    gWindow.mWidth = 800;
    gWindow.mHeight = 600;

    {
        WNDCLASSA windowClass = {};
        windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
        windowClass.lpszClassName = "wndclassname";
        windowClass.lpfnWndProc = Win32WndProc;
        windowClass.hInstance = GetModuleHandle(NULL);  // Active executable
        RegisterClassA(&windowClass);

        HWND winHandle = CreateWindowExA(
            0, windowClass.lpszClassName,
            gAppName,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            gWindow.mWidth, gWindow.mHeight,
            NULL, NULL,
            windowClass.hInstance,
            NULL);
        ASSERT(winHandle);

        gWindow.mWinHandle = winHandle;
    }

    gWindow.Show();

    while (!gWindow.IsClosed())
    {
        Win32MessageLoop();
    }

    gWindow.Destroy();

    return 0;
}