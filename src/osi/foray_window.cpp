#include "foray_window.hpp"

#include "../foray_exception.hpp"
#include "foray_osi.hpp"

namespace foray::osi {

    Window::Window()
        : mHandle(nullptr), mTitle(), mDisplayMode(EDisplayMode::WindowedResizable), mDisplayId(), mFullScreenSize{0, 0}, mWindowedSize{1280, 720}, mPosition{WINDOWPOS_AUTO, WINDOWPOS_AUTO}
    {
        sWindows.push_back(this);
    }

    Window::~Window()
    {
        Destroy();
        for(auto iter = sWindows.begin(); iter != sWindows.end(); ++iter)
        {
            if((*iter) == this)
            {
                sWindows.erase(iter);
                break;
            }
        }
    }

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
            mFullScreenSize = VkExtent2D{(uint32_t)displayMode.w, (uint32_t)displayMode.h};

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

    void Window::Position(VkOffset2D pos)
    {
        mPosition = pos;
        if(Exists() && mDisplayMode <= EDisplayMode::WindowedResizable)
        {
            int x = (mPosition.x == WINDOWPOS_AUTO ? SDL_WINDOWPOS_CENTERED : mPosition.x);
            int y = (mPosition.y == WINDOWPOS_AUTO ? SDL_WINDOWPOS_CENTERED : mPosition.y);
            SDL_SetWindowPosition(mHandle, x, y);
        }
    }

    void Window::Create()
    {
        if(Exists())
        {
            return;
        }
        mOwningThreadID = SDL_ThreadID();

        SDL_DisplayMode displayMode;
        SDL_GetDisplayMode(mDisplayId, 0, &displayMode);
        mFullScreenSize = VkExtent2D{(uint32_t)displayMode.w, (uint32_t)displayMode.h};
        if(mWindowedSize.width * mWindowedSize.height == 0U)
        {
            mWindowedSize = VkExtent2D{1280, 720};
        }
        if(mTitle.length() == 0)
        {
            mTitle = "Foray";
        }
        int x = (mPosition.x == WINDOWPOS_AUTO ? SDL_WINDOWPOS_CENTERED : mPosition.x);
        int y = (mPosition.y == WINDOWPOS_AUTO ? SDL_WINDOWPOS_CENTERED : mPosition.y);

        switch(mDisplayMode)
        {
            case EDisplayMode::Windowed:
                mHandle = SDL_CreateWindow(mTitle.data(), x, y, mWindowedSize.width, mWindowedSize.height, SDL_WINDOW_VULKAN);
                break;
            case EDisplayMode::WindowedResizable:
                mHandle = SDL_CreateWindow(mTitle.data(), x, y, mWindowedSize.width, mWindowedSize.height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
                break;
            case EDisplayMode::FullscreenHardware:
                mHandle = SDL_CreateWindow(mTitle.data(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mFullScreenSize.width, mFullScreenSize.height,
                                           SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN);
                break;
            case EDisplayMode::FullscreenWindowed:
                mHandle = SDL_CreateWindow(mTitle.data(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mFullScreenSize.width, mFullScreenSize.height,
                                           SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN_DESKTOP);
                break;
        }
        mId = SDL_GetWindowID(mHandle);
    }

    inline void Window::Destroy()
    {
        mId             = 0;
        mOwningThreadID = 0;
        if(!Exists())
        {
            return;
        }
        SDL_DestroyWindow(mHandle);
        mHandle  = nullptr;
        mSurface = nullptr;
    }
    uint32_t Window::SDLId() const
    {
        if(Exists())
        {
            return mId;
        }
        return 0;
    }

    VkSurfaceKHR Window::GetOrCreateSurfaceKHR(VkInstance instance)
    {
        if(!Exists())
        {
            return nullptr;
        }
        if(!!mSurface)
        {
            return mSurface;
        }
        SDL_Vulkan_CreateSurface(mHandle, instance, &mSurface);
        return mSurface;
    }

    std::vector<const char*> Window::GetVkSurfaceExtensions() const
    {
        if(!Exists())
        {
            return std::vector<const char*>();
        }
        uint32_t count = 0;
        SDL_Vulkan_GetInstanceExtensions(mHandle, &count, nullptr);
        std::vector<const char*> extensions(count);
        SDL_Vulkan_GetInstanceExtensions(mHandle, &count, extensions.data());
        return extensions;
    }

    void Window::Title(const std::string& title)
    {
        mTitle = title;
        if(Exists())
        {
            SDL_SetWindowTitle(mHandle, mTitle.data());
        }
    }
    void Window::Size(VkExtent2D size)
    {
        mWindowedSize = size;
        if(Exists() && mDisplayMode <= EDisplayMode::WindowedResizable)
        {
            SDL_SetWindowSize(mHandle, mWindowedSize.width, mWindowedSize.height);
        }
    }

    void Window::assertThreadIsOwner()
    {
        auto threadId = SDL_ThreadID();
        if(threadId != mOwningThreadID)
        {
            Exception::Throw("Attempted altering a window from a thread which is not the owner!");
        }
    }

    std::vector<Window*>& Window::Windows()
    {
        return sWindows;
    }

    Window* Window::FindBySDLId(uint32_t id)
    {
        Window* out;

        for(auto window : sWindows)
        {
            if(window->SDLId() == id)
            {
                out = window;
                break;
            }
        }
        return out;
    }

    void Window::HandleEvent(const Event* event)
    {
        auto resized = dynamic_cast<const EventWindowResized*>(event);
        auto closed  = dynamic_cast<const EventWindowCloseRequested*>(event);
        if(resized)
        {
            HandleEvent_Resized(resized);
        }
        if(closed)
        {
            HandleEvent_Closed(closed);
        }
    }

    void Window::HandleEvent_Resized(const EventWindowResized* event)
    {
        if(mDisplayMode <= EDisplayMode::WindowedResizable)
        {
            mWindowedSize = event->Current;
        }
        else
        {
            mFullScreenSize = event->Current;
        }
    }

    void Window::HandleEvent_Closed(const EventWindowCloseRequested* event)
    {
        Destroy();
    }
}  // namespace foray::osi
// namespace foray
