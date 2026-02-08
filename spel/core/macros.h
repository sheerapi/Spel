#ifndef SPEL_MACROS
#define SPEL_MACROS

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(DEBUG)
#	define sp_debug_build 1
#else
#	define sp_debug_build 0
#endif

#if defined(_WIN32)
#	define sp_api __declspec(dllexport)
#else
#	define sp_api __attribute__((visibility("default")))
#endif

#ifdef SP_WEAK_LINK
#	if defined(__GNUC__) || defined(__clang__)
#		define sp_weak __attribute__((weak))
#	else
#		define sp_weak
#	endif
#else
#	define sp_weak
#endif

#if defined(__GNUC__) || defined(__clang__)
#	define sp_hidden __attribute__((visibility("hidden")))
#else
#	define sp_hidden
#endif

#ifdef SP_WEAK_LINK
#	if defined(_MSC_VER)
#		define sp_weak_alias(sym, fallback)                                             \
			__pragma(comment(linker, "/alternatename:" #sym "=" #fallback))
#	endif
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

#define sp_color_declare(name) sp_api spel_color spel_color_##name()
#define sp_color_define(name, r, g, b)                                                   \
	sp_api spel_color spel_color_##name()                                                \
	{                                                                                    \
		return (spel_color){r, g, b, 255};                                               \
	}

#ifdef DEBUG
#	define sp_malloc(size, tag) spel_memory_malloc(size, tag)
#	define sp_free(ptr) spel_memory_free(ptr)
#	define sp_realloc(ptr, size, tag) spel_memory_realloc(ptr, size, tag)
#else
#	define sp_malloc(size, tag) spel_memory_malloc(size, tag)
#	define sp_free(ptr) spel_memory_free(ptr)
#	define sp_realloc(ptr, size, tag) spel_memory_realloc(ptr, size, tag)
#endif
#endif