#include "hsk_fwddeclare.hpp"
#include <string>
#include <vector>
#include <memory>

namespace hsk
{
    class InputDevice
    {
    public:
        using loanptr = const InputDevice*;
        using ptr = std::unique_ptr<InputDevice>;

        enum class EType : uint8_t
        {
            Unknown = 0,
            Mouse,
            Keyboard,
            Joystick
        };

        class Axis
        {
        public:
            int32_t Id;
            std::string Name;

        protected:
            Axis() : Id(-1), Name() {}
            Axis(int32_t id, const std::string &name) : Id(id), Name(name) {}

        public:
            virtual int16_t State() const = 0;
        };

        class Button
        {
        public:
            int32_t Id;
            std::string Name;
            EButton Type;

        protected:
            Button() : Id{}, Name(), Type() {}
            Button(int32_t id, const std::string &name, EButton type) : Id(id), Name(name), Type(type) {}

        public:
            virtual bool State() const = 0;
        };

        using AxisPtr = Axis *;
        using ButtonPtr = Button *;

    protected:
        GUID128 mId;
        std::string mName;
        EType mType;

        std::vector<AxisPtr> mAxes;
        std::vector<ButtonPtr> mButtons;

    public:
        InputDevice() : mId(), mName(), mType(EType::Unknown) {}
        InputDevice(const GUID128 &guid, const std::string &name, EType type) : mId(guid), mName(name), mType(type) {}
        InputDevice(const InputDevice &other) = delete;
        InputDevice(const InputDevice &&other) = delete;
        InputDevice &operator=(const InputDevice &other) = delete;

        const std::vector<AxisPtr> Axes() const { return mAxes; }
        const std::vector<ButtonPtr> Buttons() const { return mButtons; }

        const std::string &Name() const { return mName; }
        EType Type() const { return mType; }

        /// @brief Finds a button object based on the corresponding enum value.
        /// Button objects are valid until the input device is deleted, so storing the object for later use is a good idea.
        /// @return nullptr if not found, a valid button object
        const ButtonPtr FindButton(EButton button) const;

        std::string BuildDebugPrint() const;

        virtual ~InputDevice() {}
    };

}