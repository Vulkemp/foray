#pragma once
#include "../foray_basics.hpp"
#include <cstring>

namespace foray::osi {
    /// @brief All supported buttons (mouse, controllers, etc.) and keys (keyboard)
    enum class EButton : uint16_t
    {
        Undefined                          = 0,
        Keyboard_A                         = 4,
        Keyboard_B                         = 5,
        Keyboard_C                         = 6,
        Keyboard_D                         = 7,
        Keyboard_E                         = 8,
        Keyboard_F                         = 9,
        Keyboard_G                         = 10,
        Keyboard_H                         = 11,
        Keyboard_I                         = 12,
        Keyboard_J                         = 13,
        Keyboard_K                         = 14,
        Keyboard_L                         = 15,
        Keyboard_M                         = 16,
        Keyboard_N                         = 17,
        Keyboard_O                         = 18,
        Keyboard_P                         = 19,
        Keyboard_Q                         = 20,
        Keyboard_R                         = 21,
        Keyboard_S                         = 22,
        Keyboard_T                         = 23,
        Keyboard_U                         = 24,
        Keyboard_V                         = 25,
        Keyboard_W                         = 26,
        Keyboard_X                         = 27,
        Keyboard_Y                         = 28,
        Keyboard_Z                         = 29,
        Keyboard_1                         = 30,
        Keyboard_2                         = 31,
        Keyboard_3                         = 32,
        Keyboard_4                         = 33,
        Keyboard_5                         = 34,
        Keyboard_6                         = 35,
        Keyboard_7                         = 36,
        Keyboard_8                         = 37,
        Keyboard_9                         = 38,
        Keyboard_0                         = 39,
        Keyboard_Return                    = 40,
        Keyboard_Escape                    = 41,
        Keyboard_Backspace                 = 42,
        Keyboard_Tab                       = 43,
        Keyboard_Space                     = 44,
        Keyboard_Minus                     = 45,
        Keyboard_Equals                    = 46,
        Keyboard_LBracket                  = 47,
        Keyboard_RBracket                  = 48,
        Keyboard_Backslash                 = 49,
        Keyboard_NonUsHash                 = 50,
        Keyboard_Semicolon                 = 51,
        Keyboard_Apostrophe                = 52,
        Keyboard_Grave                     = 53,
        Keyboard_Comma                     = 54,
        Keyboard_Period                    = 55,
        Keyboard_Slash                     = 56,
        Keyboard_Capslock                  = 57,
        Keyboard_F1                        = 58,
        Keyboard_F2                        = 59,
        Keyboard_F3                        = 60,
        Keyboard_F4                        = 61,
        Keyboard_F5                        = 62,
        Keyboard_F6                        = 63,
        Keyboard_F7                        = 64,
        Keyboard_F8                        = 65,
        Keyboard_F9                        = 66,
        Keyboard_F10                       = 67,
        Keyboard_F11                       = 68,
        Keyboard_F12                       = 69,
        Keyboard_PrintScreen               = 70,
        Keyboard_ScrollLock                = 71,
        Keyboard_Pause                     = 72,
        Keyboard_Insert                    = 73,
        Keyboard_Home                      = 74,
        Keyboard_Pageup                    = 75,
        Keyboard_Delete                    = 76,
        Keyboard_End                       = 77,
        Keyboard_PageDown                  = 78,
        Keyboard_Right                     = 79,
        Keyboard_Left                      = 80,
        Keyboard_Down                      = 81,
        Keyboard_Up                        = 82,
        Keyboard_Numlock                   = 83,
        Keyboard_Numpad_Divide             = 84,
        Keyboard_Numpad_Multiply           = 85,
        Keyboard_Numpad_Minus              = 86,
        Keyboard_Numpad_Plus               = 87,
        Keyboard_Numpad_Enter              = 88,
        Keyboard_Numpad_1                  = 89,
        Keyboard_Numpad_2                  = 90,
        Keyboard_Numpad_3                  = 91,
        Keyboard_Numpad_4                  = 92,
        Keyboard_Numpad_5                  = 93,
        Keyboard_Numpad_6                  = 94,
        Keyboard_Numpad_7                  = 95,
        Keyboard_Numpad_8                  = 96,
        Keyboard_Numpad_9                  = 97,
        Keyboard_Numpad_0                  = 98,
        Keyboard_Numpad_Period             = 99,
        Keyboard_NonUSBackslash            = 100,
        Keyboard_Context                   = 101,
        Keyboard_Power                     = 102,
        Keyboard_Numpad_Equals             = 103,
        Keyboard_F13                       = 104,
        Keyboard_F14                       = 105,
        Keyboard_F15                       = 106,
        Keyboard_F16                       = 107,
        Keyboard_F17                       = 108,
        Keyboard_F18                       = 109,
        Keyboard_F19                       = 110,
        Keyboard_F20                       = 111,
        Keyboard_F21                       = 112,
        Keyboard_F22                       = 113,
        Keyboard_F23                       = 114,
        Keyboard_F24                       = 115,
        Keyboard_Execute                   = 116,
        Keyboard_Help                      = 117,
        Keyboard_Menu                      = 118,
        Keyboard_Select                    = 119,
        Keyboard_Stop                      = 120,
        Keyboard_Again                     = 121,
        Keyboard_Undo                      = 122,
        Keyboard_Cut                       = 123,
        Keyboard_Copy                      = 124,
        Keyboard_Paste                     = 125,
        Keyboard_Find                      = 126,
        Keyboard_Mute                      = 127,
        Keyboard_VolumeUp                  = 128,
        Keyboard_VolumeDown                = 129,
        Keyboard_Numpad_Comma              = 133,
        Keyboard_Numpad_EqualsAS400        = 134,
        Keyboard_International1            = 135,
        Keyboard_International2            = 136,
        Keyboard_International3            = 137,
        Keyboard_International4            = 138,
        Keyboard_International5            = 139,
        Keyboard_International6            = 140,
        Keyboard_International7            = 141,
        Keyboard_International8            = 142,
        Keyboard_International9            = 143,
        Keyboard_Lang1                     = 144,
        Keyboard_Lang2                     = 145,
        Keyboard_Lang3                     = 146,
        Keyboard_Lang4                     = 147,
        Keyboard_Lang5                     = 148,
        Keyboard_Lang6                     = 149,
        Keyboard_Lang7                     = 150,
        Keyboard_Lang8                     = 151,
        Keyboard_Lang9                     = 152,
        Keyboard_Alterase                  = 153,
        Keyboard_SysReq                    = 154,
        Keyboard_Cancel                    = 155,
        Keyboard_Clear                     = 156,
        Keyboard_Prior                     = 157,
        Keyboard_Return2                   = 158,
        Keyboard_Separator                 = 159,
        Keyboard_Out                       = 160,
        Keyboard_Oper                      = 161,
        Keyboard_ClearAgain                = 162,
        Keyboard_CRSEL                     = 163,
        Keyboard_EXSEL                     = 164,
        Keyboard_Numpad_Double_0           = 176,
        Keyboard_Numpad_Triple_0           = 177,
        Keyboard_ThousandsSeparator        = 178,
        Keyboard_DecimalSeparator          = 179,
        Keyboard_CurrencyUnit              = 180,
        Keyboard_CurrencySubUnit           = 181,
        Keyboard_Numpad_LParen             = 182,
        Keyboard_Numpad_RParen             = 183,
        Keyboard_Numpad_LBrace             = 184,
        Keyboard_Numpad_RBrace             = 185,
        Keyboard_Numpad_Tab                = 186,
        Keyboard_Numpad_Backspace          = 187,
        Keyboard_Numpad_A                  = 188,
        Keyboard_Numpad_B                  = 189,
        Keyboard_Numpad_C                  = 190,
        Keyboard_Numpad_D                  = 191,
        Keyboard_Numpad_E                  = 192,
        Keyboard_Numpad_F                  = 193,
        Keyboard_Numpad_XOR                = 194,
        Keyboard_Numpad_Power              = 195,
        Keyboard_Numpad_Percent            = 196,
        Keyboard_Numpad_Less               = 197,
        Keyboard_Numpad_Greater            = 198,
        Keyboard_Numpad_Ampersand          = 199,
        Keyboard_Numpad_Double_Ampersand   = 200,
        Keyboard_Numpad_VerticalBar        = 201,
        Keyboard_Numpad_Double_VerticalBar = 202,
        Keyboard_Numpad_COLON              = 203,
        Keyboard_Numpad_HASH               = 204,
        Keyboard_Numpad_SPACE              = 205,
        Keyboard_Numpad_AT                 = 206,
        Keyboard_Numpad_EXCLAM             = 207,
        Keyboard_Numpad_MEMSTORE           = 208,
        Keyboard_Numpad_MEMRECALL          = 209,
        Keyboard_Numpad_MEMCLEAR           = 210,
        Keyboard_Numpad_MEMADD             = 211,
        Keyboard_Numpad_MEMSUBTRACT        = 212,
        Keyboard_Numpad_MEMMULTIPLY        = 213,
        Keyboard_Numpad_MEMDIVIDE          = 214,
        Keyboard_Numpad_PLUSMINUS          = 215,
        Keyboard_Numpad_CLEAR              = 216,
        Keyboard_Numpad_CLEARENTRY         = 217,
        Keyboard_Numpad_BINARY             = 218,
        Keyboard_Numpad_OCTAL              = 219,
        Keyboard_Numpad_DECIMAL            = 220,
        Keyboard_Numpad_HEXADECIMAL        = 221,
        Keyboard_LCtrl                     = 224,
        Keyboard_LShift                    = 225,
        Keyboard_LAlt                      = 226,
        Keyboard_LGui                      = 227,
        Keyboard_RCtrl                     = 228,
        Keyboard_RShift                    = 229,
        Keyboard_RAlt                      = 230,
        Keyboard_RGui                      = 231,
        Keyboard_Mode                      = 257,
        Keyboard_AudioNext                 = 258,
        Keyboard_AudioPrev                 = 259,
        Keyboard_AudioStop                 = 260,
        Keyboard_AudioPlay                 = 261,
        Keyboard_AudioMute                 = 262,
        Keyboard_MediaSelect               = 263,
        Keyboard_WWW                       = 264,
        Keyboard_Mail                      = 265,
        Keyboard_Calculator                = 266,
        Keyboard_Computer                  = 267,
        Keyboard_AC_Search                 = 268,
        Keyboard_AC_Home                   = 269,
        Keyboard_AC_Back                   = 270,
        Keyboard_AC_Forward                = 271,
        Keyboard_AC_Stop                   = 272,
        Keyboard_AC_Refresh                = 273,
        Keyboard_AC_Bookmarks              = 274,
        Keyboard_BrightnessDown            = 275,
        Keyboard_BrightnessUp              = 276,
        Keyboard_DisplaySwitch             = 277,
        Keyboard_Eject                     = 281,
        Keyboard_Sleep                     = 282,
        Keyboard_App1                      = 283,
        Keyboard_App2                      = 284,
        Keyboard_AudioRewind               = 285,
        Keyboard_AudioFastForward          = 286,
        Mouse_Left                         = 287,
        Mouse_Right,
        Mouse_Middle,
        Mouse_X1,
        Mouse_X2,
        JoystickButton_0 = 292,
        JoystickButton_1,
        JoystickButton_2,
        JoystickButton_3,
        JoystickButton_4,
        JoystickButton_5,
        JoystickButton_6,
        JoystickButton_7,
        JoystickButton_8,
        JoystickButton_9,
        JoystickButton_10,
        JoystickButton_11,
        JoystickButton_12,
        JoystickButton_13,
        JoystickButton_14,
        JoystickButton_15,
        JoystickButton_16,
        JoystickButton_17,
        JoystickButton_18,
        JoystickButton_19,
        JoystickButton_20,
        JoystickButton_21,
        JoystickButton_22,
        JoystickButton_23,
        JoystickButton_24,
        JoystickButton_25,
        JoystickButton_26,
        JoystickButton_27,
        JoystickButton_28,
        JoystickButton_29,
        JoystickButton_30,
        JoystickButton_31,
        JoystickButton_32,
        JoystickButton_33,
        JoystickButton_34,
        JoystickButton_35,
        JoystickButton_36,
        JoystickButton_37,
        JoystickButton_38,
        JoystickButton_39,
        JoystickButton_40,
        JoystickButton_41,
        JoystickButton_42,
        JoystickButton_43,
        JoystickButton_44,
        JoystickButton_45,
        JoystickButton_46,
        JoystickButton_47,
        JoystickButton_48,
        JoystickButton_49,
        ENUM_MAX
    };

    enum class EAxis : uint16_t
    {
        JoystickAxis_0 = 0,
        JoystickAxis_1,
        JoystickAxis_2,
        JoystickAxis_3,
        JoystickAxis_4,
        JoystickAxis_5,
        JoystickAxis_6,
        JoystickAxis_7,
        JoystickAxis_8,
        JoystickAxis_9,
        JoystickAxis_10,
        JoystickAxis_11,
        JoystickAxis_12,
        JoystickAxis_13,
        JoystickAxis_14,
        JoystickAxis_15,
        JoystickAxis_16,
        JoystickAxis_17,
        JoystickAxis_18,
        JoystickAxis_19,
        JoystickAxis_20,
        JoystickAxis_21,
        JoystickAxis_22,
        JoystickAxis_23,
        JoystickAxis_24,
        JoystickAxis_25,
        JoystickAxis_26,
        JoystickAxis_27,
        JoystickAxis_28,
        JoystickAxis_29,
        JoystickAxis_30,
        JoystickAxis_31,
        JoystickAxis_32,
        JoystickAxis_33,
        JoystickAxis_34,
        JoystickAxis_35,
        JoystickAxis_36,
        JoystickAxis_37,
        JoystickAxis_38,
        JoystickAxis_39,
        JoystickAxis_40,
        JoystickAxis_41,
        JoystickAxis_42,
        JoystickAxis_43,
        JoystickAxis_44,
        JoystickAxis_45,
        JoystickAxis_46,
        JoystickAxis_47,
        JoystickAxis_48,
        JoystickAxis_49,
        ENUM_MAX
    };

    /// @brief Enum for identification of stateless directional inputs
    enum class EDirectional : uint16_t
    {
        Mouse_Scroll,
        Joystick_Hat0,
        Joystick_Hat1,
        Joystick_Hat2,
        Joystick_Hat3,
        Joystick_Hat4,
        Joystick_Hat5,
        Joystick_Hat6,
        Joystick_Hat7,
        Joystick_Hat8,
        Joystick_Hat9,
    };

    /// @brief Supported modes of window display
    enum class EDisplayMode
    {
        /// @brief An OS window with default borders etc.
        Windowed,
        /// @brief An OS window with default borders etc. and resizing enabled
        WindowedResizable,
        /// @brief Hardwarelevel window (skips desktop window manager)
        FullscreenHardware,
        /// @brief Fullscreen-like application, but run as a regular OS window
        FullscreenWindowed
    };
}  // namespace foray