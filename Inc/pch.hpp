#pragma once

/* C++ Standard Libraries (heavy to parse in embedded) */
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string_view>

/* */

/* Custom C++ Library */
#include "FloatIntExtraction.hpp"
#include "RegisterUtils.hpp"

/* Custom C Library */
#include "bit_utils.h"
#include "cmsis.h"

/* Define ConstantExpr*/
constexpr size_t DMA_CHUNK_SIZE     = 128;
constexpr size_t DMA_MAX_CHUNK_SIZE = 1024;