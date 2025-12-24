#pragma once

#define NOMINMAX
#include "Core/Base.h"
#include <taskflow/taskflow.hpp>
#include <nvrhi/utils.h>
#include <magic_enum/magic_enum.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_EXT_INLINE_NAMESPACE
#define GLM_GTX_INLINE_NAMESPACE
#define GLM_GTC_INLINE_NAMESPACE
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/extra.hpp>

namespace Math = glm;

#include <bitset>
#include <filesystem>
#include <string>
#include <span>

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::size_t;

namespace Core {

    //////////////////////////////////////////////////////////////////////////
    // Basic
    //////////////////////////////////////////////////////////////////////////

#ifdef CORE_ENABLE_LOGGING

    namespace Log {

        CORE_API void Init(const std::filesystem::path& client);
        CORE_API void Shutdown();

        CORE_API void CoreTrace(const char* s);
        CORE_API void CoreInfo(const char* s);
        CORE_API void CoreWarn(const char* s);
        CORE_API void CoreError(const char* s);
        CORE_API void CoreCritical(const char* s);

        CORE_API void ClientTrace(const char* s);
        CORE_API void ClientInfo(const char* s);
        CORE_API void ClientWarn(const char* s);
        CORE_API void ClientError(const char* s);
        CORE_API void ClientCritical(const char* s);
    }

#endif

    template <typename EnumType>
    constexpr bool HasFlags(EnumType value, EnumType group)
    {
        using UnderlyingType = typename std::underlying_type<EnumType>::type;
        return (static_cast<UnderlyingType>(value) & static_cast<UnderlyingType>(group)) != 0;
    }

    template <typename... Args>
    constexpr size_t Hash(const Args&... args)
    {
        size_t seed = 0;
        (..., (seed ^= std::hash<std::decay_t<Args>>{}(args)+0x9e3779b9 + (seed << 6) + (seed >> 2)));
        return seed;
    }

    // Aligns 'size' up to the next multiple of 'alignment' (power of two).
    template<typename T>
    constexpr T AlignUp(T size, T alignment)
    {
        static_assert(std::is_integral<T>::value, "AlignUp() requires an integral type");
        CORE_ASSERT(size >= 0, "'size' must be non-negative");
        CORE_ASSERT(alignment != 0 && (alignment & (alignment - 1)) == 0, "Alignment must be a power of two");

        return (size + alignment - 1) & ~(alignment - 1);
    }

    template<typename T>
    using Scope = std::unique_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Scope<T> CreateScope(Args&& ... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    using Ref = std::shared_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Ref<T> CreateRef(Args&& ... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    struct Timestep
    {
        float time;

        inline Timestep(float time = 0.0f) : time(time) {}
        inline operator float() const { return time; }
        inline float Seconds() const { return time; }
        inline float Milliseconds() const { return time * 1000.0f; }
    };

    struct Timer
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> start;

        inline Timer() { Reset(); }
        inline void Reset() { start = std::chrono::high_resolution_clock::now(); }
        inline float ElapsedSeconds() const { return std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - start).count(); }
        inline float ElapsedMilliseconds() const { return (float)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count(); }
        inline float ElapsedMicroseconds() const { return (float)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count(); }
        inline float ElapsedNanoseconds() const { return (float)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count(); }
    };

    struct Random
    {
        static bool Bool()
        {
            static thread_local std::uniform_int_distribution<int> dist(0, 1);
            return dist(GetGenerator()) == 1;
        }

        static int Int()
        {
            static thread_local std::uniform_int_distribution<int> dist(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
            return dist(GetGenerator());
        }

        static int Int(int min, int max)
        {
            std::uniform_int_distribution<int> dist(min, max);
            return dist(GetGenerator());
        }

        // Float ranges (0.0f, 1.0)
        static float Float()
        {
            static thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            return dist(GetGenerator());
        }

        static float Float(float min, float max)
        {
            std::uniform_real_distribution<float> dist(min, max);
            return dist(GetGenerator());
        }

        // Float ranges (0.0f, 1.0)
        static double Double()
        {
            static thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
            return dist(GetGenerator());
        }

        static double Double(double min, double max)
        {
            std::uniform_real_distribution<double> dist(min, max);
            return dist(GetGenerator());
        }

        CORE_API static std::mt19937& GetGenerator()
        {
            static thread_local std::mt19937 generator = CreateSeededGenerator();
            return generator;
        }

    private:
        static std::mt19937 CreateSeededGenerator()
        {
            std::random_device rd;
            std::seed_seq seeds{
                rd(),
                rd(),
                static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count()),
                static_cast<unsigned>(reinterpret_cast<uintptr_t>(&seeds))
            };
            return std::mt19937(seeds);
        }
    };

    // Non-owning raw buffer
    struct Buffer
    {
        uint8_t* data = nullptr;
        uint64_t size = 0;

        Buffer() = default;

        inline Buffer(uint64_t size) { Allocate(size); }
        inline Buffer(const void* data, uint64_t pSize) : data((uint8_t*)data), size(pSize) {}
        Buffer(const Buffer&) = default;

        static Buffer Copy(Buffer other)
        {
            Buffer result(other.size);
            std::memcpy(result.data, other.data, other.size);
            return result;
        }

        inline void Allocate(uint64_t pSize)
        {
            Release();
            data = (uint8_t*)std::malloc(pSize);
            size = pSize;
        }

        inline void Release()
        {
            free(data);
            data = nullptr;
            size = 0;
        }

        template<typename T> T* As() { return (T*)data; }
        operator bool() const { return (bool)data; }
    };

    struct ScopedBuffer
    {
        ScopedBuffer(Buffer buffer) : m_Buffer(buffer) {}
        ScopedBuffer(uint64_t size) : m_Buffer(size) {}
        ~ScopedBuffer() { m_Buffer.Release(); }

        uint8_t* Data() { return m_Buffer.data; }
        uint64_t Size() { return m_Buffer.size; }

        template<typename T> T* As() { return m_Buffer.As<T>(); }
        operator bool() const { return m_Buffer; }
    private:
        Buffer m_Buffer;
    };

    class CORE_API Image
    {
    public:
        Image(const std::filesystem::path& filename, int desiredChannels = 4, bool flipVertically = false);
        Image(Buffer buffer, int desiredChannels = 4, bool flipVertically = false);
        Image(int width, int height, int channels, uint8_t* data);
        ~Image();

        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;

        Image(Image&& other) noexcept;
        Image& operator=(Image&& other) noexcept;


        static bool GetImageInfo(const std::filesystem::path& filePath, int& outWidth, int& outWeight, int& outChannels);
        static bool SaveAsPNG(const std::filesystem::path& filePath, int width, int height, int channels, const void* data, int strideInBytes);
        static bool SaveAsJPG(const std::filesystem::path& filePath, int width, int height, int channels, const void* data, int quality = 90);
        static bool SaveAsBMP(const std::filesystem::path& filePath, int width, int height, int channels, const void* data);

        bool isValid() const { return data != nullptr; }
        int GetWidth() const { return width; }
        int GetHeight() const { return height; }
        int GetChannels() const { return channels; }
        unsigned char* GetData() const { return data; }
        void SetData(uint8_t* data);
        uint8_t* ExtractData();

    private:
        uint8_t* data = nullptr;
        int width = 0;
        int height = 0;
        int channels = 0;
    };

    //////////////////////////////////////////////////////////////////////////
    // Event
    //////////////////////////////////////////////////////////////////////////

    struct CodeStrPair
    {
        uint16_t code;
        std::string_view codeStr;
    };

    using MouseCode = uint16_t;
    namespace MouseKey
    {
        enum Code : MouseCode
        {
            Button0, Button1, Button2, Button3, Button4, Button5, Button6, Button7,

            Count,

            Left = Button0,
            Right = Button1,
            Middle = Button2
        };

        CORE_API constexpr std::string_view ToString(MouseCode code);
        CORE_API constexpr MouseCode FromString(std::string_view code);
        CORE_API constexpr std::span<const CodeStrPair> Map();
    }

    using JoystickCode = uint16_t;
    namespace Joystick
    {
        enum Code : JoystickCode
        {
            Joystick0, Joystick1, Joystick2, Joystick3, Joystick4, Joystick5,
            Joystick6, Joystick7, Joystick8, Joystick9, Joystick10, Joystick11,
            Joystick12, Joystick13, Joystick14, Joystick15,

            Count
        };

        CORE_API constexpr std::string_view ToString(JoystickCode code);
        CORE_API constexpr JoystickCode FromString(std::string_view codeStr);
        CORE_API constexpr std::span<const CodeStrPair> Map();
    }

    using GamepadCode = uint16_t;
    namespace GamepadButton
    {
        enum Code : GamepadCode
        {
            A, B, X, Y,
            LeftBumper, RightBumper, Back,
            Start, Guide, LeftThumb, RightThumb,
            Up, Right, Down, Left,

            Count,

            Cross = A, Circle = B, Square = X, Triangle = Y
        };

        CORE_API constexpr std::string_view ToString(GamepadCode code);
        CORE_API constexpr GamepadCode FromString(std::string_view codeStr);
        CORE_API constexpr std::span<const CodeStrPair> Map();
    }

    using GamepadAxisCode = uint16_t;
    namespace GamepadAxis
    {
        enum Code : GamepadAxisCode
        {
            Left, Right,

            Count
        };

        CORE_API constexpr std::string_view ToString(GamepadAxisCode code);
        CORE_API constexpr GamepadAxisCode FromString(std::string_view codeStr);
        CORE_API constexpr std::span<const CodeStrPair> Map();
    }

    using KeyCode = uint16_t;
    namespace Key
    {
        enum Code : KeyCode
        {
            Space, Apostrophe, Comma, Minus, Period, Slash,
            D0, D1, D2, D3, D4, D5, D6, D7, D8, D9,
            Semicolon, Equal,
            A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
            LeftBracket, Backslash, RightBracket, GraveAccent,
            World1, World2,
            Escape, Enter, Tab, Backspace, Insert, Delete, Right, Left, Down, Up, PageUp, PageDown, Home, End, CapsLock, ScrollLock, NumLock, PrintScreen, Pause,
            F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25,
            KP0, KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9,
            KPDecimal, KPDivide, KPMultiply, KPSubtract, KPAdd, KPEnter, KPEqual,
            LeftShift, LeftControl, LeftAlt, LeftSuper, RightShift, RightControl, RightAlt, RightSuper, Menu,

            Count
        };

        CORE_API constexpr std::string_view ToString(KeyCode code);
        CORE_API constexpr KeyCode FromString(std::string_view code);
        CORE_API constexpr std::span<const CodeStrPair> Map();
    }

    struct Cursor
    {
        enum class Mode : uint8_t
        {
            Normal,     // the regular arrow cursor
            Hidden,     // the cursor to become hidden when it is over a window but still want it to behave normally,
            Disabled    // unlimited mouse movement,This will hide the cursor and lock it to the specified window
        };

        Mode CursorMode;
    };

    enum class EventType
    {
        KeyPressed, KeyReleased, KeyTyped,
        MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled, MouseEnter,
        GamepadButtonPressed, GamepadButtonReleased, GamepadAxisMoved, GamepadConnected,
        WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved, WindowDrop, WindowContentScale, WindowMaximize, WindowMinimized,
        None,
    };

    enum class EventCategory
    {
        Keyboard,
        Mouse,
        Gamepad,
        Window,
        None,
    };

    CORE_API constexpr std::string_view ToString(EventType code);
    CORE_API constexpr EventType FromStringToEventType(std::string_view code);
    CORE_API constexpr std::span<const CodeStrPair> EventTypeMap();
    CORE_API constexpr std::string_view ToString(EventCategory code);
    CORE_API constexpr EventCategory FromStringToEventCategory(std::string_view code);
    CORE_API constexpr std::span<const CodeStrPair> EventCategoryMap();

#define EVENT_CLASS_TYPE(type)  inline static EventType GetStaticType() { return EventType::type; }\
                                inline virtual EventType GetEventType() const override { return GetStaticType(); }\
                                inline virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) inline virtual EventCategory GetCategory() const override { return EventCategory::category; }

    struct Event
    {
        bool handled = false;

        virtual ~Event() = default;
        virtual EventType GetEventType() const = 0;
        virtual EventCategory GetCategory() const = 0;
        virtual const char* GetName() const = 0;
        virtual std::string ToString() const { return GetName(); }
    };

    template<typename T, typename F>
    bool DispatchEvent(Event& event, const F& func)
    {
        if (event.GetEventType() == T::GetStaticType())
        {
            event.handled |= func(static_cast<T&>(event));
            return true;
        }
        return false;
    }

    inline std::ostream& operator<<(std::ostream& os, const Event& e) { return os << e.ToString(); }

    //////////////////////////////////////////////////////////////////////////
    // Application Events
    //////////////////////////////////////////////////////////////////////////

    struct WindowResizeEvent : public Event
    {
        uint32_t width, height;

        WindowResizeEvent(uint32_t pWidth, uint32_t pHeight) : width(pWidth), height(pHeight) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "WindowResizeEvent: " << width << ", " << height;
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowResize)
        EVENT_CLASS_CATEGORY(Window)
    };

    struct WindowCloseEvent : public Event
    {
        WindowCloseEvent() = default;

        EVENT_CLASS_TYPE(WindowClose)
        EVENT_CLASS_CATEGORY(Window)
    };

    struct WindowDropEvent : public Event
    {
        const char** paths;
        int count;

        WindowDropEvent(const char** pPaths, int pPathCount) : paths(pPaths), count(pPathCount) {}

        std::string ToString() const override
        {
            std::stringstream ss;

            ss << "WindowDropEvent: " << paths << "\n";
            for (int i = 0; i < count; i++)
                ss << paths[i] << "\n";

            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowDrop)
        EVENT_CLASS_CATEGORY(Window)
    };

    struct WindowContentScaleEvent : public Event
    {
        float scaleX, scaleY;

        WindowContentScaleEvent(float sx, float sy) : scaleX(sx), scaleY(sy) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "WindowContentScaleEvent: " << scaleX << ", " << scaleY;
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowContentScale)
        EVENT_CLASS_CATEGORY(Window)
    };

    struct WindowMaximizeEvent : public Event
    {
        bool maximized;

        WindowMaximizeEvent(bool pMaximized) : maximized(pMaximized) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "WindowMaximizeEvent: " << (maximized ? "maximized" : "restored");
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowMaximize)
        EVENT_CLASS_CATEGORY(Window)
    };

    struct WindowMinimizeEvent : public Event
    {
        bool minimized;

        WindowMinimizeEvent(bool minimized) : minimized(minimized) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "WindowMinimizeEvent: " << (minimized ? "true" : "false");
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowMinimized)
        EVENT_CLASS_CATEGORY(Window)
    };

    //////////////////////////////////////////////////////////////////////////
    // Keybord Events
    //////////////////////////////////////////////////////////////////////////

    struct KeyPressedEvent : public Event
    {
        KeyCode keyCode;
        bool isRepeat;

        KeyPressedEvent(const KeyCode pKeycode, bool pIsRepeat = false) : keyCode(pKeycode), isRepeat(pIsRepeat) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "KeyPressedEvent: " << Key::ToString(keyCode) << " (repeat = " << (isRepeat ? "true" : "false") << ")";
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyPressed)
        EVENT_CLASS_CATEGORY(Keyboard)
    };

    struct KeyReleasedEvent : public Event
    {
        KeyCode keyCode;

        KeyReleasedEvent(const KeyCode pKeyCode) : keyCode(pKeyCode) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "KeyReleasedEvent: " << Key::ToString(keyCode);
            return ss.str();
        }

        EVENT_CLASS_CATEGORY(Keyboard)
        EVENT_CLASS_TYPE(KeyReleased)
    };

    struct KeyTypedEvent : public Event
    {
        uint32_t codePoint;

        KeyTypedEvent(const uint32_t pCodePoint) : codePoint(pCodePoint) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "KeyTypedEvent: " << static_cast<char>(codePoint);
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyTyped)
        EVENT_CLASS_CATEGORY(Keyboard)
    };

    //////////////////////////////////////////////////////////////////////////
    // Mouse Events
    //////////////////////////////////////////////////////////////////////////	

    struct MouseMovedEvent : public Event
    {
        float x, y;

        MouseMovedEvent(const float pX, const float pY) : x(pX), y(pY) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseMovedEvent: " << x << ", " << y;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseMoved)
        EVENT_CLASS_CATEGORY(Mouse)
    };

    struct MouseEnterEvent : public Event
    {
        bool entered;

        MouseEnterEvent(const bool pEntered) : entered(pEntered) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseEnterEvent: " << entered;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseEnter)
        EVENT_CLASS_CATEGORY(Mouse)
    };

    struct MouseScrolledEvent : public Event
    {
        float xOffset, yOffset;

        MouseScrolledEvent(const float pXOffset, const float pYOffset) : xOffset(pXOffset), yOffset(pYOffset) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseScrolledEvent: " << xOffset << ", " << yOffset;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseScrolled)
        EVENT_CLASS_CATEGORY(Mouse)
    };

    struct MouseButtonPressedEvent : public Event
    {
        MouseCode button;

        MouseButtonPressedEvent(const MouseCode pButton) : button(pButton) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseButtonPressedEvent: " << MouseKey::ToString(button);
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseButtonPressed)
        EVENT_CLASS_CATEGORY(Mouse)
    };

    struct MouseButtonReleasedEvent : public Event
    {
        MouseCode button;

        MouseButtonReleasedEvent(const MouseCode pButton) : button(pButton) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseButtonReleasedEvent: " << MouseKey::ToString(button);
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseButtonReleased)
        EVENT_CLASS_CATEGORY(Mouse)
    };

    //////////////////////////////////////////////////////////////////////////
    // Gamepad Events
    //////////////////////////////////////////////////////////////////////////

    struct GamepadAxisMovedEvent : public Event
    {
        JoystickCode joystickId = 0;
        GamepadAxisCode axisCode;
        float x, y;

        GamepadAxisMovedEvent(JoystickCode joystickCode, GamepadAxisCode axisCode, const float pX, const float pY) : joystickId(joystickCode), axisCode(axisCode), x(pX), y(pY) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "GamepadAxisMovedEvent: Joystick : " << joystickId << ", value : " << x << "," << y;
            return ss.str();
        }

        EVENT_CLASS_TYPE(GamepadAxisMoved)
        EVENT_CLASS_CATEGORY(Gamepad)
    };

    struct GamepadButtonPressedEvent : public Event
    {
        JoystickCode joystickCode;
        GamepadCode button;

        GamepadButtonPressedEvent(JoystickCode pJoystickCode, const GamepadCode pButton) : joystickCode(pJoystickCode), button(pButton) {}

        virtual std::string ToString() const override
        {
            std::stringstream ss;
            ss << "GamepadButtonPressedEvent: Joystick : " << joystickCode << ", button : " << button;
            return ss.str();
        }

        EVENT_CLASS_TYPE(GamepadButtonPressed)
        EVENT_CLASS_CATEGORY(Gamepad)
    };

    struct GamepadButtonReleasedEvent : public Event
    {
        JoystickCode joystickCode;
        GamepadCode button;

        GamepadButtonReleasedEvent(JoystickCode joystickCode, const GamepadCode button) : joystickCode(joystickCode), button(button) {}

        virtual std::string ToString() const override
        {
            std::stringstream ss;
            ss << "GamepadButtonReleasedEvent: Joystick : " << joystickCode << ", button : " << GamepadButton::ToString(button);
            return ss.str();
        }

        EVENT_CLASS_TYPE(GamepadButtonReleased)
        EVENT_CLASS_CATEGORY(Gamepad)
    };

    struct GamepadConnectedEvent : public Event
    {
        JoystickCode joystickCode;
        bool connected;

        GamepadConnectedEvent(JoystickCode joystickCode, bool connected) : joystickCode(joystickCode), connected(connected) {}

        virtual std::string ToString() const override
        {
            std::stringstream ss;
            ss << "GamepadConnectedEvent: Joystick : " << joystickCode << ", state : " << (connected ? "Connected" : "Disconnected");
            return ss.str();
        }

        EVENT_CLASS_TYPE(GamepadConnected)
        EVENT_CLASS_CATEGORY(Gamepad)
    };

    //////////////////////////////////////////////////////////////////////////
    // Input
    //////////////////////////////////////////////////////////////////////////

    constexpr int c_MaxModifierCount = 4;

    struct KeyBindingDesc
    {
        std::string name;
        std::array<uint16_t, c_MaxModifierCount> modifiers;
        uint16_t code;
        EventType eventType;
        EventCategory eventCategory;
        std::string shortCut;
    };

    namespace Input {

        CORE_API bool IsKeyPressed(KeyCode key);
        CORE_API bool IsKeyReleased(KeyCode key);
        CORE_API bool IsKeyDown(KeyCode key);
        CORE_API bool IsKeyUp(KeyCode key);

        CORE_API bool IsMouseButtonPressed(MouseCode button);
        CORE_API bool IsMouseButtonReleased(MouseCode button);
        CORE_API bool IsMouseButtonDown(MouseCode button);
        CORE_API bool IsMouseButtonUp(MouseCode button);
        CORE_API std::pair<float, float> GetMousePosition();
        CORE_API float GetMouseX();
        CORE_API float GetMouseY();

        CORE_API bool IsGamepadButtonPressed(JoystickCode code, GamepadCode key);
        CORE_API bool IsGamepadButtonReleased(JoystickCode code, GamepadCode key);
        CORE_API bool IsGamepadButtonDown(JoystickCode code, GamepadCode key);
        CORE_API bool IsGamepadButtonUp(JoystickCode code, GamepadCode key);
        CORE_API std::pair<float, float> GetGamepadLeftAxis(JoystickCode code);
        CORE_API std::pair<float, float> GetGamepadRightAxis(JoystickCode code);
        CORE_API void SetDeadZoon(float value);

        CORE_API void SetCursorMode(Cursor::Mode mode);
        CORE_API Cursor::Mode GetCursorMode();

        CORE_API bool Triggered(const std::string_view& name);
        CORE_API void BlockEventsUntilNextFrame();
        CORE_API bool IsEventsBlocked();
        CORE_API bool RegisterKeyBinding(const KeyBindingDesc& name);
        CORE_API std::map<uint64_t, KeyBindingDesc>& GetKeyBindings();
        CORE_API std::string_view GetShortCut(std::string_view name);
        CORE_API void SerializeKeyBindings(const std::filesystem::path& filePath);
        CORE_API bool DeserializeKeyBindings(const std::filesystem::path& filePath);
    }

    //////////////////////////////////////////////////////////////////////////
    // Window
    //////////////////////////////////////////////////////////////////////////

    struct SwapChainDesc
    {
        nvrhi::Format swapChainFormat = nvrhi::Format::RGBA8_UNORM;
        uint32_t refreshRate = 0;
        uint32_t swapChainBufferCount = 3;
        uint32_t swapChainSampleCount = 1;
        uint32_t swapChainSampleQuality = 0;
        uint32_t maxFramesInFlight = 2;
        uint32_t backBufferWidth = 0;
        uint32_t backBufferHeight = 0;
        bool allowModeSwitch = true;
        bool vsync = true;

#if NVRHI_HAS_D3D11 || NVRHI_HAS_D3D12
        uint32_t swapChainUsage = 0x00000010UL/* DXGI_USAGE_SHADER_INPUT */ | 0x00000020UL /* DXGI_USAGE_RENDER_TARGET_OUTPUT */;
#endif
    };

    struct WindowDesc
    {
        std::string title = "Hydra Engine";
        std::string iconFilePath;
        uint32_t width = 0, height = 0;
        int minWidth = -1, minHeight = -1;
        int maxWidth = -1, maxHeight = -1;
        float sizeRatio = 0.7f;				    // Percentage of screen size to use when width/height is 0
        bool resizeable = true;
        bool customTitlebar = false;
        bool decorated = true;
        bool centered = true;
        bool fullScreen = false;
        bool maximized = false;
        bool perMonitorDPIAware = true;
        bool scaleToMonitor = true;
        bool startVisible = true;
        bool setCallbacks = true;

        SwapChainDesc swapChainDesc;
    };

    // internal
    struct InputState
    {
        InputState()
        {
            keyDownPrevFrame.set();
            keyUpPrevFrame.set();
            mouseButtonDownPrevFrame.set();
            mouseButtonUpPrevFrame.set();

            for (auto& gamepad : gamepadButtonDownPrevFrame) gamepad.set();
            for (auto& gamepad : gamepadButtonUpPrevFrame) gamepad.set();
        }

        Cursor cursor;

        std::bitset<Key::Count> keyDownPrevFrame;
        std::bitset<Key::Count> keyUpPrevFrame;

        std::bitset<MouseKey::Count> mouseButtonDownPrevFrame;
        std::bitset<MouseKey::Count> mouseButtonUpPrevFrame;

        std::bitset<GamepadButton::Count> gamepadButtonDownPrevFrame[Joystick::Count];
        std::bitset<GamepadButton::Count> gamepadButtonUpPrevFrame[Joystick::Count];

        std::bitset<GamepadButton::Count> gamepadEventButtonDownPrevFrame[Joystick::Count];
        std::bitset<GamepadButton::Count> gamepadEventButtonUpPrevFrame[Joystick::Count];

        float deadZoon = 0.1f;
    };

    struct SwapChain
    {
        SwapChainDesc desc;
        void* windowHandle = nullptr;
        std::vector<nvrhi::FramebufferHandle> swapChainFramebuffers;
        nvrhi::DeviceHandle nvrhiDevice;
        bool isVSync = false;

        virtual ~SwapChain() = default;

        CORE_API nvrhi::IFramebuffer* GetCurrentFramebuffer();
        CORE_API nvrhi::IFramebuffer* GetFramebuffer(uint32_t index);
        CORE_API void UpdateSize();

        CORE_API void ResetBackBuffers();
        CORE_API void ResizeBackBuffers();

        virtual nvrhi::ITexture* GetCurrentBackBuffer() = 0;
        virtual nvrhi::ITexture* GetBackBuffer(uint32_t index) = 0;
        virtual uint32_t GetCurrentBackBufferIndex() = 0;
        virtual uint32_t GetBackBufferCount() = 0;
        virtual void ResizeSwapChain(uint32_t width, uint32_t height) = 0;
        virtual bool Present() = 0;
        virtual bool BeginFrame() = 0;
    };

    using WindowEventCallback = std::function<void(Event&)>;

    struct Window
    {
        void* handle = nullptr;
        WindowDesc desc;
        WindowEventCallback eventCallback = 0;
        InputState inputData;
        bool isTitleBarHit = false;
        int prevPosX = 0, prevPosY = 0, prevWidth = 0, prevHeight = 0;
        SwapChain* swapChain = nullptr;

        CORE_API ~Window();
        CORE_API void Init(const WindowDesc& windowDesc);
        CORE_API void* GetNativeHandle();
        CORE_API void SetTitle(const std::string_view& title);
        CORE_API void Maximize();
        CORE_API void Minimize();
        CORE_API void Restore();
        CORE_API bool IsMaximize();
        CORE_API bool IsMinimized();
        CORE_API bool IsFullScreen();
        CORE_API bool ToggleScreenState();
        CORE_API void Focus();
        CORE_API bool IsFocused();
        CORE_API void Show();
        CORE_API void Hide();
        CORE_API std::pair<float, float> GetWindowContentScale();
        CORE_API void UpdateEvent();
        inline uint32_t GetWidth() const { return desc.width; }
        inline uint32_t GetHeight() const { return desc.height; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Layer
    //////////////////////////////////////////////////////////////////////////

    struct FrameInfo
    {
        Timestep ts;
        nvrhi::IFramebuffer* fb;
    };

    class Layer
    {
    public:
        virtual ~Layer() = default;

        inline virtual void OnAttach() {}
        inline virtual void OnDetach() {}
        inline virtual void OnEvent(Event& event) {}
        inline virtual void OnBegin(const FrameInfo& info) {}
        inline virtual void OnUpdate(const FrameInfo& info) {}
        inline virtual void OnEnd(const FrameInfo& info) {}
    };

    class LayerStack
    {
    public:
        LayerStack() = default;
        CORE_API ~LayerStack();

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);

        void PopLayer(Layer* layer);
        void PopOverlay(Layer* overlay);

        std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
        std::vector<Layer*>::iterator end() { return m_Layers.end(); }
        std::vector<Layer*>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
        std::vector<Layer*>::reverse_iterator rend() { return m_Layers.rend(); }

        std::vector<Layer*>::const_iterator begin() const { return m_Layers.begin(); }
        std::vector<Layer*>::const_iterator end()	const { return m_Layers.end(); }
        std::vector<Layer*>::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
        std::vector<Layer*>::const_reverse_iterator rend() const { return m_Layers.rend(); }
    private:
        std::vector<Layer*> m_Layers;
        uint32_t m_LayerInsertIndex = 0;
    };

    int Main(int argc, char** argv);
}

//////////////////////////////////////////////////////////////////////////
// RHI
//////////////////////////////////////////////////////////////////////////

namespace RHI {

    consteval uint8_t BackendCount()
    {
        uint8_t backendCount = 0;
#if NVRHI_HAS_D3D11
        backendCount++;
#endif
#if NVRHI_HAS_D3D12
        backendCount++;
#endif
#if NVRHI_HAS_VULKAN
        backendCount++;
#endif
        return backendCount;
    }

    struct DeviceInstanceDesc
    {
        bool enableDebugRuntime = false;
        bool enableWarningsAsErrors = false;
        bool enableGPUValidation = false; // DX12 only 
        bool headlessDevice = false;
        bool logBufferLifetime = false;
        bool enableHeapDirectlyIndexed = false; // Allows ResourceDescriptorHeap on DX12

#if NVRHI_HAS_VULKAN
        std::string vulkanLibraryName;
        std::vector<std::string> requiredVulkanInstanceExtensions;
        std::vector<std::string> requiredVulkanLayers;
        std::vector<std::string> optionalVulkanInstanceExtensions;
        std::vector<std::string> optionalVulkanLayers;
#endif
    };

    struct DeviceDesc : public DeviceInstanceDesc
    {
        std::array<nvrhi::GraphicsAPI, BackendCount()> api;

        bool enableNvrhiValidationLayer = false;
        bool enableRayTracingExtensions = false;
        bool enableComputeQueue = false;
        bool enableCopyQueue = false;
        int adapterIndex = -1;

#if NVRHI_HAS_D3D11 || NVRHI_HAS_D3D12
        uint32_t featureLevel = 0xb100 /* D3D_FEATURE_LEVEL_11_1 */;
#endif

#if NVRHI_HAS_VULKAN
        std::vector<std::string> requiredVulkanDeviceExtensions;
        std::vector<std::string> optionalVulkanDeviceExtensions;
        std::vector<size_t> ignoredVulkanValidationMessageLocations;
        void* physicalDeviceFeatures2Extensions = nullptr;
#endif

        inline DeviceDesc() { api.fill(nvrhi::GraphicsAPI(-1)); }
    };

    struct AdapterInfo
    {
        using UUID = std::array<uint8_t, 16>;
        using LUID = std::array<uint8_t, 8>;

        std::string name;
        uint32_t vendorID = 0;
        uint32_t deviceID = 0;
        uint64_t dedicatedVideoMemory = 0;

        std::optional<UUID> uuid;
        std::optional<LUID> luid;

        //void* adapter = nullptr;
    };

    struct DefaultMessageCallback : public nvrhi::IMessageCallback
    {
        static DefaultMessageCallback& GetInstance();

        void message(nvrhi::MessageSeverity severity, const char* messageText) override;
    };

    struct DeviceManager
    {
        DeviceDesc desc;
        bool isNvidia = false;
        bool instanceCreated = false;

        CORE_API virtual ~DeviceManager() = default;
        CORE_API bool CreateDevice(const DeviceDesc& desc);
        CORE_API bool CreateInstance(const DeviceInstanceDesc& desc);
        virtual Core::SwapChain* CreateSwapChain(const Core::SwapChainDesc& swapChainDesc, void* windowHandle) = 0;
        virtual bool EnumerateAdapters(std::vector<AdapterInfo>& outAdapters) = 0;
        virtual nvrhi::IDevice* GetDevice() const = 0;
        virtual std::string_view GetRendererString() const = 0;
        virtual bool CreateInstanceInternal() = 0;
        virtual bool CreateDevice() = 0;
        virtual void ReportLiveObjects() {}

#if NVRHI_HAS_VULKAN
        virtual bool IsVulkanInstanceExtensionEnabled(const char* extensionName) const { return false; }
        virtual bool IsVulkanDeviceExtensionEnabled(const char* extensionName) const { return false; }
        virtual bool IsVulkanLayerEnabled(const char* layerName) const { return false; }
        virtual void GetEnabledVulkanInstanceExtensions(std::vector<std::string>& extensions) const {}
        virtual void GetEnabledVulkanDeviceExtensions(std::vector<std::string>& extensions) const {}
        virtual void GetEnabledVulkanLayers(std::vector<std::string>& layers) const {}
#endif
    };

    struct DeviceContext
    {
        std::vector<DeviceManager*> managers;

        CORE_API ~DeviceContext();
    };

    CORE_API DeviceManager* CreateDeviceManager(const DeviceDesc& desc);
    CORE_API DeviceManager* GetDeviceManager(uint32_t index = 0);
    CORE_API nvrhi::DeviceHandle GetDevice(uint32_t index = 0);
    CORE_API void TryCreateDefaultDevice();

#if NVRHI_HAS_D3D11
    CORE_API DeviceManager* CreateD3D11();
#endif
#if NVRHI_HAS_D3D12
    CORE_API DeviceManager* CreateD3D12();
#endif
#if NVRHI_HAS_VULKAN

    CORE_API DeviceManager* CreateVULKAN();
#endif

    struct StaticShader
    {
        Core::Buffer dxbc;
        Core::Buffer dxil;
        Core::Buffer spirv;
    };

    struct ShaderMacro
    {
        std::string_view name;
        std::string_view definition;
    };

    CORE_API nvrhi::ShaderHandle CreateStaticShader(nvrhi::IDevice* device, StaticShader staticShader, const std::vector<ShaderMacro>* pDefines, const nvrhi::ShaderDesc& desc);
    CORE_API nvrhi::ShaderLibraryHandle CreateShaderLibrary(nvrhi::IDevice* device, StaticShader staticShader, const std::vector<ShaderMacro>* pDefines);
}

//////////////////////////////////////////////////////////////////////////
// Modules
//////////////////////////////////////////////////////////////////////////

namespace Modules {

    // Use these callbacks in your module:
    // EXPORT void OnModuleLoaded() {}
    // EXPORT void OnModuleShutdown() {}

    struct CORE_API SharedLib
    {
        SharedLib(const SharedLib&) = delete;
        SharedLib& operator=(const SharedLib&) = delete;

        SharedLib(SharedLib&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
        SharedLib& operator=(SharedLib&& other) noexcept { if (this != &other) std::swap(handle, other.handle); return *this; }

        SharedLib(const std::filesystem::path& filePath, bool decorations = false);
        ~SharedLib();

        bool IsLoaded();
        bool HasSymbol(const std::string_view& symbol) const noexcept;
        void* GetSymbol(const std::string_view& symbolName) const;

        template<typename T>
        T* Symbol(const std::string_view& symbolName) const
        {
            return reinterpret_cast<T*>(GetSymbol(symbolName));
        }

        template<typename T>
        T* Function(const std::string_view& symbolName) const
        {
#           if (defined(__GNUC__) && __GNUC__ >= 8)
#               pragma GCC diagnostic push
#               pragma GCC diagnostic ignored "-Wcast-function-type"
#           endif
            return reinterpret_cast<T*>(GetSymbol(symbolName.data()));
#           if (defined(__GNUC__) && __GNUC__ >= 8)
#               pragma GCC diagnostic pop
#           endif
        }

        // Internals
        static void* Open(const char* path) noexcept;
        static void* GetSymbolAddress(void* handle, const char* name) noexcept;
        static void Close(void* handle) noexcept;
        static std::string GetError() noexcept;

        void* handle = nullptr;
    };

    using ModuleHandle = uint64_t;

    struct ModuleData
    {
        std::string name;
        SharedLib lib;

        uint32_t loadOrder;
        inline static uint32_t s_CurrentLoadOrder = 0;

        ModuleData() = delete;
        ModuleData(const std::filesystem::path& filePath) : name(filePath.stem().string()), lib(filePath), loadOrder(s_CurrentLoadOrder++) {}
    };

    struct ModulesContext
    {
        std::unordered_map<ModuleHandle, Core::Ref<ModuleData>> modules;

        CORE_API ~ModulesContext();
    };

    CORE_API bool LoadModule(const std::filesystem::path& filePath);
    CORE_API bool IsModuleLoaded(ModuleHandle handle);
    CORE_API bool UnloadModule(ModuleHandle handle);
    CORE_API Core::Ref<ModuleData> GetModuleData(ModuleHandle handle);
}

//////////////////////////////////////////////////////////////////////////
// Plugin
//////////////////////////////////////////////////////////////////////////

namespace Plugins {

    inline constexpr const char* c_PluginDescriptorExtension = ".hplugin";

    using PluginHandle = uint64_t;

    struct PluginDesc
    {
        std::string name;
        std::string description;
        std::string URL;
        bool reloadable = false;
        bool enabledByDefault = false;
        std::vector<std::string> modules;	// the base name of module without extension
        std::vector<std::string> plugins;	// Plugins used by this plugin 
    };

    struct Plugin
    {
        PluginDesc desc;
        std::filesystem::path descFilePath;
        bool enabled = false;

        Plugin(PluginDesc pDesc) : desc(pDesc) {}

        std::filesystem::path BaseDirectory() const { return descFilePath.parent_path(); }
        std::filesystem::path BinariesDirectory() const { return BaseDirectory() / "Binaries"; }
        std::filesystem::path AssetsDirectory() const { return BaseDirectory() / "Assets"; }
        std::filesystem::path SourceDirectory() const { return BaseDirectory() / "Source"; }
    };

    struct PluginContext
    {
        std::unordered_map<PluginHandle, Core::Ref<Plugin>> plugins;
    };

    CORE_API void LoadPluginsInDirectory(const std::filesystem::path& directory);
    CORE_API void LoadPlugin(const std::filesystem::path& descriptor);
    CORE_API void LoadPlugin(PluginHandle handle);
    CORE_API bool UnloadPlugin(PluginHandle handle);
    CORE_API void ReloadPlugin(PluginHandle handle);
    CORE_API const Core::Ref<Plugin> GetPlugin(PluginHandle handle);
}

//////////////////////////////////////////////////////////////////////////
// Profiler
//////////////////////////////////////////////////////////////////////////

namespace Profiler {

    struct CPURecord
    {
        std::string_view name;
        float lastWrite = 0.0f;
        int depth = 0;
        float delta = 0.0f;
        float time = 0.0f;
        float timeSum = 0.0;
    };

    struct GPURecord
    {
        std::string_view name;
        float lastWrite = 0.0f;
        int depth = 0;
        nvrhi::TimerQueryHandle tq[2];
        int tqIndex = 0;

        float delta = 0.0f;
        float time = 0.0f;
        float timeSum = 0.0;
    };

    struct CPUScope
    {
        std::string_view name;
        float start;
        int index = 0;

        CORE_API CPUScope(std::string_view pName);
        CORE_API ~CPUScope();
    };

    struct GPUScope
    {
        nvrhi::IDevice* device;
        nvrhi::ICommandList* commandList;
        std::string_view name;
        int index = 0;

        CORE_API GPUScope(nvrhi::IDevice* pDevice, nvrhi::ICommandList* pCommandList, std::string_view pName);
        CORE_API ~GPUScope();
    };

    void BeginFrame();
    void EndFrame();
    CORE_API void CPUBegin(std::string_view pName);
    CORE_API void CPUEnd();
    CORE_API void GPUBegin(nvrhi::IDevice* pDevice, nvrhi::ICommandList* pCommandList, std::string_view pName);
    CORE_API void GPUEnd();
}

//////////////////////////////////////////////////////////////////////////
// Jops
//////////////////////////////////////////////////////////////////////////

namespace Jops {

    using Taskflow = tf::Taskflow;
    using Task = tf::Task;
    using Executor = tf::Executor;
    using Future = tf::Future<void>;

    CORE_API std::future<void> SubmitTask(const std::function<void()>& function);
    CORE_API Future RunTaskflow(Taskflow& taskflow);
    CORE_API void WaitForAll();
    CORE_API void SetMainThreadMaxJobsPerFrame(uint32_t max);
    CORE_API void SubmitToMainThread(const std::function<void()>& function);
}

//////////////////////////////////////////////////////////////////////////
// FileSystem
//////////////////////////////////////////////////////////////////////////

namespace FileSystem {

    enum class AppDataType
    {
        Roaming,
        Local
    };

    enum class FileWatcherEvent
    {
        None,
        Added,
        Removed,
        Modified,
        RenamedOldName,
        RenamedNewName,
    };

    struct FileWatcher
    {
        using Callback = std::function<void(const std::filesystem::path& file, FileWatcherEvent event)>;

        CORE_API ~FileWatcher();
        CORE_API void Start(const std::filesystem::path& target, bool watchSubtree, Callback callback);
        CORE_API void Stop();

        // Internals
        std::filesystem::path target;
        std::thread thread;
        bool running = false;
        Callback callback;
        void* dirHandle = nullptr;
        bool watchSubtree = false;
    };

    CORE_API bool Delete(const std::filesystem::path& path);
    CORE_API bool Rename(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
    CORE_API bool Copy(const std::filesystem::path& from, const std::filesystem::path& to, std::filesystem::copy_options options = std::filesystem::copy_options::recursive);
    CORE_API bool Open(const std::filesystem::path& path);
    CORE_API std::vector<uint8_t> ReadBinaryFile(const std::filesystem::path& filePath);
    CORE_API bool ReadBinaryFile(const std::filesystem::path& filePath, Core::Buffer buffer);
    CORE_API std::string ReadTextFile(const std::filesystem::path& filePath);
    CORE_API bool ConvertBinaryToHeader(const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, const std::string& arrayName);
    CORE_API bool GenerateFileWithReplacements(const std::filesystem::path& input, const std::filesystem::path& output, const std::initializer_list<std::pair<std::string_view, std::string_view>>& replacements);
    CORE_API bool ExtractZip(const std::filesystem::path& zipPath, const std::filesystem::path& outputDir);
    CORE_API std::filesystem::path GetAppDataPath(const std::string& appName, AppDataType type = AppDataType::Roaming);
}

//////////////////////////////////////////////////////////////////////////
// Profiler
//////////////////////////////////////////////////////////////////////////

namespace FileDialog {

    CORE_API std::filesystem::path OpenFile(std::initializer_list<std::pair<std::string_view, std::string_view>> filters);
    CORE_API std::filesystem::path SaveFile(std::initializer_list<std::pair<std::string_view, std::string_view>> filters);
    CORE_API std::filesystem::path SelectFolder();
}

//////////////////////////////////////////////////////////////////////////
// OS
//////////////////////////////////////////////////////////////////////////

namespace OS {

    CORE_API void SetEnvVar(const char* var, const char* value);
    CORE_API void RemoveEnvVar(const char* var);
}

//////////////////////////////////////////////////////////////////////////
// Application
//////////////////////////////////////////////////////////////////////////

namespace Application {

    struct ApplicationCommandLineArgs
    {
        char** args = nullptr;
        int count = 0;

        inline const char* operator[](int index) const { CORE_ASSERT(index < count); return args[index]; }
    };

    struct ApplicationDesc
    {
        Core::WindowDesc windowDesc;
        RHI::DeviceDesc deviceDesc;
        ApplicationCommandLineArgs commandLineArgs;
        std::filesystem::path workingDirectory;
        bool createDefaultDevice = true;
        uint32_t workersNumber = std::thread::hardware_concurrency() - 1;
        std::filesystem::path logFile = "Core";
    };

    struct Stats
    {
        float CPUMainTime;
        uint32_t FPS;
    };

    struct ApplicationContext
    {
        ApplicationDesc applicatoinDesc;
        RHI::DeviceContext deviceContext;
        Core::Window mainWindow;

        Core::LayerStack layerStack;
        Modules::ModulesContext modulesContext;
        Plugins::PluginContext pluginContext;

        std::map<uint64_t, Core::KeyBindingDesc> keyBindings;
        bool blockingEventsUntilNextFrame = false;

        // threads
        tf::Executor executor;
        uint32_t mainThreadMaxJobsPerFrame = 1;
        std::queue<std::function<void()>> mainThreadQueue;
        std::mutex mainThreadQueueMutex;

        // profiler
        std::vector<Profiler::CPURecord> cpuProfilerRecords;
        std::stack<Profiler::CPUScope> cpuProfilerStack;
        int cpuProfilerRecordCount = 0;
        int cpuProfilerDepth = 0;
        int cpuProfilerIndex = 0;

        std::vector<Profiler::GPURecord> gpuProfilerRecords;
        std::stack<Profiler::GPUScope> gpuProfilerStack;
        int gpuProfilerRecordCount = 0;
        int gpuProfilerDepth = 0;
        int gpuProfilerIndex = 0;

        Stats appStats;
        bool running = true;
        float lastFrameTime = 0.0f;
        float frameTimestamp = 0.0f;
        float averageFrameTime = 0.0;
        float averageTimeUpdateInterval = 0.5f;
        float frameTimeSum = 0.0;
        int numberOfAccumulatedFrames = 0;

        inline static bool s_ApplicationRunning = true;
        inline static ApplicationContext* s_Instance = nullptr;

        CORE_API ApplicationContext(const ApplicationDesc& desc);
        CORE_API void Run();
    };

    CORE_API void Restart();
    CORE_API void Shutdown();
    CORE_API bool IsApplicationRunning();
    CORE_API void PushLayer(Core::Layer* overlay);
    CORE_API void PushOverlay(Core::Layer* layer);
    CORE_API void PopLayer(Core::Layer* layer);
    CORE_API void PopOverlay(Core::Layer* overlay);
    CORE_API float GetTime();
    CORE_API const Stats& GetStats();
    CORE_API const ApplicationDesc& GetApplicationDesc();
    CORE_API float GetAverageFrameTimeSeconds();
    CORE_API float GetLastFrameTime();
    CORE_API float GetTimestamp();
    CORE_API void SetFrameTimeUpdateInterval(float seconds);
    CORE_API Core::Window& GetWindow();

    CORE_API ApplicationContext& GetAppContext();
    ApplicationContext* CreateApplication(ApplicationCommandLineArgs args);
}
