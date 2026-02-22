#ifndef SPEL_MACROS
#define SPEL_MACROS

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(DEBUG)
#	define spel_debug_build 1
#else
#	define spel_debug_build 0
#endif

#if defined(_WIN32)
#	define spel_api __declspec(dllexport)
#else
#	define spel_api __attribute__((visibility("default")))
#endif

#ifdef SP_WEAK_LINK
#	if defined(__GNUC__) || defined(__clang__)
#		define spel_weak __attribute__((weak))
#	else
#		define spel_weak
#	endif
#else
#	define spel_weak
#endif

#if defined(__GNUC__) || defined(__clang__)
#	define spel_hidden __attribute__((visibility("hidden")))
#else
#	define spel_hidden
#endif

#ifdef SP_WEAK_LINK
#	if defined(_MSC_VER)
#		define spel_weak_alias(sym, fallback)                                             \
			_Pragma("message(\"aliasing " #sym " to " #fallback "\")")                  \
			_Pragma("comment(linker, \"/alternatename:\"" #sym "=" #fallback ")")
#	endif
#else
#	define spel_weak_alias(sym, fallback)
#endif

#if defined(_MSC_VER)
#	define spel_inline __forceinline
#	define spel_no_inline __declspec(noinline)
#else
#	define spel_inline inline __attribute__((always_inline))
#	define spel_no_inline __attribute__((noinline))
#endif

#if defined(_MSC_VER)
#	define spel_align(N) __declspec(align(N))
#else
#	define spel_align(N) __attribute__((aligned(N)))
#endif

#define spel_unused(x) (void)(x)

#if defined(_WIN32)
#	define spel_platform_win 1
#	define spel_platform "Windows"
#elif defined(__MACOSX__)
#	define spel_platform_mac 1
#	define spel_platform "MacOS"
#elif defined(__IPHONEOS__)
#	define spel_platform_ios 1
#	define spel_platform "iOS"
#elif defined(__linux__)
#	define spel_platform_linux 1
#	define spel_platform "Linux"
#endif

#define spel_array_size(x) (sizeof(x) / sizeof((x)[0]))

#if defined(_MSC_VER)
#	define spel_deprecated(msg) __declspec(deprecated(msg))
#else
#	define spel_deprecated(msg) __attribute__((deprecated(msg)))
#endif

#if defined(_MSC_VER)
#	define spel_compiler_msvc 1
#elif defined(__clang__)
#	define spel_compiler_clang 1
#elif defined(__GNUC__)
#	define spel_compiler_gcc 1
#endif

#define spel_callback(callback)                                                            \
	if (callback)                                                                        \
	{                                                                                    \
		callback();                                                                      \
	}
#endif