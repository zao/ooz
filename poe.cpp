#include "poe.h"

#include <string.h>

int64_t Kraken_Decompress(const uint8_t* src, size_t src_len, uint8_t* dst, size_t dst_len);

using OozMemHeader = int64_t;

OozMem OozMemAlloc(size_t size) {
    OozMemHeader h = size;
    uint8_t* p = new uint8_t[sizeof(OozMemHeader) + size];
    memcpy(p, &h, sizeof(OozMemHeader));
    return p + sizeof(OozMemHeader);
}

int64_t OozMemSize(OozMem mem) {
    if (!mem) {
        return -1;
    }
    OozMemHeader h;
    memcpy(&h, mem - sizeof(OozMemHeader), sizeof(OozMemHeader));
    return h;
}

void OozMemFree(OozMem mem) {
    if (!mem) {
        return;
    }
    uint8_t* p = mem - sizeof(OozMemHeader);
    delete[] p;
}

int64_t OozDecompressBlock(uint8_t const* src_data, size_t src_size, uint8_t* dst_data, size_t dst_size) {
    return Kraken_Decompress(src_data, src_size, dst_data, dst_size);
}

OozMem OozDecompressBlockAlloc(uint8_t const* src_data, size_t src_size, size_t dst_size) {
    OozMem mem = OozMemAlloc(dst_size);
    int64_t res = Kraken_Decompress(src_data, src_size, mem, dst_size);
    if (res != dst_size) {
        OozMemFree(mem);
        return nullptr;
    }
    return mem;
}

OozMem OozDecompressBundle(uint8_t const* src_data, size_t src_size) {
    return 0;
}