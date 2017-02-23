// Copyright (C) 2009-2017, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <anki/util/StdTypes.h>

namespace anki
{

/// @addtogroup util_system
/// @{

/// Get the number of CPU cores
extern U32 getCpuCoresCount();

/// Print the backtrace
extern void printBacktrace();

/// Trap the sefaults.
extern void trapSegfaults(void (*)(void*));

/// @}

} // end namespace anki
