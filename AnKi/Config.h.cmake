// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

// ====================== WARNING: THIS FILE IS AUTO-GENERATED BY THE BUILD SYSTEM. DON'T ALTER IT =====================

#pragma once

/// @addtogroup config
/// @{

#define _ANKI_STR_HELPER(x) #x
#define _ANKI_STR(x) _ANKI_STR_HELPER(x)

#define ANKI_VERSION_MINOR ${ANKI_VERSION_MINOR}
#define ANKI_VERSION_MAJOR ${ANKI_VERSION_MAJOR}
#define ANKI_REVISION ${ANKI_REVISION}

#define ANKI_EXTRA_CHECKS ${_ANKI_EXTRA_CHECKS}
#define ANKI_DEBUG_SYMBOLS ${ANKI_DEBUG_SYMBOLS}
#define ANKI_OPTIMIZE ${ANKI_OPTIMIZE}
#define ANKI_TESTS ${ANKI_TESTS}
#define ANKI_ENABLE_TRACE ${_ANKI_ENABLE_TRACE}
#define ANKI_SOURCE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"

// Compiler
#if defined(__clang__)
#	define ANKI_COMPILER_CLANG 1
#	define ANKI_COMPILER_GCC 0
#	define ANKI_COMPILER_MSVC 0
#	define ANKI_COMPILER_GCC_COMPATIBLE 1
#	define ANKI_COMPILER_STR "clang " __VERSION__
#elif defined(__GNUC__) || defined(__GNUG__)
#	define ANKI_COMPILER_CLANG 0
#	define ANKI_COMPILER_GCC 1
#	define ANKI_COMPILER_MSVC 0
#	define ANKI_COMPILER_GCC_COMPATIBLE 1
#	define ANKI_COMPILER_STR "gcc " __VERSION__
#elif defined(_MSC_VER)
#	define ANKI_COMPILER_CLANG 0
#	define ANKI_COMPILER_GCC 0
#	define ANKI_COMPILER_MSVC 1
#	define ANKI_COMPILER_GCC_COMPATIBLE 0
#	define ANKI_COMPILER_STR "MSVC " _ANKI_STR(_MSC_FULL_VER)
#else
#	error Unknown compiler
#endif

// Operating system
#if defined(__WIN32__) || defined(_WIN32)
#	define ANKI_OS_LINUX 0
#	define ANKI_OS_ANDROID 0
#	define ANKI_OS_MACOS 0
#	define ANKI_OS_IOS 0
#	define ANKI_OS_WINDOWS 1
#	define ANKI_OS_STR "Windows"
#elif defined(__APPLE_CC__)
#	if (defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) && __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 40000) || (defined(__IPHONE_OS_VERSION_MIN_REQUIRED) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 40000)
#		define ANKI_OS_LINUX 0
#		define ANKI_OS_ANDROID 0
#		define ANKI_OS_MACOS 0
#		define ANKI_OS_IOS 1
#		define ANKI_OS_WINDOWS 0
#		define ANKI_OS_STR "iOS"
#	else
#		define ANKI_OS_LINUX 0
#		define ANKI_OS_ANDROID 0
#		define ANKI_OS_MACOS 1
#		define ANKI_OS_IOS 0
#		define ANKI_OS_WINDOWS 0
#		define ANKI_OS_STR "MacOS"
#	endif
#elif defined(__ANDROID__)
#	define ANKI_OS_LINUX 0
#	define ANKI_OS_ANDROID 1
#	define ANKI_OS_MACOS 0
#	define ANKI_OS_IOS 0
#	define ANKI_OS_WINDOWS 0
#	define ANKI_OS_STR "Android"
#elif defined(__linux__)
#	define ANKI_OS_LINUX 1
#	define ANKI_OS_ANDROID 0
#	define ANKI_OS_MACOS 0
#	define ANKI_OS_IOS 0
#	define ANKI_OS_WINDOWS 0
#	define ANKI_OS_STR "Linux"
#else
#	error Unknown OS
#endif

// Mobile or not
#define ANKI_PLATFORM_MOBILE (ANKI_OS_ANDROID || ANKI_OS_IOS)

// POSIX system or not
#if ANKI_OS_LINUX || ANKI_OS_ANDROID || ANKI_OS_MACOS || ANKI_OS_IOS
#	define ANKI_POSIX 1
#else
#	define ANKI_POSIX 0
#endif

// CPU architecture
#if ANKI_COMPILER_GCC_COMPATIBLE
#	if defined(__arm__) || defined(__aarch64__)
#		define ANKI_CPU_ARCH_X86 0
#		define ANKI_CPU_ARCH_ARM 1
#	elif defined(__i386__) || defined(__amd64__)
#		define ANKI_CPU_ARCH_X86 1
#		define ANKI_CPU_ARCH_ARM 0
#	endif
#elif ANKI_COMPILER_MSVC
#	if defined(_M_ARM)
#		define ANKI_CPU_ARCH_X86 0
#		define ANKI_CPU_ARCH_ARM 1
#	elif defined(_M_X64) || defined(_M_IX86)
#		define ANKI_CPU_ARCH_X86 1
#		define ANKI_CPU_ARCH_ARM 0
#	endif
#endif

#if defined(ANKI_CPU_ARCH_X86) && ANKI_CPU_ARCH_X86
#	define ANKI_GPU_ARCH_STR "x86"
#elif defined(ANKI_CPU_ARCH_ARM) && ANKI_CPU_ARCH_ARM
#	define ANKI_GPU_ARCH_STR "arm"
#else
#	error Unknown CPU arch
#endif

// SIMD
#define ANKI_ENABLE_SIMD ${_ANKI_ENABLE_SIMD}

#if !ANKI_ENABLE_SIMD
#	define ANKI_SIMD_NONE 1
#	define ANKI_SIMD_SSE 0
#	define ANKI_SIMD_NEON 0
#elif ANKI_CPU_ARCH_X86
#	define ANKI_SIMD_NONE 0
#	define ANKI_SIMD_SSE 1
#	define ANKI_SIMD_NEON 0
#else
#	define ANKI_SIMD_NONE 0
#	define ANKI_SIMD_SSE 0
#	define ANKI_SIMD_NEON 1
#endif

// Graphics backend
#define ANKI_GR_BACKEND_GL 0
#define ANKI_GR_BACKEND_VULKAN 1

// Some compiler attributes
#if ANKI_COMPILER_GCC_COMPATIBLE
#	define ANKI_LIKELY(x) __builtin_expect(!!(x), 1)
#	define ANKI_UNLIKELY(x) __builtin_expect(!!(x), 0)
#	define ANKI_RESTRICT __restrict
#	define ANKI_USE_RESULT __attribute__((warn_unused_result))
#	define ANKI_FORCE_INLINE __attribute__((always_inline))
#	define ANKI_DONT_INLINE __attribute__((noinline))
#	define ANKI_UNUSED __attribute__((__unused__))
#	define ANKI_COLD __attribute__((cold, optimize("Os")))
#	define ANKI_HOT __attribute__ ((hot))
#	define ANKI_UNREACHABLE() __builtin_unreachable()
#	define ANKI_PREFETCH_MEMORY(addr) __builtin_prefetch(addr)
#	define ANKI_CHECK_FORMAT(fmtArgIdx, firstArgIdx) __attribute__((format(printf, fmtArgIdx + 1, firstArgIdx + 1))) // On methods need to include "this"
#else
#	define ANKI_LIKELY(x) ((x) == 1)
#	define ANKI_UNLIKELY(x) ((x) == 1)
#	define ANKI_RESTRICT
#	define ANKI_USE_RESULT
#	define ANKI_FORCE_INLINE
#	define ANKI_DONT_INLINE
#	define ANKI_UNUSED
#	define ANKI_COLD
#	define ANKI_HOT
#	define ANKI_UNREACHABLE() __assume(false)
#	define ANKI_PREFETCH_MEMORY(addr) (void)(addr)
#	define ANKI_CHECK_FORMAT(fmtArgIdx, firstArgIdx)
#endif

// Pack structs
#if ANKI_COMPILER_MSVC
#	define ANKI_BEGIN_PACKED_STRUCT __pragma(pack (push, 1))
#	define ANKI_END_PACKED_STRUCT __pragma(pack (pop))
#else
#	define ANKI_BEGIN_PACKED_STRUCT _Pragma("pack (push, 1)")
#	define ANKI_END_PACKED_STRUCT _Pragma("pack (pop)")
#endif

// Builtins
#if ANKI_COMPILER_MSVC
#	include <intrin.h>
#	define __builtin_popcount __popcnt
#	define __builtin_popcountl(x) int(__popcnt64(x))
#	define __builtin_clzll(x) int(__lzcnt64(x))

#pragma intrinsic(_BitScanForward)
inline int __builtin_ctzll(unsigned long long x)
{
	unsigned long o;
	_BitScanForward64(&o, x);
	return o;
}
#endif

// Constants
#define ANKI_SAFE_ALIGNMENT 16
#define ANKI_CACHE_LINE_SIZE 64

// Misc
#define ANKI_FILE __FILE__
#define ANKI_FUNC __func__

// A macro used to mark some functions or variables as AnKi internal.
#ifdef ANKI_SOURCE_FILE
#	define ANKI_INTERNAL
#else
#	define ANKI_INTERNAL [[deprecated("This is an AnKi internal interface. Don't use it")]]
#endif

// Define the main() function.
#if ANKI_OS_ANDROID
extern "C" {
struct android_app;
}

namespace anki {
extern android_app* g_androidApp;

void* getAndroidCommandLineArguments(int& argc, char**& argv);
void cleanupGetAndroidCommandLineArguments(void* ptr);
}

#	define ANKI_MAIN_FUNCTION(myMain) \
	int myMain(int argc, char* argv[]); \
	extern "C" void android_main(android_app* app) \
	{ \
		anki::g_androidApp = app; \
		char** argv; \
		int argc; \
		void* cleanupToken = anki::getAndroidCommandLineArguments(argc, argv); \
		myMain(argc, argv); \
		anki::cleanupGetAndroidCommandLineArguments(cleanupToken); \
	}
#else
#	define ANKI_MAIN_FUNCTION(myMain) \
	int myMain(int argc, char* argv[]); \
	int main(int argc, char* argv[]) \
	{ \
		return myMain(argc, argv); \
	}
#endif
/// @}
