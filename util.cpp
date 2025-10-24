#include "util.h"

#ifdef _WIN32
#include <Windows.h>
#endif
#include <fstream>

std::string hex_dump(size_t width, void const* data, size_t size) {
	auto* p = reinterpret_cast<uint8_t const*>(data);
	auto n = size;
	std::string s;
	char buf[10];
	while (n) {
		size_t k = (std::min<size_t>)(n, width);
		for (size_t i = 0; i < k; ++i) {
			sprintf(buf, "%s%02X", (i ? " " : ""), p[i]);
			s += buf;
		}
		for (size_t i = k; i < width; ++i) {
			s += "   ";
		}
		s += " | ";
		for (size_t i = 0; i < k; ++i) {
			auto c = p[i];
			if (isprint((int)c)) {
				s += c;
			}
			else {
				s += ".";
			}
		}
		s += "\n";
		p += k;
		n -= k;
	}
	return s;
}

bool slurp_file(const std::filesystem::path& path, std::vector<uint8_t>& data) {
	std::ifstream is(path, std::ios::binary);
	if (!is) {
		return false;
	}

	is.seekg(0, std::ios::end);
	auto size = static_cast<uint64_t>(is.tellg());
	is.seekg(0, std::ios::beg);

	data.resize(size);
	if (!is.read(reinterpret_cast<char*>(data.data()), data.size())) {
		return false;
	}
	return true;
}

bool dump_file(const std::filesystem::path& filename, void const* data, size_t size) {
#if _WIN32
  HANDLE fh = CreateFileW(filename.wstring().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (!fh)
    return false;
  auto* data_ptr = (const char *)data;
  size_t remaining = size;
  while (remaining) {
    DWORD chunk_size = (DWORD)(std::min)(remaining, 1ull << 20);
    DWORD bytes_written{};
    if (!WriteFile(fh, data_ptr, chunk_size, &bytes_written, nullptr)) {
      CloseHandle(fh);
      return false;
    }
    remaining -= bytes_written;
    data_ptr += bytes_written;
  }
  CloseHandle(fh);
  return true;
#else
	std::ofstream os(filename, std::ios::binary | std::ios::trunc);
	if (!os) {
		return false;
	}
	return !!os.write(reinterpret_cast<char const*>(data), size);
#endif
}