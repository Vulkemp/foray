#include "hsk_event.hpp"
#include "hsk_inputdevice.hpp"

namespace hsk
{
    class OsManager
    {
    protected:
        /// @brief A collection of all non-standard input devices recognized by this application
        std::vector<InputDevice::ptr> m_InputDevices;

    public:
        /// @brief Mouse input device. Assumed standard and always present
        InputDevice::loanptr m_Mouse;
        /// @brief Keyboard input device. Assumed standard and always present
        InputDevice::loanptr m_Keyboard;

        OsManager() : m_InputDevices(), m_Mouse(), m_Keyboard() {}

    public:
        OsManager(const OsManager &other) = delete;
        OsManager(const OsManager &&other) = delete;
        OsManager &operator=(const OsManager &other) = delete;
        virtual ~OsManager() {}

        /// @brief A collection of all non-standard input devices recognized by this application
        const std::vector<InputDevice::loanptr> &InputDevices() const;

    public:
        void Init();
        void Cleanup();

        Event::ptr PollEvent();
    };
}