#ifndef SPEL_MACROS
#define SPEL_MACROS

#ifdef DEBUG
#	define sp_debug 1
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(_WIN32)
#	if defined(sp_build)
#		define sp_api __declspec(dllexport)
#	else
#		define sp_api __declspec(dllimport)
#	endif
#else
#	define sp_api __attribute__((visibility("default")))
#endif

#if defined(__GNUC__) || defined(__clang__)
#	define sp_weak __attribute__((weak))
#else
#	define sp_weak
#endif

#if defined(__GNUC__) || defined(__clang__)
#	define sp_hidden __attribute__((visibility("hidden")))
#else
#	define sp_weak
#endif

#if defined(_MSC_VER)
#	define sp_weak_alias(sym, fallback)                                                 \
		__pragma(comment(linker, "/alternatename:" #sym "=" #fallback))
#else
#	define sp_weak_alias(sym, fallback)
#endif

#if defined(_MSC_VER)
#	define sp_inline __forceinline
#	define sp_no_inline __declspec(noinline)
#else
#	define sp_inline inline __attribute__((always_inline))
#	define sp_no_inline __attribute__((noinline))
#endif

#if defined(_MSC_VER)
#	define sp_align(N) __declspec(align(N))
#else
#	define sp_align(N) __attribute__((aligned(N)))
#endif

#define sp_unused(x) (void)(x)

#if sp_debug
#	define sp_assert(expr)                                                              \
		do                                                                               \
		{                                                                                \
			if (!(expr))                                                                 \
			{                                                                            \
				log_fatal("Assertion failed: %s", #expr);                                \
				exit(-1);                                                                \
			}                                                                            \
		} while (0)
#else
#	define sp_assert(expr)
#endif

#if defined(_WIN32)
#	define sp_platform_win 1
#	define sp_platform "Windows"
#elif defined(__MACOSX__)
#	define sp_platform_mac 1
#	define sp_platform "MacOS"
#elif defined(__IPHONEOS__)
#	define sp_platform_ios 1
#	define sp_platform "iOS"
#elif defined(__linux__)
#	define sp_platform_linux 1
#	define sp_platform "Linux"
#endif

#define sp_array_size(x) (sizeof(x) / sizeof((x)[0]))

#if defined(_MSC_VER)
#	define sp_deprecated(msg) __declspec(deprecated(msg))
#else
#	define sp_deprecated(msg) __attribute__((deprecated(msg)))
#endif

#if defined(_MSC_VER)
#	define sp_compiler_msvc 1
#elif defined(__clang__)
#	define sp_compiler_clang 1
#elif defined(__GNUC__)
#	define sp_compiler_gcc 1
#endif

#define sp_callback(callback)                                                            \
	if (callback)                                                                        \
	{                                                                                    \
		callback();                                                                      \
	}

#define sp_color_declare(name) sp_api Color spel_color_##name()
#define sp_color_define(name, r, g, b)                                                   \
	sp_api Color spel_color_##name()                                                     \
	{                                                                                    \
		return (Color){r, g, b, 255};                                                    \
	}
#endif