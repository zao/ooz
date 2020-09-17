#include <bun.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include "util.h"

#include <poe/format/ggpk.hpp>
#include <poe/util/utf.hpp>

int main(int argc, char** argv) {
	std::error_code ec;
	if (argc < 3) {
		fprintf(stderr, "%s GGPK_OR_STEAM_DIR OUTPUT_DIR [FILE_PATHS...]\n\n"
			"GGPK_OR_STEAM_DIR should be either a full path to a Standalone GGPK file or the Steam game directory.\n"
			"If FILE_PATHS are omitted the file paths are taken from stdin.\n", argv[0]);
		return 1;
	}

	std::filesystem::path ggpk_or_steam_dir = argv[1];
	std::filesystem::path output_dir = argv[2];

	Vfs* vfs = nullptr;
	namespace ggpk = poe::format::ggpk;

	struct GgpkVfs {
		Vfs vfs;
		std::unique_ptr<poe::format::ggpk::parsed_ggpk> pack;
	} ggpk_vfs;
	ggpk_vfs.vfs.open = [](Vfs* vfs, char const* c_path) -> VfsFile* {
		auto* gvfs = reinterpret_cast<GgpkVfs*>(vfs);
		ggpk::parsed_directory const* dir = gvfs->pack->root_;
		std::u16string path = poe::util::lowercase(poe::util::to_u16string(c_path));
		std::u16string_view tail(path);
		while (!tail.empty() && dir) {
			size_t delim = tail.find(u'/');
			if (delim == 0) {
				continue;
			}
			std::u16string_view head = tail.substr(0, delim);
			bool next_found = false;
			for (auto& child : dir->entries_) {
				if (poe::util::lowercase(child->name_) == head) {
					if (delim == std::string_view::npos) {
						return (VfsFile*)dynamic_cast<ggpk::parsed_file const*>(child.get());
					}
					else {
						dir = dynamic_cast<ggpk::parsed_directory const*>(child.get());
						tail = tail.substr(delim + 1);
						next_found = true;
					}
				}
			}
			if (!next_found) {
				return nullptr;
			}
		}
		return nullptr;
	};
	ggpk_vfs.vfs.close = [](Vfs* vfs, VfsFile* file) {};
	ggpk_vfs.vfs.size = [](Vfs*, VfsFile* file) -> int64_t {
		auto* f = reinterpret_cast<ggpk::parsed_file const*>(file);
		return f ? f->data_size_ : -1;
	};
	ggpk_vfs.vfs.read = [](Vfs* vfs, VfsFile* file, uint8_t* out, int64_t offset, int64_t size) -> int64_t {
		auto* gvfs = reinterpret_cast<GgpkVfs*>(vfs);
		auto* f = reinterpret_cast<ggpk::parsed_file const*>(file);
		if (offset + size > f->data_size_) {
			return -1;
		}
		memcpy(out, gvfs->pack->mapping_.data() + f->data_offset_ + offset, size);
		return size;
	};
	if (ggpk_or_steam_dir.extension() == ".ggpk") {
		ggpk_vfs.pack = poe::format::ggpk::index_ggpk(ggpk_or_steam_dir);
		vfs = &ggpk_vfs.vfs;
	}

#if _WIN32
	std::string ooz_dll = "libooz.dll";
#else
	std::string ooz_dll = "./liblibooz.so";
#endif
	Bun* bun = BunNew(ooz_dll.c_str(), "Ooz_Decompress");
	if (!bun) {
		fprintf(stderr, "Could not initialize Bun library\n");
		return 1;
	}

	BunIndex* idx = BunIndexOpen(bun, vfs, ggpk_or_steam_dir.string().c_str());
	if (!idx) {
		fprintf(stderr, "Could not open index\n");
		return 1;
	}

	std::vector<std::string> wanted_paths;
	if (argc == 3) {
		std::string line;
		while (std::getline(std::cin, line)) {
			wanted_paths.push_back(line);
		}
	}
	else {
		for (size_t i = 3; i < argc; ++i) {
			wanted_paths.push_back(argv[i]);
		}
	}

	struct extract_info {
		std::string path;
		uint32_t offset;
		uint32_t size;
	};

	std::unordered_map<uint32_t, std::vector<extract_info>> bundle_parts_to_extract;

	for (auto path : wanted_paths) {
		if (path.front() == '"' && path.back() == '"') {
			path = path.substr(1, path.size() - 2);
		}

		int32_t file_id = BunIndexLookupFileByPath(idx, path.c_str());
		if (file_id < 0) {
			fprintf(stderr, "Could not find file \"%s\"\n", path.c_str());
		}

		uint32_t bundle_id;

		uint64_t path_hash;
		extract_info ei;
		ei.path = path;

		BunIndexFileInfo(idx, file_id, &path_hash, &bundle_id, &ei.offset, &ei.size);
		bundle_parts_to_extract[bundle_id].push_back(ei);
	}

	for (auto& [bundle_id, parts] : bundle_parts_to_extract) {
		std::vector<uint8_t> bundle_data;
		auto bundle_mem = BunIndexExtractBundle(idx, bundle_id);
		for (auto& part : parts) {
			std::filesystem::path output_path = output_dir / part.path;
			std::filesystem::create_directories(output_path.parent_path(), ec);
			if (!dump_file(output_path, bundle_mem + part.offset, part.size)) {
				fprintf(stderr, "Could not write file \"%s\"\n", output_path.string().c_str());
				return 1;
			}
		}
		BunMemFree(bundle_mem);
	}
	BunIndexClose(idx);
	BunDelete(bun);

	return 0;
}
