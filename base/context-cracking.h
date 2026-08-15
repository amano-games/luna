#pragma once

// Compiler cracking

#if defined(__clang__)
#define COMPILER_CLANG 1
#elif defined(_MSC_VER)
#define COMPILER_MSVC 1
#if _MSC_VER >= 1920
#define COMPILER_MSVC_YEAR 2019
#elif _MSC_VER >= 1910
#define COMPILER_MSVC_YEAR 2017
#elif _MSC_VER >= 1900
#define COMPILER_MSVC_YEAR 2015
#elif _MSC_VER >= 1800
#define COMPILER_MSVC_YEAR 2013
#elif _MSC_VER >= 1700
#define COMPILER_MSVC_YEAR 2012
#elif _MSC_VER >= 1600
#define COMPILER_MSVC_YEAR 2010
#elif _MSC_VER >= 1500
#define COMPILER_MSVC_YEAR 2008
#elif _MSC_VER >= 1400
#define COMPILER_MSVC_YEAR 2005
#else
#define COMPILER_MSVC_YEAR 0
#endif
#elif defined(__GNUC__) || defined(__GNUG__)
#define COMPILER_GCC 1
#else
#error Compiler not supported.
#endif

// Arch cracking

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_AMD64)
#define ARCH_X64 1
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86)
#define ARCH_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define ARCH_ARM64 1
#elif defined(__arm__) || defined(_M_ARM)
#define ARCH_ARM32 1
#elif defined(__wasm64__)
#define ARCH_WASM64 1
#elif defined(__wasm32__) || defined(__wasm__)
#define ARCH_WASM32 1
#else
#error Architecture not supported.
#endif

#if defined(ARCH_X64) || defined(ARCH_ARM64) || defined(ARCH_WASM64)
#define ARCH_64BIT 1
#elif defined(ARCH_X86) || defined(ARCH_ARM32) || defined(ARCH_WASM32)
#define ARCH_32BIT 1
#endif

#if defined(ARCH_ARM32) || defined(ARCH_ARM64) || defined(ARCH_X64) || defined(ARCH_X86) || defined(ARCH_WASM32) || defined(ARCH_WASM64)
#define ARCH_LITTLE_ENDIAN 1
#else
#error Endianness of this architecture not understood by context cracker.
#endif

// Language cracking

#if defined(__cplusplus)
#define LANG_CPP 1
#else
#define LANG_C 1
#endif

// OS cracking — Playdate SDK flags first (sim builds on a host OS), else compiler builtins.

#if defined(TARGET_PLAYDATE) && TARGET_PLAYDATE
#define OS_PLAYDATE 1
#define PD_DEVICE   1
#elif defined(TARGET_SIMULATOR)
#define OS_PLAYDATE 1
#define PD_SIM      1
#elif defined(_WIN32)
#define OS_WINDOWS 1
#elif defined(__EMSCRIPTEN__)
#define OS_WASM 1
#elif defined(__gnu_linux__) || defined(__linux__)
#define OS_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define OS_MACOS 1
#else
#error Operating system not supported.
#endif

// Build option cracking

#if !defined(BUILD_DEBUG)
#if defined(DEBUG) && DEBUG
#define BUILD_DEBUG 1
#elif defined(NDEBUG)
#define BUILD_DEBUG 0
#else
#define BUILD_DEBUG 0
#endif
#endif

// Prof option cracking
// PROF_HISTORY: 0 = off, 1 = exclusive-µs ring, 2 = full present ring
#define PROF_HISTORY_NONE  0
#define PROF_HISTORY_ZONE  1
#define PROF_HISTORY_FRAME 2

#if !defined(PROF)
#define PROF BUILD_DEBUG
#endif
#if !defined(PROF_HISTORY)
#if PROF
#define PROF_HISTORY PROF_HISTORY_FRAME
#else
#define PROF_HISTORY PROF_HISTORY_NONE
#endif
#endif
#if !defined(PROF_HISTORY_SIZE)
#define PROF_HISTORY_SIZE 128
#endif

#if !PROF
#undef PROF_HISTORY
#define PROF_HISTORY PROF_HISTORY_NONE
#endif

// Zero all undefined options

#if !defined(ARCH_32BIT)
#define ARCH_32BIT 0
#endif
#if !defined(ARCH_64BIT)
#define ARCH_64BIT 0
#endif
#if !defined(ARCH_X64)
#define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
#define ARCH_X86 0
#endif
#if !defined(ARCH_ARM64)
#define ARCH_ARM64 0
#endif
#if !defined(ARCH_ARM32)
#define ARCH_ARM32 0
#endif
#if !defined(ARCH_WASM64)
#define ARCH_WASM64 0
#endif
#if !defined(ARCH_WASM32)
#define ARCH_WASM32 0
#endif
#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif
#if !defined(COMPILER_GCC)
#define COMPILER_GCC 0
#endif
#if !defined(COMPILER_CLANG)
#define COMPILER_CLANG 0
#endif
#if !defined(OS_WINDOWS)
#define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
#define OS_LINUX 0
#endif
#if !defined(OS_MACOS)
#define OS_MACOS 0
#endif
#if !defined(OS_WASM)
#define OS_WASM 0
#endif
#if !defined(OS_PLAYDATE)
#define OS_PLAYDATE 0
#endif
#if !defined(PD_DEVICE)
#define PD_DEVICE 0
#endif
#if !defined(PD_SIM)
#define PD_SIM 0
#endif
#if !defined(SYS_GFX_SOKOL)
#define SYS_GFX_SOKOL 0
#endif
#if !defined(LANG_CPP)
#define LANG_CPP 0
#endif
#if !defined(LANG_C)
#define LANG_C 0
#endif
#if !defined(PROF)
#define PROF 0
#endif
#if !defined(PROF_HISTORY)
#define PROF_HISTORY PROF_HISTORY_NONE
#endif

// Pairing / sanity checks

#if OS_PLAYDATE && SYS_GFX_SOKOL
#error Playdate platform cannot use Sokol backend.
#endif
#if OS_PLAYDATE && (PD_DEVICE + PD_SIM) != 1
#error Playdate builds require exactly one of PD_DEVICE or PD_SIM.
#endif
#if !OS_PLAYDATE && (PD_DEVICE || PD_SIM)
#error PD_DEVICE/PD_SIM are only valid with OS_PLAYDATE.
#endif
#if (OS_LINUX + OS_MACOS + OS_WINDOWS + OS_WASM + OS_PLAYDATE) != 1
#error Exactly one OS_* platform flag must be set.
#endif
#if PROF_HISTORY_SIZE < 1
#error PROF_HISTORY_SIZE must be >= 1
#endif
#if PROF_HISTORY < PROF_HISTORY_NONE || PROF_HISTORY > PROF_HISTORY_FRAME
#error PROF_HISTORY must be 0 (off), 1 (zone), or 2 (frame)
#endif

// Platform identity as a value
//
// These mirror the OS_* macros above, but as an enum constant, so platform
// checks can be written as ordinary C. The comparison folds at compile time;
// the untaken branch is eliminated. Prefer this over #if when the branch body
// compiles on every platform — it stays type-checked everywhere.
enum os_kind {
	OS_KIND_NONE,

	OS_KIND_PLAYDATE,
	OS_KIND_WINDOWS,
	OS_KIND_LINUX,
	OS_KIND_MACOS,
	OS_KIND_WASM,

	OS_KIND_NUM_COUNT,

#if OS_PLAYDATE
	OS_KIND_CURRENT = OS_KIND_PLAYDATE,
#elif OS_WINDOWS
	OS_KIND_CURRENT = OS_KIND_WINDOWS,
#elif OS_LINUX
	OS_KIND_CURRENT = OS_KIND_LINUX,
#elif OS_MACOS
	OS_KIND_CURRENT = OS_KIND_MACOS,
#elif OS_WASM
	OS_KIND_CURRENT = OS_KIND_WASM,
#else
	OS_KIND_CURRENT = OS_KIND_NONE,
#endif
};
