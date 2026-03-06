#include "stdio.h"
#include "stdint.h"
#include "Windows.h"
#include "math.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <vector>
#include <iostream>
#include <string>


typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

const char* gAppName = "Solanum";
LRESULT CALLBACK Win32WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

#define BIT(x) (1UL << (x))


void SolAssert(uint64 expr)
{
#if SOL_DEBUG
    if (expr)
    {
        return;
    }
    
    DebugBreak();
    ExitProcess(-1);
#endif
}


void SolAssertMsg(uint64 expr, std::string msg)
{
#if SOL_DEBUG
	if (expr)
	{
		return;
	}

    std::cerr << msg << std::endl;
	DebugBreak();
	ExitProcess(-1);
#endif
}


#if SOL_DEBUG
#define ASSERT(EXPR) SolAssert((uint64)(EXPR))
#define ASSERT_MSG(EXPR, MSG) SolAssertMsg((uint64)(EXPR), (MSG))
#define VKASSERT(EXPR) ASSERT(EXPR == VK_SUCCESS)
#else
#define ASSERT_MSG(EXPR, MSG) (EXPR)
#define ASSERT(EXPR) (EXPR)
#define VKASSERT(EXPR) (EXPR)
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

	bool Init(int32 width, int32 height);
	void Show();
	void Destroy();
	void Close();
	bool IsClosed();
};

struct Renderer
{
    VkInstance mInstance;
    VkDevice mGPU;
    VkQueue mQueue;
    VkPhysicalDevice mGPUPhysicalHandle;
    VkSurfaceKHR mSurface;
    uint32 mGraphicsQueueFamilyType;

#if SOL_DEBUG
    VkDebugUtilsMessengerEXT mDebugMessenger;
#endif;

    bool Init(Window& win);
    bool CreateSurface();
    bool PickPhysicalDevice();
    bool CreateDevice();
    
    Window* mWindow;

};

Renderer gRenderer;

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanValidationLayerLogCallback(VkDebugUtilsMessageSeverityFlagBitsEXT  messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
#if SOL_DEBUG
    ASSERT_MSG(!(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT), pCallbackData->pMessage);
	return VK_FALSE;
#endif
}

#if SOL_DEBUG
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto loadedFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (loadedFunc != nullptr)
	{
		return loadedFunc(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto loadedFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (loadedFunc != nullptr)
		loadedFunc(instance, debugMessenger, pAllocator);
}
#endif

bool Renderer::Init(Window& win)
{
    mWindow = &win;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Solanum";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 4, 0);
    appInfo.pEngineName = "Solanum";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 4, 0);
    appInfo.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> requiredExtensions;
    requiredExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    requiredExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    
#if SOL_DEBUG
    requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    uint32 supportedExtensionCount = 0;

    vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);
    std::vector<VkExtensionProperties> vkSupportedExtensions(supportedExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, vkSupportedExtensions.data());

    for (int32 x = 0; x < requiredExtensions.size(); x++)
    {
        bool found = false;
        
        for (int32 i = 0; i < vkSupportedExtensions.size(); i++)
        {
            if (requiredExtensions[x] == std::string(vkSupportedExtensions[i].extensionName))
            {
                found = true; 
                break;
            }
        }

        if (!found)
            ASSERT_MSG(false, "Extension not found: " + std::string(requiredExtensions[x]));
    }


    createInfo.enabledExtensionCount = (int32)requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

#if SOL_DEBUG
    uint32 layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    const char* debugLayer = "VK_LAYER_KHRONOS_validation";

    bool layerFound = false;
    for (int32 i = 0; i < availableLayers.size(); i++)
    {
        if (std::string(availableLayers[i].layerName) == debugLayer)
        {
            layerFound = true;
            break;
        }
    }

    ASSERT_MSG(layerFound, "Could not find debug layer.");

    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = &debugLayer;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateinfo = {};
    debugCreateinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateinfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateinfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateinfo.pfnUserCallback = VulkanValidationLayerLogCallback;
    debugCreateinfo.pUserData = nullptr;

    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateinfo;
#endif

    VKASSERT(vkCreateInstance(&createInfo, nullptr, &mInstance));
    VKASSERT(CreateDebugUtilsMessengerEXT(mInstance, &debugCreateinfo, nullptr, &mDebugMessenger));

    CreateSurface();
    PickPhysicalDevice();
    CreateDevice();

    return true;
}

bool Renderer::CreateSurface()
{
    VkWin32SurfaceCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hwnd = mWindow->mWinHandle;
    info.hinstance = GetModuleHandle(NULL);
    info.flags = 0;
    info.pNext = NULL;
    
    vkCreateWin32SurfaceKHR(mInstance, &info, nullptr, &mSurface);

    return true;
}

bool Renderer::PickPhysicalDevice()
{
    uint32 deviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);

    ASSERT_MSG(deviceCount, "No Vulkan compatible GPU found");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

    uint32 graphicsFamily = (uint32)-1;

    for (const VkPhysicalDevice& device : devices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        bool discrete = deviceProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        bool graphicsFamilySupported = false;

        uint32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());


        for (int32_t i = 0; i < queueFamilies.size(); i++)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &presentSupport);

                if (!presentSupport)
                    continue;

                graphicsFamily = i;
                gRenderer.mGPUPhysicalHandle = device;

                break;
            }
        }
    }

        ASSERT_MSG(graphicsFamily != (uint32)-1, "Couldn't find suitable GPU");
        mGraphicsQueueFamilyType = graphicsFamily;

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(mGPUPhysicalHandle, &props);
        std::cout << std::string(props.deviceName) + "\n";

        return true;
}

bool Renderer::CreateDevice()
{
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = mGraphicsQueueFamilyType;
    queueCreateInfo.queueCount = 1;

    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    deviceCreateInfo.enabledLayerCount = 0;

#if SOL_DEBUG
    const char* debugLayer = "VK_LAYER_KHRONOS_validation";
    deviceCreateInfo.enabledLayerCount = 1;
    deviceCreateInfo.ppEnabledLayerNames = &debugLayer;
#endif

    std::vector<const char*> deviceRequiredExtensions;
    deviceRequiredExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    deviceRequiredExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    deviceRequiredExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    deviceRequiredExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);

    uint32 extesionCount = 0;
    vkEnumerateDeviceExtensionProperties(mGPUPhysicalHandle, nullptr, &extesionCount, nullptr);
    std::vector<VkExtensionProperties> availableDeviceExtensions(extesionCount);
    vkEnumerateDeviceExtensionProperties(mGPUPhysicalHandle, nullptr, &extesionCount, availableDeviceExtensions.data());

	for (int32 x = 0; x < deviceRequiredExtensions.size(); x++)
	{
		bool found = false;

		for (int32 i = 0; i < availableDeviceExtensions.size(); i++)
		{
			std::string requiredExtension(deviceRequiredExtensions[x]);
			if (requiredExtension == availableDeviceExtensions[i].extensionName)
			{
				found = true;
				break;
			}
		}

		if (!found)
            ASSERT_MSG(false, "Required extension not found / supported!Extension: " + std::string(deviceRequiredExtensions[x]) + '\n');
	}

    
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> swapChainFormats;
	std::vector<VkPresentModeKHR> swapChainPresentModes;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mGPUPhysicalHandle, mSurface, &surfaceCapabilities);

    uint32 formatCount = 0;

    vkGetPhysicalDeviceSurfaceFormatsKHR(mGPUPhysicalHandle, mSurface, &formatCount, nullptr);

    if (formatCount)
    {
        swapChainFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(mGPUPhysicalHandle, mSurface, &formatCount, swapChainFormats.data());
    }

    uint32 presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(mGPUPhysicalHandle, mSurface, &presentModeCount, nullptr);

    if (presentModeCount)
    {
        swapChainPresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(mGPUPhysicalHandle, mSurface, &presentModeCount, swapChainPresentModes.data());
    }

    ASSERT_MSG(!swapChainPresentModes.empty() || !swapChainFormats.empty(), "No suitable swap chain found.");

    deviceCreateInfo.enabledExtensionCount = (uint32)deviceRequiredExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceRequiredExtensions.data();

    VKASSERT(vkCreateDevice(mGPUPhysicalHandle, &deviceCreateInfo, nullptr, &mGPU));

    vkGetDeviceQueue(mGPU, mGraphicsQueueFamilyType, 0, &mQueue);

    return true;
}

bool Window::Init(int32 width, int32 height)
{
    mWidth = width;
    mHeight = height;

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
		mWidth, mHeight,
		NULL, NULL,
		windowClass.hInstance,
		NULL);
	ASSERT(winHandle);

	mWinHandle = winHandle;

    return true;
}

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

    mPosX = (int32)x;
    mPosY = (int32)y;
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
    gWindow.Init(800, 600);
    gWindow.Show();

    gRenderer.Init(gWindow);

    while (!gWindow.IsClosed())
    {
        Win32MessageLoop();
        gInput.Poll();
        gInput.Debug();
    }

	//#TODO wrap under destroy funcs

    vkDestroyDevice(gRenderer.mGPU, nullptr);
	vkDestroySurfaceKHR(gRenderer.mInstance, gRenderer.mSurface, nullptr);
    DestroyDebugUtilsMessengerEXT(gRenderer.mInstance, gRenderer.mDebugMessenger, nullptr);
	vkDestroyInstance(gRenderer.mInstance, nullptr);

    gWindow.Destroy();

    return 0;
}