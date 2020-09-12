#pragma once

#include <stdint.h>

#if defined _WIN32 || defined __CYGWIN__
#ifdef OOZ_BUILD_DLL
#ifdef __GNUC__
#define OOZ_DLL_PUBLIC __attribute__ ((dllexport))
#else
#define OOZ_DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define OOZ_DLL_PUBLIC __attribute__ ((dllimport))
#else
#define OOZ_DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#define OOZ_DLL_LOCAL
#else
#if __GNUC__ >= 4
#define OOZ_DLL_PUBLIC __attribute__ ((visibility ("default")))
#define OOZ_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define OOZ_DLL_PUBLIC
#define OOZ_DLL_LOCAL
#endif
#endif

extern "C" {
	typedef uint8_t* OozMem;

	OOZ_DLL_PUBLIC OozMem OozMemAlloc(size_t size);

	OOZ_DLL_PUBLIC int64_t OozMemSize(OozMem mem);

	OOZ_DLL_PUBLIC void OozMemFree(OozMem mem);

	OOZ_DLL_PUBLIC int64_t OozDecompressBlock(uint8_t const* src_data, size_t src_size, uint8_t* dst_data, size_t dst_size);

	OOZ_DLL_PUBLIC OozMem OozDecompressBlockAlloc(uint8_t const* src_data, size_t src_size, size_t dst_size);

	OOZ_DLL_PUBLIC OozMem OozDecompressBundle(uint8_t const* src_data, size_t src_size);
}