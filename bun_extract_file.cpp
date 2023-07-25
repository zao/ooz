#include <bun.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "ggpk_vfs.h"
#include "path_rep.h"
#include "util.h"

#include <poe/util/utf.hpp>

using namespace std::string_view_literals;

static char const *const USAGE =
    "bun_extract_file list-files GGPK_OR_STEAM_DIR\n"
    "bun_extract_file extract-files [--regex] GGPK_OR_STEAM_DIR OUTPUT_DIR [FILE_PATHS...]\n\n"
    "GGPK_OR_STEAM_DIR should be either a full path to a Standalone GGPK file or the Steam game directory.\n"
    "If FILE_PATHS are omitted the file paths are taken from stdin.\n"
    "If --regex is given, FILE_PATHS are interpreted as regular expressions to match.\n";

int main(int argc, char *argv[]) {
  std::error_code ec;
  if (argc < 2 || argv[1] == "--help"sv || argv[1] == "-h"sv) {
    fprintf(stderr, USAGE);
    return 1;
  }

  std::string command;
  std::filesystem::path ggpk_or_steam_dir;
  std::filesystem::path output_dir;
  bool use_regex = false;
  std::vector<std::string> tail_args;

  command = argv[1];

  int argi = 2;
  while (argi < argc) {
    if (argv[argi] == "--regex"sv) {
      use_regex = true;
      ++argi;
    } else {
      break;
    }
  }

  if (command == "list-files"sv) {
    if (argi < argc) {
      ggpk_or_steam_dir = argv[argi++];
    }
    if (argi != argc) {
      fprintf(stderr, USAGE);
      return 1;
    }
  } else if (command == "extract-files"sv) {
    if (argi + 1 < argc) {
      ggpk_or_steam_dir = argv[argi++];
      output_dir = argv[argi++];
      while (argi < argc) {
        tail_args.push_back(argv[argi++]);
      }
    }
  } else {
    fprintf(stderr, USAGE);
    return 1;
  }

  std::shared_ptr<GgpkVfs> vfs;
  if (ggpk_or_steam_dir.extension() == ".ggpk") {
    vfs = open_ggpk(ggpk_or_steam_dir);
  }

#if _WIN32
  std::string ooz_dll = "libooz.dll";
#else
  std::string ooz_dll = "liblibooz.so";
#endif
  Bun *bun = BunNew(ooz_dll.c_str(), "Ooz_Decompress");
  if (!bun) {
    bun = BunNew(("./" + ooz_dll).c_str(), "Ooz_Decompress");
    if (!bun) {
      fprintf(stderr, "Could not initialize Bun library\n");
      return 1;
    }
  }

  BunIndex *idx = BunIndexOpen(bun, borrow_vfs(vfs), ggpk_or_steam_dir.string().c_str());
  if (!idx) {
    fprintf(stderr, "Could not open index\n");
    return 1;
  }

  if (command == "list-files") {
    auto rep_mem = BunIndexPathRepContents(idx);
    for (size_t path_rep_id = 0;; ++path_rep_id) {
      uint64_t hash;
      uint32_t offset;
      uint32_t size;
      uint32_t recursive_size;
      if (BunIndexPathRepInfo(idx, (int32_t)path_rep_id, &hash, &offset, &size, &recursive_size) < 0) {
        break;
      }
      auto generated = generate_paths(rep_mem + offset, size);
      for (auto &p : generated) {
        std::cout << p << std::endl;
      }
    }
    return 0;
  }

  std::vector<std::string> wanted_paths = tail_args;
  if (wanted_paths.empty()) {
    std::string line;
    while (std::getline(std::cin, line)) {
      wanted_paths.push_back(line);
    }
  }

  for (auto &path : wanted_paths) {
    if (BunIndexPathRepLowercase(idx)) {
      for (auto &ch : path) {
        ch = (char)std::tolower((int)(unsigned char)ch);
      }
    }
  }

  std::vector<std::regex> regexes;
  if (use_regex) {
    bool regexes_good = true;
    for (auto &path : wanted_paths) {
      try {
        regexes.push_back(std::regex(path));
      } catch (std::exception &e) {
        fprintf(stderr, "Could not compile regex \"%s\": %s\n", path.c_str(), e.what());
        regexes_good = false;
      }
    }
    if (!regexes_good) {
      return 1;
    }

    if (command == "extract-files") {
      std::unordered_set<std::string> matching_paths;
      auto rep_mem = BunIndexPathRepContents(idx);
      for (size_t path_rep_id = 0;; ++path_rep_id) {
        uint64_t hash;
        uint32_t offset;
        uint32_t size;
        uint32_t recursive_size;
        if (BunIndexPathRepInfo(idx, (int32_t)path_rep_id, &hash, &offset, &size, &recursive_size) < 0) {
          break;
        }
        auto generated = generate_paths(rep_mem + offset, size);
        for (auto &p : generated) {
          if (BunIndexPathRepLowercase(idx)) {
            for (auto &ch : p) {
              ch = (char)std::tolower((int)(unsigned char)ch);
            }
          }
          if (!matching_paths.count(p)) {
            for (auto &r : regexes) {
              if (std::regex_match(p, r)) {
                matching_paths.insert(p);
              }
            }
          }
        }
      }
      wanted_paths.assign(matching_paths.begin(), matching_paths.end());
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

  size_t extracted = 0;
  size_t missed = 0;
  for (auto &[bundle_id, parts] : bundle_parts_to_extract) {
    std::vector<uint8_t> bundle_data;
    auto bundle_mem = BunIndexExtractBundle(idx, bundle_id);
    if (!bundle_mem) {
      missed += parts.size();
      char const *name;
      uint32_t cb;
      BunIndexBundleInfo(idx, bundle_id, &name, &cb);
      fprintf(stderr, "Could not open bundle \"%s\", missing %zu files.\n", name, parts.size());
      continue;
    }
    for (auto &part : parts) {
      std::filesystem::path output_path = output_dir / part.path;
      std::filesystem::create_directories(output_path.parent_path(), ec);
      if (!dump_file(output_path, bundle_mem + part.offset, part.size)) {
        fprintf(stderr, "Could not write file \"%s\"\n", output_path.string().c_str());
        ++missed;
        continue;
      }
      ++extracted;
    }
    BunMemFree(bundle_mem);
  }
  fprintf(stderr, "Done, %zu/%zu extracted, %zu missed.\n", extracted, wanted_paths.size(), missed);
  BunIndexClose(idx);
  BunDelete(bun);

  return 0;
}
