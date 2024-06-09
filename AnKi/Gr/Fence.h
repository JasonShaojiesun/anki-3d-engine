// Copyright (C) 2009-present, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Gr/GrObject.h>

namespace anki {

/// @addtogroup graphics
/// @{

/// GPU fence.
class Fence : public GrObject
{
	ANKI_GR_OBJECT

public:
	static constexpr GrObjectType kClassType = GrObjectType::kFence;

	/// Wait for the fence.
	/// @param seconds The time to wait in seconds. If it's zero then just return the status.
	/// @return True if is signaled (signaled == GPU work is done).
	Bool clientWait(Second seconds);

protected:
	/// Construct.
	Fence(CString name)
		: GrObject(kClassType, name)
	{
	}

	/// Destroy.
	~Fence()
	{
	}

private:
	/// Allocate and initialize a new instance.
	[[nodiscard]] static Fence* newInstance();
};
/// @}

} // end namespace anki
