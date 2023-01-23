// Copyright (C) 2009-2022, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Scene/Components/SpatialComponent.h>
#include <AnKi/Scene/SceneNode.h>
#include <AnKi/Scene/SceneGraph.h>

namespace anki {

SpatialComponent::SpatialComponent(SceneNode* node)
	: SceneComponent(node, getStaticClassId())
	, m_node(node)
	, m_markedForUpdate(true)
	, m_placed(false)
	, m_updateOctreeBounds(true)
	, m_alwaysVisible(false)
{
	ANKI_ASSERT(node);
	m_octreeInfo.m_userData = this;
	setAabbWorldSpace(Aabb(Vec3(-1.0f), Vec3(1.0f)));

	getExternalSubsystems(*node).m_gpuSceneMemoryPool->allocate(sizeof(Vec3) * 2, alignof(F32), m_gpuSceneAabb);
}

SpatialComponent::~SpatialComponent()
{
	if(m_placed)
	{
		m_node->getSceneGraph().getOctree().remove(m_octreeInfo);
	}

	m_convexHullPoints.destroy(m_node->getMemoryPool());

	getExternalSubsystems(*m_node).m_gpuSceneMemoryPool->free(m_gpuSceneAabb);
}

void SpatialComponent::setConvexHullWorldSpace(const ConvexHullShape& hull)
{
	ANKI_ASSERT(hull.getPoints().getSize() > 0);

	if(m_convexHullPoints.getSize() != hull.getPoints().getSize())
	{
		m_convexHullPoints.resize(m_node->getMemoryPool(), hull.getPoints().getSize());
	}

	memcpy(&m_convexHullPoints[0], &hull.getPoints()[0], hull.getPoints().getSizeInBytes());

	m_hull = ConvexHullShape(&m_convexHullPoints[0], m_convexHullPoints.getSize());
	if(!hull.isTransformIdentity())
	{
		m_hull.setTransform(hull.getTransform());
	}

	m_collisionObjectType = hull.kClassType;
	m_markedForUpdate = true;
}

Error SpatialComponent::update([[maybe_unused]] SceneComponentUpdateInfo& info, Bool& updated)
{
	ANKI_ASSERT(info.m_node == m_node);

	updated = m_markedForUpdate;
	if(updated)
	{
		if(!m_alwaysVisible)
		{
			// Compute the AABB
			switch(m_collisionObjectType)
			{
			case CollisionShapeType::kAabb:
				m_derivedAabb = m_aabb;
				break;
			case CollisionShapeType::kObb:
				m_derivedAabb = computeAabb(m_obb);
				break;
			case CollisionShapeType::kSphere:
				m_derivedAabb = computeAabb(m_sphere);
				break;
			case CollisionShapeType::kConvexHull:
				m_derivedAabb = computeAabb(m_hull);
				break;
			default:
				ANKI_ASSERT(0);
			}

			m_node->getSceneGraph().getOctree().place(m_derivedAabb, &m_octreeInfo, m_updateOctreeBounds);
		}
		else
		{
			m_node->getSceneGraph().getOctree().placeAlwaysVisible(&m_octreeInfo);

			// Set something random
			m_derivedAabb = Aabb(Vec3(-1.0f), Vec3(1.0f));
		}

		m_markedForUpdate = false;
		m_placed = true;

		// Update the GPU scene
		Array<Vec3, 2> aabb;
		aabb[0] = m_derivedAabb.getMin().xyz();
		aabb[1] = m_derivedAabb.getMax().xyz();
		info.m_gpuSceneMicroPatcher->newCopy(*info.m_framePool, m_gpuSceneAabb.m_offset, sizeof(aabb), &aabb[0]);
	}

	m_octreeInfo.reset();

	return Error::kNone;
}

} // end namespace anki
