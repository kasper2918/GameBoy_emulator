#pragma once

#include <cstdint>

using BYTE = uint8_t;
using SIGNED_BYTE = int8_t;
using WORD = uint16_t;
using SIGNED_WORD = int16_t;

inline constexpr int WIDTH_MULTIPLIER{ 4 };
inline constexpr int HEIGHT_MULTIPLIER{ 3 };

inline constexpr int WINDOW_WIDTH{ 160 * WIDTH_MULTIPLIER };
inline constexpr int WINDOW_HEIGHT{ 144 * HEIGHT_MULTIPLIER };