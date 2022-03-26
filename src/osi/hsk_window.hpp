#pragma once
#include <string>
#include <vector>
#include "../hsk_basics.hpp"
#include "hsk_fwddeclare.hpp"
#include <glm/glm.hpp>
#include <sdl2/SDL.h>
#include <vulkan/vulkan.hpp>

namespace hsk
{
    /// @brief WindowPtr interface class. Provides access to common properties of operating system level windows
    class Window
    {
    public:
        static const int32_t WINDOWPOS_AUTO = INT32_MAX;

    protected:
        SDL_Window *mHandle;

        std::string mTitle;
        EDisplayMode mDisplayMode;
        int32_t mDisplayId;
        Extent2D mFullScreenSize;
        Extent2D mWindowedSize;
        glm::ivec2 mPosition;
        SDL_threadID mOwningThreadID;

    public:
        Window();

        Window(const std::string& title, EDisplayMode displaymode, Extent2D size)
            : mTitle(title), mDisplayMode(displaymode), mDisplayId(), mFullScreenSize{ 0, 0 }, mWindowedSize(size), mPosition{ WINDOWPOS_AUTO, WINDOWPOS_AUTO }, mOwningThreadID()
        {
        }
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
        inline glm::ivec2 Position() const { return mPosition; }
        /// @brief (Setter) Window Position
        void Position(const glm::ivec2 pos);
        /// @brief (Setter) Display Mode
        void DisplayMode(EDisplayMode mode, bool preserveSize = false);
        /// @brief (Getter) Display Mode
        inline EDisplayMode DisplayMode() const { return mDisplayMode; };

        uint32_t SDLId() const;

        VkSurfaceKHR GetSurfaceKHR(const VkInstance &instance) const;
        std::vector<const char *> GetVkSurfaceExtensions() const;
    };

}