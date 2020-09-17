#pragma once

#include <cstdint>

uint64_t fnv1_64(void const* data, size_t n);
uint64_t fnv1a_64(void const* data, size_t n);