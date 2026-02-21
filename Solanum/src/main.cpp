#include "stdio.h"
#include "stdint.h"
#include "Windows.h"
#include "math.h"

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

    void Show();
    void Destroy();
    void Close();
    bool IsClosed();
};

void Window::Show()
{
    ShowWindow(mWinHandle, SW_SHOWNORMAL);
    SetActiveWindow(mWinHandle);
}

void Window::Destroy()
{
    DestroyWindow(mWinHandle);
}

void Window::Close()
{
    mFlags |= WINDOW_FLAGS_CLOSE;
}

bool Window::IsClosed()
{
    return mFlags & WINDOW_FLAGS_CLOSE;
}

#define MAX_KEYS 256

struct Input
{
    uint8 mKeys[MAX_KEYS];
    uint8 mPrevKeys[MAX_KEYS];

    // Cursor position in pixels from top left of window
    int32 mPosX   = 0;
    int32 mPosY   = 0;
    // Direction from last cursor movement (normalized)
    float mDeltaX = 0.f;
    float mDeltaY = 0.f;

    void Poll();
    bool IsDown(uint8 key);
    bool IsJustDown(uint8 key);
    bool IsUp(uint8 key);
    bool IsJustUp(uint8 key);
    void Debug();
};

void Input::Poll()
{
    // Keys
    memcpy(mPrevKeys, mKeys, MAX_KEYS * sizeof(uint8));

    BOOL ret = GetKeyboardState(mKeys);
    ASSERT(ret);

    // Cursor
    POINT point;
    ret = GetCursorPos(&point);
    ASSERT(ret);
    HWND window = GetActiveWindow();
    if (!window) return;

    ret = ScreenToClient(window, &point);
    ASSERT(ret);

    float x = (float)point.x;
    float y = (float)point.y;
    float prevx = (float)mPosX;
    float prevy = (float)mPosY;

    float dx = x - prevx;
    float dy = y - prevy;
    float dlen = sqrtf(dx * dx + dy * dy);

    if (dlen < 1e-5)
    {
        mDeltaX = 0;
        mDeltaY = 0;
    }
    else
    {
        mDeltaX = dx / dlen;
        mDeltaY = dy / dlen;
    }

    mPosX = x;
    mPosY = y;
}

bool Input::IsDown(uint8 key)
{
    return mKeys[key] & 0x80;
}

bool Input::IsJustDown(uint8 key)
{
    return IsDown(key)
        && !(mPrevKeys[key] & 0x80);
}

bool Input::IsUp(uint8 key)
{
    return !(mKeys[key] & 0x80);
}

bool Input::IsJustUp(uint8 key)
{
    return IsUp(key)
        && (mPrevKeys[key] & 0x80);
}

void Input::Debug()
{
    for (uint32 i = 0; i < MAX_KEYS; i++)
    {
        if (IsJustDown(i))
        {
            printf("Key %u is down\n", i);
        }
    }

    if (mDeltaX != 0 || mDeltaY != 0)
    {
        printf("Mouse pos x %d, y %d (delta %.2f, %.2f)\n",
            mPosX, mPosY, mDeltaX, mDeltaY);
    }
}

const char* gAppName = "Solanum";
Window gWindow = {};
Input gInput = {};

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
        gInput.Poll();
        gInput.Debug();
    }

    gWindow.Destroy();

    return 0;
}