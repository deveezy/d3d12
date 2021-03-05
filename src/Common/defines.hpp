#pragma once

// Unsigned int types.
typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

// Signed int types.
typedef signed char         i8;
typedef signed short        i16;
typedef signed int          i32;
typedef signed long long    i64;

// Floating point types
typedef float               f32;
typedef double              f64;

// Boolean type
typedef int                 b32;

// Ensure all types are of the correct size.
static_assert(sizeof(u8)  == 1, "Expected u8 to be 1 byte.");
static_assert(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
static_assert(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
static_assert(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

static_assert(sizeof(i8)  == 1, "Expected i8 to be 1 byte.");
static_assert(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
static_assert(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
static_assert(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

static_assert(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
static_assert(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");

#define SL_U32_MAX 0xffffffffui32
#define SL_U64_MAX 0xffffffffffffffffui64
#define NAME_MAX 255
#define PATH_MAX 4096

#define BUF_1K 1024
#define BUF_2K 2048 
#define BUF_4K 4096 

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#if _WIN32 || _WIN64
#   define SL_PLATFORM SL_PLATFORM_WINDOWS
#else
# if __linux__
#   define SL_PLATFORM SL_PLATFORM_LINUX
# elif __APPLE__
#   define SL_PLATFORM SL_PLATFORM_MAC
# else
#   error "Unable to determine platform!"
# endif
#endif

#if SL_PLATFORM == SL_PLATFORM_WINDOWS
#define SL_INLINE __forceinline
#define SL_NOINLINE _declspec(noinline)
#elif SL_PLATFORM == SL_PLATFORM_LINUX || SL_PLATFORM == SL_PLATFORM_MAC
#define SL_INLINE inline
#define SL_NOINLINE
#endif

// Assertions
#define SL_ASSERTIONS_ENABLED
#ifdef SL_ASSERTIONS_ENABLED
#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak();
#else
#define debugBreak() __asm { int 3 }
#endif
#endif

void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line);

#define SL_ASSERT(expr) { \
    if( expr ) { \
    } else { \
        report_assertion_failure(#expr, "", __FILE__, __LINE__); \
        debugBreak(); \
    } \
}

#define SL_ASSERT_MSG(expr, message) { \
    if( expr ) { \
    } else { \
        report_assertion_failure(#expr, message, __FILE__, __LINE__); \
        debugBreak(); \
    } \
}

#if defined(DEBUG) || defined(_DEBUG)
#define SL_ASSERT_DEBUG(expr) { \
    if( expr ) { \
    } else { \
        report_assertion_failure(#expr, "", __FILE__, __LINE__); \
        debugBreak(); \
    } \
}
#else
#define SL_ASSERT_DEBUG(expr) // Does nothing at all
#endif

#if _DEBUG
#define SL_DEBUG_BUILD 1
#define SL_RELEASE_BUILD 0
#else
#define SL_DEBUG_BUILD 0
#define SL_RELEASE_BUILD 1
#endif

#define SL_CLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max : value;

