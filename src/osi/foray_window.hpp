#pragma once
#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"
#include "foray_helpers.hpp"
#include "foray_osi_declares.hpp"
#include "../foray_event.hpp"
#include <memory>
#include <sdl2/SDL.h>
#include <sdl2/SDL_vulkan.h>
#include <string>
#include <vector>

namespace foray::osi {
    /// @brief Window class. Provides access to common properties of operating system level windows
    class Window
    {
      public:
        static const int32_t WINDOWPOS_AUTO = INT32_MAX;

        static std::vector<Window*>& Windows();

        static Window* FindBySDLId(uint32_t id);

        Window();

        Window(const Window& other)         = delete;
        Window(const Window&& other)        = delete;
        void operator=(const Window& other) = delete;
        virtual ~Window();

        /// @brief Check if the window exists on an OS level
        /// @return true if window exists
        inline bool Exists() const { return mHandle != nullptr; }
        /// @brief Creates the OS level window based on the parameters of the window object
        virtual void Create();
        /// @brief Destroys the OS side of the window
        virtual void Destroy();

        /// @brief (Getter) WindowPtr Title
        inline const std::string& Title() const { return mTitle; };
        /// @brief (Setter) WindowPtr Title
        /// @param title utf-8 encoded null terminated source to copy the updated title from
        void Title(const std::string& title);
        /// @brief (Getter) WindowPtr Size
        /// @return Hardware-Level size in Pixels
        inline VkExtent2D Size() const { return (mDisplayMode > EDisplayMode::WindowedResizable ? mFullScreenSize : mWindowedSize); };
        /// @brief (Setter) WindowPtr Size. Only works in windowed display modes.
        /// @param size Hardware-Level size in Pixels
        void Size(const VkExtent2D size);
        /// @brief (Getter) Window Position
        inline VkOffset2D Position() const { return mPosition; }
        /// @brief (Setter) Window Position
        void Position(const VkOffset2D pos);
        /// @brief (Setter) Display Mode
        void DisplayMode(EDisplayMode mode, bool preserveSize = false);
        /// @brief (Getter) Display Mode
        inline EDisplayMode DisplayMode() const { return mDisplayMode; };

        SDL_Window* GetSdlWindowHandle() const { return mHandle; }

        virtual uint32_t SDLId() const;

        virtual VkSurfaceKHR             GetOrCreateSurfaceKHR(VkInstance instance);
        virtual std::vector<const char*> GetVkSurfaceExtensions() const;

      protected:
        inline static std::vector<Window*> sWindows = std::vector<Window*>();

        SDL_Window*  mHandle  = nullptr;
        VkSurfaceKHR mSurface = nullptr;
        uint32_t     mId;

        std::string  mTitle;
        EDisplayMode mDisplayMode;
        int32_t      mDisplayId;
        VkExtent2D   mFullScreenSize;
        VkExtent2D   mWindowedSize;
        VkOffset2D   mPosition;

        SDL_threadID mOwningThreadID;

        virtual void HandleEvent_Resized(const EventWindowResized* event);
        virtual void HandleEvent_Closed(const EventWindowCloseRequested* event);

        virtual void assertThreadIsOwner();
    };

}  // namespace foray