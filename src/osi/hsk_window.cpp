#include "hsk_window.hpp"
#include "sdl2/SDL_vulkan.h"

namespace hsk
{
    Window::Window()
        : mTitle(), mDisplayMode(EDisplayMode::Windowed), mDisplayId(), mFullScreenSize{0, 0}, mWindowedSize{1280, 720}, mPosition{WINDOWPOS_AUTO, WINDOWPOS_AUTO}, mHandle(nullptr) { mOwningThreadID = static_cast<uint64_t>(SDL_ThreadID()); }

    void Window::DisplayMode(EDisplayMode mode, bool preserveSize)
    {
        if(mDisplayMode == mode)
        {
            return;
        }
        mDisplayMode = mode;
        if(Exists())
        {
            SDL_DisplayMode displayMode;
            SDL_GetDisplayMode(mDisplayId, 0, &displayMode);
            mFullScreenSize = Extent2D{displayMode.w, displayMode.h};

            switch(mDisplayMode)
            {
                case EDisplayMode::Windowed:
                case EDisplayMode::WindowedResizable: {
                    SDL_SetWindowFullscreen(mHandle, 0);
                    SDL_SetWindowResizable(mHandle, mDisplayMode == EDisplayMode::WindowedResizable ? SDL_TRUE : SDL_FALSE);
                    Size(mWindowedSize);
                    break;
                }
                case EDisplayMode::FullscreenHardware: {
                    SDL_SetWindowFullscreen(mHandle, SDL_WINDOW_FULLSCREEN);
                    SDL_SetWindowDisplayMode(mHandle, &displayMode);
                    break;
                }
                case EDisplayMode::FullscreenWindowed: {
                    SDL_SetWindowFullscreen(mHandle, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    break;
                }
            }
        }
    }

    void Window::Position(glm::ivec2 pos)
    {
        mPosition = pos;
        if(Exists() && mDisplayMode <= EDisplayMode::WindowedResizable)
        {
            int x = (mPosition.x == WINDOWPOS_AUTO ? SDL_WINDOWPOS_CENTERED : mPosition.x);
            int y = (mPosition.y == WINDOWPOS_AUTO ? SDL_WINDOWPOS_CENTERED : mPosition.y);
            SDL_SetWindowPosition(mHandle, x, y);
        }
    }

    Window::~Window() { Destroy(); }

    void Window::Create()
    {
        if(Exists())
        {
            return;
        }
        SDL_DisplayMode displayMode;
        SDL_GetDisplayMode(mDisplayId, 0, &displayMode);
        mFullScreenSize = Extent2D{displayMode.w, displayMode.h};
        if(mWindowedSize.IsZeroArea())
        {
            mWindowedSize = Extent2D{1280, 720};
        }
        if(mTitle.length() == 0)
        {
            mTitle = "Raytracing Rapid Prototyping Framework - Hochschule Kempten";
        }
        int x = (mPosition.x == WINDOWPOS_AUTO ? SDL_WINDOWPOS_CENTERED : mPosition.x);
        int y = (mPosition.y == WINDOWPOS_AUTO ? SDL_WINDOWPOS_CENTERED : mPosition.y);

        switch(mDisplayMode)
        {
            case EDisplayMode::Windowed:
                mHandle = SDL_CreateWindow(mTitle.data(), x, y, mWindowedSize.Width, mWindowedSize.Height, SDL_WINDOW_VULKAN);
                break;
            case EDisplayMode::WindowedResizable:
                mHandle = SDL_CreateWindow(mTitle.data(), x, y, mWindowedSize.Width, mWindowedSize.Height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
                break;
            case EDisplayMode::FullscreenHardware:
                mHandle = SDL_CreateWindow(mTitle.data(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mFullScreenSize.Width, mFullScreenSize.Height,
                                            SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN);
                break;
            case EDisplayMode::FullscreenWindowed:
                mHandle = SDL_CreateWindow(mTitle.data(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mFullScreenSize.Width, mFullScreenSize.Height,
                                            SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN_DESKTOP);
                break;
        }
    }

    inline void Window::Destroy()
    {
        SDL_DestroyWindow(mHandle);
        mHandle = nullptr;
    }
    uint32_t Window::SDLId() const
    {
        if(Exists())
        {
            return SDL_GetWindowID(mHandle);
        }
        return 0;
    }

    VkSurfaceKHR Window::GetSurfaceKHR(const VkInstance &instance) const
    {
        if (!Exists())
        {
            return nullptr;
        }
        VkSurfaceKHR surface;
        SDL_Vulkan_CreateSurface(mHandle, instance, &surface);
        return surface;
    }

    std::vector<const char *> Window::GetVkSurfaceExtensions() const
    {
        if (!Exists())
        {
            return std::vector<const char *>();
        }
        uint32_t count = 0;
        SDL_Vulkan_GetInstanceExtensions(mHandle, &count, nullptr);
        std::vector<const char *> extensions(count);
        SDL_Vulkan_GetInstanceExtensions(mHandle, &count, extensions.data());
        return extensions;
    }

    void Window::Title(const std::string &title)
    {
        mTitle = title;
        if (Exists())
        {
            SDL_SetWindowTitle(mHandle, mTitle.data());
        }
    }
    void Window::Size(Extent2D size)
    {
        mWindowedSize = size;
        if (Exists() && mDisplayMode <= EDisplayMode::WindowedResizable)
        {
            SDL_SetWindowSize(mHandle, mWindowedSize.Width, mWindowedSize.Height);
        }
    }
}
// namespace hsk
