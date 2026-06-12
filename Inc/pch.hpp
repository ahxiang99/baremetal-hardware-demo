#pragma once

/* C++ Standard Libraries (heavy to parse in embedded) */
#include <array>
#include <atomic>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string_view>

/* Define ConstantExpr */
constexpr size_t   DMA_CHUNK_SIZE     = 128;
constexpr size_t   DMA_MAX_CHUNK_SIZE = 1024;
constexpr uint32_t HSI_Freq_Hz        = 16000000;

/* Custom C++ Library */
#include "FloatIntExtraction.hpp"
#include "Middleware/logger.hpp"
#include "RegisterUtils.hpp"
#include "drivers.hpp"

/* Custom C Library */
#include "bit_utils.h"
#include "cmsis.h"
#include "low-level/nvic.h"
