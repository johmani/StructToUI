#pragma once

//////////////////////////////////////////////////////////////////////////
// PlatformDetection
//////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)
    #if defined(_WIN64)
        #define CORE_PLATFORM_WINDOWS
        constexpr const char* c_SharedLibExtension = ".dll";
        constexpr const char* c_System = "Windows";
        constexpr const char* c_Architecture = "x86_64";
        constexpr const char* c_ExecutableExtension = ".exe";
    #else
        #error "x86 Builds are not supported!"
    #endif
#elif defined(__linux__)
    #define CORE_PLATFORM_LINUX
    constexpr const char* c_SharedLibExtension = ".so";
    constexpr const char* c_System = "Linux";
    constexpr const char* c_Architecture = "x86_64";
    constexpr const char* c_ExecutableExtension = "";
#else
    #error "Unknown platform!"
#endif


//////////////////////////////////////////////////////////////////////////
// Macros
//////////////////////////////////////////////////////////////////////////

#if defined(CORE_AS_SHAREDLIB) 
#   if defined(CORE_BUILD) 
#       if defined(_MSC_VER)
#           define CORE_API __declspec(dllexport)
#       elif defined(__GNUC__)
#           define CORE_API __attribute__((visibility("default")))
#       else
#           define CORE_API
#       endif
#   else 
#       if defined(_MSC_VER)
#           define CORE_API __declspec(dllimport)
#       else
#           define CORE_API
#       endif
#   endif
#else
#  define CORE_API
#endif

#ifdef _MSC_VER
#	define EXPORT extern "C" __declspec(dllexport)
#elif defined(__GNUC__)
#	define EXPORT extern "C" __attribute__((visibility("default")))
#else
#	define EXPORT extern "C"
#endif

#if CORE_DEBUG
    constexpr const char* c_BuildConfig = "Debug";
#   define CORE_ENABLE_ASSERTS
#   define CORE_ENABLE_VERIFY
#	define CORE_ENABLE_LOGGING
#elif CORE_RELEASE
    constexpr const char* c_BuildConfig = "Release";
#   define CORE_ENABLE_VERIFY
#	define CORE_ENABLE_LOGGING
#elif CORE_PROFILE
    constexpr const char* c_BuildConfig = "Profile";
#   define CORE_ENABLE_VERIFY
#   define CORE_ENABLE_LOGGING
#else
    constexpr const char* c_BuildConfig = "Dist";
#endif

#define CORE_EXPAND_MACRO(x) x
#define CORE_STRINGIFY_MACRO(x) #x
#define CORE_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

#define ENUM_CLASS_FLAG_OPERATORS(T)                                        \
    inline T operator | (T a, T b) { return T(uint32_t(a) | uint32_t(b)); } \
    inline T operator & (T a, T b) { return T(uint32_t(a) & uint32_t(b)); } \
    inline T operator ^ (T a, T b) { return T(uint32_t(a) ^ uint32_t(b)); } \
    inline T operator ~ (T a) { return T(~uint32_t(a)); }                   \
    inline T& operator |= (T& a, T b) { a = a | b; return a; }              \
    inline T& operator &= (T& a, T b) { a = a & b; return a; }              \
    inline T& operator ^= (T& a, T b) { a = a ^ b; return a; }              \
    inline bool operator !(T a) { return uint32_t(a) == 0; }                \
    inline bool operator ==(T a, uint32_t b) { return uint32_t(a) == b; }   \
    inline bool operator !=(T a, uint32_t b) { return uint32_t(a) != b; }

#define NOT_YET_IMPLEMENTED() LOG_CORE_ERROR("{0}, {1}, {2} not implemented yet\n", __FILE__, __LINE__ ,__func__); CORE_VERIFY(false)

//////////////////////////////////////////////////////////////////////////
// Log
//////////////////////////////////////////////////////////////////////////

#ifdef CORE_ENABLE_LOGGING
    #define LOG_CORE_TRACE(...)    Core::Log::CoreTrace(std::format(__VA_ARGS__).c_str())
    #define LOG_CORE_INFO(...)     Core::Log::CoreInfo(std::format(__VA_ARGS__).c_str())
    #define LOG_CORE_WARN(...)     Core::Log::CoreWarn(std::format(__VA_ARGS__).c_str())
    #define LOG_CORE_ERROR(...)    Core::Log::CoreError(std::format(__VA_ARGS__).c_str())
    #define LOG_CORE_CRITICAL(...) Core::Log::CoreCritical(std::format(__VA_ARGS__).c_str())

    #define LOG_TRACE(...)         Core::Log::ClientTrace(std::format(__VA_ARGS__).c_str()) 
    #define LOG_INFO(...)          Core::Log::ClientInfo(std::format(__VA_ARGS__).c_str())
    #define LOG_WARN(...)          Core::Log::ClientWarn(std::format(__VA_ARGS__).c_str())
    #define LOG_ERROR(...)         Core::Log::ClientError(std::format(__VA_ARGS__).c_str())
    #define LOG_CRITICAL(...)      Core::Log::ClientCritical(std::format(__VA_ARGS__).c_str())
#else
    #define LOG_CORE_TRACE(...)
    #define LOG_CORE_INFO(...)
    #define LOG_CORE_WARN(...)
    #define LOG_CORE_ERROR(...)
    #define LOG_CORE_CRITICAL(...)
    
    #define LOG_TRACE(...)
    #define LOG_INFO(...)
    #define LOG_WARN(...)
    #define LOG_ERROR(...)
    #define LOG_CRITICAL(...)
#endif


//////////////////////////////////////////////////////////////////////////
// Assert && Verify
//////////////////////////////////////////////////////////////////////////

#if defined(CORE_PLATFORM_WINDOWS)
#   define CORE_DEBUGBREAK() __debugbreak()
#elif defined(CORE_PLATFORM_LINUX)
#   include <signal.h>
#   define CORE_DEBUGBREAK() raise(SIGTRAP)
#else
#   error "Platform doesn't support debugbreak yet!"
#endif

#define CORE_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { LOG##type##ERROR(msg, __VA_ARGS__); CORE_DEBUGBREAK(); } }
#define CORE_INTERNAL_ASSERT_WITH_MSG(type, check, ...) CORE_INTERNAL_ASSERT_IMPL(type, check, "Check failed: {}", __VA_ARGS__)
#define CORE_INTERNAL_ASSERT_NO_MSG(type, check) CORE_INTERNAL_ASSERT_IMPL(type, check, "Check '{}' failed at {}:{}", CORE_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)
#define CORE_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define CORE_INTERNAL_ASSERT_GET_MACRO(...) CORE_EXPAND_MACRO( CORE_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, CORE_INTERNAL_ASSERT_WITH_MSG, CORE_INTERNAL_ASSERT_NO_MSG) )

#ifdef CORE_ENABLE_ASSERTS
#   define CORE_ASSERT(...) CORE_EXPAND_MACRO( CORE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
#   define CORE_ASSERT(...)
#endif

#ifdef CORE_ENABLE_VERIFY
#   define CORE_VERIFY(...) CORE_EXPAND_MACRO( CORE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
#   define CORE_VERIFY(...)
#endif

//////////////////////////////////////////////////////////////////////////
// Profiler
//////////////////////////////////////////////////////////////////////////

#define BUILTIN_PROFILE_CPU_BEGIN(name) Profiler::CPUBegin(name)
#define BUILTIN_PROFILE_CPU_END() Profiler::CPUEnd()

#define BUILTIN_PROFILE_GPU_BEGIN(device, commandList, name) Profiler::GPUBegin(device, commandList, name)
#define BUILTIN_PROFILE_GPU_END() Profiler::GPUProfileEnd()

#define BUILTIN_PROFILE_BEGIN(device, commandList, name)\
            Profiler::CPUBegin(name);\
            Profiler::GPUBegin(device, commandList, name)

#define BUILTIN_PROFILE_END()\
            Profiler::CPUEnd();\
            Profiler::GPUEnd();

#define BUILTIN_PROFILE_CPU(name) Profiler::CPUScope _profiler_cpu_scope_timer(name)
#define BUILTIN_PROFILE_GPU(device, commandList, name) Profiler::GPUScope _profiler_gpu_scope_timer(device, commandList, name)
#define BUILTIN_PROFILE(device, commandList, name)\
            Profiler::CPUScope _profiler_cpu_scope_timer(name);\
            Profiler::GPUScope _profiler_gpu_scope_timer(device, commandList, name)

#if CORE_PROFILE 
#   ifndef TRACY_ENABLE
#       define TRACY_ENABLE
#   endif
#   include "tracy/Tracy.hpp"
#   define CORE_PROFILE_SCOPE(name) ZoneScopedN(name)
#   define CORE_PROFILE_SCOPE_COLOR(color) ZoneScopedC(color)
#   define CORE_PROFILE_SCOPE_NC(name,color) ZoneScopedNC(name, color)
#   define CORE_PROFILE_FUNCTION() ZoneScoped
#   define CORE_PROFILE_FRAME() FrameMark 
#   define CORE_PROFILE_TAG(y, x) ZoneText(x, strlen(x))
#   define CORE_PROFILE_LOG(text, size) TracyMessage(text, size)
#   define CORE_PROFILE_VALUE(text, value) TracyPlot(text, value)
#   define CORE_PROFILE_ALLOC(p, size) TracyAlloc(ptr, size)
#   define CORE_PROFILE_FREE(p) TracyFree(ptr);
#else
#   define CORE_PROFILE_SCOPE(name)
#   define CORE_PROFILE_SCOPE_COLOR(color)
#   define CORE_PROFILE_SCOPE_NC(name, color)
#   define CORE_PROFILE_FUNCTION()
#   define CORE_PROFILE_FRAME()
#   define CORE_PROFILE_TAG(y, x)
#   define CORE_PROFILE_LOG(text, size)
#   define CORE_PROFILE_VALUE(text, value)
#   define CORE_PROFILE_ALLOC(ptr, size)
#   define CORE_PROFILE_FREE(ptr)
#endif

//////////////////////////////////////////////////////////////////////////
// Shader
//////////////////////////////////////////////////////////////////////////

#ifdef NVRHI_HAS_D3D11
#   define STATIC_SHADER_D3D11(NAME) Core::Buffer{g_##NAME##_dxbc, std::size(g_##NAME##_dxbc)}
#else
#   define STATIC_SHADER_D3D11(NAME) Core::Buffer()
#endif
#ifdef NVRHI_HAS_D3D12
#   define STATIC_SHADER_D3D12(NAME) Core::Buffer{ g_##NAME##_dxil, std::size(g_##NAME##_dxil) }
#else
#   define STATIC_SHADER_D3D12(NAME) Core::Buffer()
#endif
#ifdef NVRHI_HAS_VULKAN
#   define STATIC_SHADER_SPIRV(NAME) Core::Buffer{ g_##NAME##_spirv, std::size(g_##NAME##_spirv) }
#else
#   define STATIC_SHADER_SPIRV(NAME) Core::Buffer()
#endif
#define STATIC_SHADER(NAME) RHI::StaticShader{ STATIC_SHADER_D3D11(NAME), STATIC_SHADER_D3D12(NAME), STATIC_SHADER_SPIRV(NAME) }

//////////////////////////////////////////////////////////////////////////
// Meta
//////////////////////////////////////////////////////////////////////////

#ifdef  META
#   define TYPE(...)   __attribute__((annotate("TYPE____ "#__VA_ARGS__)))
#   define PROPERTY(...) __attribute__((annotate("PROPERTY "#__VA_ARGS__)))
#else 
#   define TYPE(...)
#   define PROPERTY(...)
#endif

#define MetaHeader(nameSpace)                                                                                                            \
namespace Meta::nameSpace{                                                                                                               \
                                                                                                                                         \
    const Meta::TypeRegistry& Registry();                                                                                                \
                                                                                                                                         \
    template<typename T>                                                                                                                 \
    inline Meta::Type* Type()                                                                                                            \
    {                                                                                                                                    \
        auto str = std::string_view(typeid(T).name());                                                                                   \
                                                                                                                                         \
        if (str.substr(0, 6) == "class ") str = str.substr(6);                                                                           \
        else if (str.substr(0, 7) == "struct ") str = str.substr(7);                                                                     \
                                                                                                                                         \
        const auto& reg = Registry();                                                                                                    \
        auto type = reg.GetType(str);                                                                                                    \
                                                                                                                                         \
        if (type)                                                                                                                        \
            return reg.GetType(str);                                                                                                     \
                                                                                                                                         \
        return nullptr;                                                                                                                  \
    }                                                                                                                                    \
                                                                                                                                         \
    inline Meta::Type* Type(const std::string_view& name) { return Registry().GetType(name); }                                           \
}
