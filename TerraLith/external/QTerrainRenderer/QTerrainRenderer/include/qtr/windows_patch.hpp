/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once

// include in the sources, not in the headers

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef IN
#undef OUT
#undef Data
#endif
