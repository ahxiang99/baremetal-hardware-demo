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
constexpr size_t   CHUNK_SIZE    = 64;
constexpr size_t   BUFF_SIZE     = 128;
constexpr uint32_t HSI_Freq_Hz   = 16000000;
constexpr bool     kSensorEnable = true;

/* Custom C++ Library */
#include "AppMode.hpp"
#include "FloatIntExtraction.hpp"
#include "Middleware/logger.hpp"
#include "RegisterUtils.hpp"
#include "Result.hpp"
#include "drivers.hpp"

/* Custom C Library */
#include "FreeRTOS.h"
#include "bit_utils.h"
#include "cmsis.h"
#include "low-level/nvic.h"
#include "semphr.h"
#include "task.h"
