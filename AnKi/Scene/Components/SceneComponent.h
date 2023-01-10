// Copyright (C) 2009-2022, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Scene/Common.h>
#include <AnKi/Util/Functions.h>
#include <AnKi/Util/BitMask.h>

namespace anki {

// Forward
class SceneComponentUpdateInfo;

/// @addtogroup scene
/// @{

constexpr U32 kMaxSceneComponentClasses = 64;
static_assert(kMaxSceneComponentClasses < 128, "It can oly be 7 bits because of SceneComponent::m_classId");

#define ANKI_SCENE_COMPONENT_VIRTUAL(name, type) using SceneComponentCallback_##name = type;
#include <AnKi/Scene/Components/SceneComponentVirtuals.defs.h>

class SceneComponentVtable
{
public:
#define ANKI_SCENE_COMPONENT_VIRTUAL(name, type) SceneComponentCallback_##name m_##name;
#include <AnKi/Scene/Components/SceneComponentVirtuals.defs.h>
};

/// Callbacks live in their own arrays for better caching.
class SceneComponentCallbacks
{
public:
#define ANKI_SCENE_COMPONENT_VIRTUAL(name, type) SceneComponentCallback_##name m_##name[kMaxSceneComponentClasses];
#include <AnKi/Scene/Components/SceneComponentVirtuals.defs.h>
};

extern SceneComponentCallbacks g_sceneComponentCallbacks;

/// Scene component class info.
class SceneComponentRtti
{
public:
	const char* m_className;
	U16 m_classSize;
	U8 m_classAlignment;
	U8 m_classId;

	SceneComponentRtti(const char* name, U32 size, U32 alignment, SceneComponentVtable vtable);
};

/// Define a scene component.
#define ANKI_SCENE_COMPONENT(className) \
	static SceneComponentRtti _m_rtti; \
	static void _construct(SceneComponent& self, SceneNode& node) \
	{ \
		callConstructor(static_cast<className&>(self), &node); \
	} \
	static void _destruct(SceneComponent& self) \
	{ \
		callDestructor(static_cast<className&>(self)); \
	} \
	static void _onDestroy(SceneComponent& self, SceneNode& node) \
	{ \
		static_cast<className&>(self).onDestroy(node); \
	} \
	static Error _update(SceneComponent& self, SceneComponentUpdateInfo& info, Bool& updated) \
	{ \
		return static_cast<className&>(self).update(info, updated); \
	} \
	static void _onOtherComponentRemovedOrAdded(SceneComponent& self, SceneComponent* other, Bool added) \
	{ \
		static_cast<className&>(self).onOtherComponentRemovedOrAdded(other, added); \
	} \
\
public: \
	static U8 getStaticClassId() \
	{ \
		return _m_rtti.m_classId; \
	} \
\
private:

/// Define the statics of a scene component.
#define ANKI_SCENE_COMPONENT_STATICS(className) \
	SceneComponentRtti className::_m_rtti(ANKI_STRINGIZE(className), sizeof(className), alignof(className), \
										  {className::_construct, className::_destruct, className::_onDestroy, \
										   className::_update, className::_onOtherComponentRemovedOrAdded});

/// Passed to SceneComponent::update.
/// @memberof SceneComponent
class SceneComponentUpdateInfo
{
public:
	SceneNode* m_node = nullptr;
	const Second m_previousTime;
	const Second m_currentTime;
	const Second m_dt;
	StackMemoryPool* m_framePool = nullptr;
	GpuSceneMicroPatcher* m_gpuSceneMicroPatcher = nullptr;

	SceneComponentUpdateInfo(Second prevTime, Second crntTime)
		: m_previousTime(prevTime)
		, m_currentTime(crntTime)
		, m_dt(crntTime - prevTime)
	{
	}
};

/// Scene node component.
/// @note Doesn't have C++ virtuals to keep it compact.
class SceneComponent
{
public:
	/// Construct the scene component.
	SceneComponent(SceneNode* node, U8 classId, Bool isFeedbackComponent = false);

	U8 getClassId() const
	{
		return m_classId;
	}

	Timestamp getTimestamp() const
	{
		return m_timestamp;
	}

	Bool isFeedbackComponent() const
	{
		return m_feedbackComponent;
	}

	static const SceneComponentRtti& findClassRtti(CString className);

	static const SceneComponentRtti& getClassRtti(U8 classId);

	ANKI_INTERNAL void onDestroyReal(SceneNode& node)
	{
		g_sceneComponentCallbacks.m_onDestroy[m_classId](*this, node);
	}

	ANKI_INTERNAL Error updateReal(SceneComponentUpdateInfo& info, Bool& updated)
	{
		return g_sceneComponentCallbacks.m_update[m_classId](*this, info, updated);
	}

	/// Don't call it directly.
	ANKI_INTERNAL void setTimestamp(Timestamp timestamp)
	{
		ANKI_ASSERT(timestamp > 0);
		ANKI_ASSERT(timestamp >= m_timestamp);
		m_timestamp = timestamp;
	}

protected:
	static SceneGraphExternalSubsystems& getExternalSubsystems(const SceneNode& node);

	/// Pseudo-virtual
	void onDestroy([[maybe_unused]] SceneNode& node)
	{
		// Do nothing
	}

	/// Pseudo-virtual to update the component.
	/// @param[in,out] info Update info.
	/// @param[out] updated true if an update happened.
	Error update([[maybe_unused]] SceneComponentUpdateInfo& info, Bool& updated)
	{
		updated = false;
		return Error::kNone;
	}

	/// Called when a component is added or removed in the SceneNode.
	/// @param other The component that was inserted.
	/// @param added Was it inserted or removed?
	void onOtherComponentRemovedOrAdded([[maybe_unused]] SceneComponent* other, [[maybe_unused]] Bool added)
	{
	}

private:
	Timestamp m_timestamp = 1; ///< Indicates when an update happened
	U8 m_classId : 7; ///< Cache the type ID.
	U8 m_feedbackComponent : 1;
};
/// @}

} // end namespace anki
