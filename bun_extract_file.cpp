#include <bun.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include "util.h"

int main(int argc, char** argv) {
	std::error_code ec;
	if (argc < 3) {
		fprintf(stderr, "%s BUNDLES2_DIR OUTPUT_DIR [FILE_PATHS...]\n\nIf FILE_PATHS are omitted the file paths are taken from stdin.\n", argv[0]);
		return 1;
	}

	std::filesystem::path bundle_dir = argv[1];
	std::filesystem::path output_dir = argv[2];

	Bun* bun = BunNew("libooz.dll", "Ooz_Decompress");
	if (!bun) {
		fprintf(stderr, "libooz.dll not found\n");
		return 1;
	}

	BunIndex* idx = BunIndexOpen(bun, nullptr, bundle_dir.string().c_str());
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