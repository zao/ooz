#include "poe.h"

#include <string.h>

#include <algorithm>
#include <vector>

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

struct reader {
	uint8_t const* p;
	size_t n;

	template <typename T>
	bool read(T& t) {
		size_t k = sizeof(T);
		if (n < k) {
			return false;
		}
		memcpy(&t, p, k);
		p += k;
		n -= k;
		return true;
	}

	template <typename T, size_t N>
	bool read(T(&ts)[N]) {
		size_t k = N * sizeof(T);
		if (n < k) {
			return false;
		}
		memcpy(ts, p, k);
		p += k;
		n -= k;
		return true;
	}

	template <typename T>
	bool read(std::vector<T>& ts) {
		size_t k = ts.size() * sizeof(T);
		if (n < k) {
			return false;
		}
		memcpy(ts.data(), p, k);
		p += k;
		n -= k;
		return true;
	}
};

OozMem OozDecompressBundle(uint8_t const* src_data, size_t src_size) {
	struct fixed_header {
		uint32_t uncompressed_size;
		uint32_t total_payload_size;
		uint32_t head_payload_size;
		enum encoding_schemes { Kraken_6 = 8, Mermaid_A = 9, Leviathan_C = 13 };
		uint32_t first_file_encode;
		uint32_t unk10;
		uint64_t uncompressed_size2;
		uint64_t total_payload_size2;
		uint32_t block_count;
		uint32_t unk28[5];
	} fix_h;

	reader r = { src_data, src_size };

	if (!r.read(fix_h.uncompressed_size) ||
		!r.read(fix_h.total_payload_size) ||
		!r.read(fix_h.head_payload_size) ||
		!r.read(fix_h.first_file_encode) ||
		!r.read(fix_h.unk10) ||
		!r.read(fix_h.uncompressed_size2) ||
		!r.read(fix_h.total_payload_size2) ||
		!r.read(fix_h.block_count) ||
		!r.read(fix_h.unk28)) {
		return 0;
	}

	std::vector<uint32_t> entry_sizes(fix_h.block_count);
	if (!r.read(entry_sizes)) {
		return 0;
	}

	if (r.n < fix_h.total_payload_size2) {
		return 0;
	}

	OozMem out_mem = OozMemAlloc(fix_h.uncompressed_size2);
	if (!out_mem) {
		return 0;
	}

	uint8_t const* p = r.p;
	size_t n = r.n;
	uint8_t* out_p = out_mem;
	size_t out_cur = 0;
	for (size_t i = 0; i < entry_sizes.size(); ++i) {
		size_t amount_to_write = (std::min<size_t>)(fix_h.uncompressed_size2 - out_cur, fix_h.unk28[0]);
		int64_t amount_written = OozDecompressBlock(p, entry_sizes[i], out_p + out_cur, amount_to_write);
		if (amount_written != amount_to_write) {
			OozMemFree(out_mem);
			return 0;
		}
		p += entry_sizes[i];
		n -= entry_sizes[i];
		out_cur += amount_written;
	}

	return out_mem;
}