#pragma once
#include <string>
#include <vector>
#include "../hsk_basics.hpp"
#include "../hsk_memory.hpp"
#include "hsk_osi_declares.hpp"
#include "hsk_helpers.hpp"
#include <sdl2/SDL.h>
#include <sdl2/SDL_vulkan.h>

namespace hsk
{
    /// @brief Window class. Provides access to common properties of operating system level windows
    class Window
    {
    public:
        using loanptr = loan_ptr<Window>;
        using ptr = std::unique_ptr<Window>;

        static const int32_t WINDOWPOS_AUTO = INT32_MAX;

    public:
        inline static std::vector<Window::loanptr> sWindows = std::vector<Window::loanptr>();

        static std::vector<Window::loanptr>& Windows();

        static Window::loanptr FindBySDLId(uint32_t id);

    protected:
        SDL_Window *mHandle;
        uint32_t mId;

        std::string mTitle;
        EDisplayMode mDisplayMode;
        int32_t mDisplayId;
        Extent2D mFullScreenSize;
        Extent2D mWindowedSize;
        Pos2D mPosition;

        SDL_threadID mOwningThreadID;

    public:
        Window();

        Window(const Window &other) = delete;
        Window(const Window &&other) = delete;
        void operator=(const Window &other) = delete;
        virtual ~Window();

        /// @brief Check if the window exists on an OS level
        /// @return true if window exists
        inline bool Exists() const { return mHandle != nullptr; }
        /// @brief Creates the OS level window based on the parameters of the window object
        void Create();
        /// @brief Destroys the OS side of the window
        void Destroy();

        /// @brief (Getter) WindowPtr Title
        inline const std::string &Title() const { return mTitle; };
        /// @brief (Setter) WindowPtr Title
        /// @param title utf-8 encoded null terminated source to copy the updated title from
        void Title(const std::string &title);
        /// @brief (Getter) WindowPtr Size
        /// @return Hardware-Level size in Pixels
        inline Extent2D Size() const { return (mDisplayMode > EDisplayMode::WindowedResizable ? mFullScreenSize : mWindowedSize); };
        /// @brief (Setter) WindowPtr Size. Only works in windowed display modes.
        /// @param size Hardware-Level size in Pixels
        void Size(const Extent2D size);
        /// @brief (Getter) Window Position
        inline Pos2D Position() const { return mPosition; }
        /// @brief (Setter) Window Position
        void Position(const Pos2D pos);
        /// @brief (Setter) Display Mode
        void DisplayMode(EDisplayMode mode, bool preserveSize = false);
        /// @brief (Getter) Display Mode
        inline EDisplayMode DisplayMode() const { return mDisplayMode; };

        uint32_t SDLId() const;

        VkSurfaceKHR GetSurfaceKHR(const VkInstance &instance) const;
        std::vector<const char *> GetVkSurfaceExtensions() const;

        void HandleEvent(std::shared_ptr<Event> event);

    protected:

        void HandleEvent_Resized(std::shared_ptr<EventWindowResized> event);
        void HandleEvent_Closed(std::shared_ptr<EventWindowCloseRequested> event);

        void assertThreadIsOwner();
    };

}