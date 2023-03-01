// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Ui/UiObject.h>
#include <AnKi/Ui/UiManager.h>

namespace anki {

HeapMemoryPool& UiObject::getMemoryPool() const
{
	return m_manager->getMemoryPool();
}

UiExternalSubsystems& UiObject::getExternalSubsystems() const
{
	return m_manager->m_subsystems;
}

} // end namespace anki
