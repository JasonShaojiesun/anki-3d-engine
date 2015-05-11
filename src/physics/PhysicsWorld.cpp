// Copyright (C) 2009-2015, Panagiotis Christopoulos Charitos.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include "anki/physics/PhysicsWorld.h"
#include "anki/physics/PhysicsPlayerController.h"
#include "anki/physics/PhysicsCollisionShape.h"

namespace anki {

//==============================================================================
// Ugly but there is no other way
static HeapAllocator<U8>* gAlloc = nullptr;

static void* newtonAlloc(int size)
{
	return gAlloc->allocate(size);
}

static void newtonFree(void* const ptr, int size)
{
	gAlloc->deallocate(ptr, size);
}

//==============================================================================
PhysicsWorld::PhysicsWorld()
{}

//==============================================================================
PhysicsWorld::~PhysicsWorld()
{
	if(m_sceneBody)
	{
		NewtonDestroyBody(m_sceneBody);
		m_sceneBody = nullptr;
	}

	if(m_sceneCollision)
	{
		NewtonDestroyCollision(m_sceneCollision);
		m_sceneCollision = nullptr;
	}

	if(m_world)
	{
		NewtonDestroy(m_world);
		m_world = nullptr;
	}

	gAlloc = nullptr;
}

//==============================================================================
Error PhysicsWorld::create(AllocAlignedCallback allocCb, void* allocCbData)
{
	Error err = ErrorCode::NONE;

	m_alloc = HeapAllocator<U8>(allocCb, allocCbData);

	// Set allocators
	gAlloc = &m_alloc;
	NewtonSetMemorySystem(newtonAlloc, newtonFree);

	// Initialize world
	m_world = NewtonCreate();
	if(!m_world)
	{
		ANKI_LOGE("NewtonCreate() failed");
		return ErrorCode::FUNCTION_FAILED;
	}

	// Set the simplified solver mode (faster but less accurate)
	NewtonSetSolverModel(m_world, 1);

	// Create scene collision
	m_sceneCollision = NewtonCreateSceneCollision(m_world, 0);
	Mat4 trf = Mat4::getIdentity();
	m_sceneBody = NewtonCreateDynamicBody(m_world, m_sceneCollision, &trf[0]);

	// Set the post update listener
	NewtonWorldAddPostListener(m_world, "world", this, postUpdateCallback,
		destroyCallback);

	return err;
}

//==============================================================================
Error PhysicsWorld::updateAsync(F32 dt)
{
	m_dt = dt;

	// Do cleanup of marked for deletion
	cleanupMarkedForDeletion();

	// Update
	NewtonUpdateAsync(m_world, dt);

	return ErrorCode::NONE;
}

//==============================================================================
void PhysicsWorld::waitUpdate()
{
	NewtonWaitForUpdateToFinish(m_world);
}

//==============================================================================
void PhysicsWorld::cleanupMarkedForDeletion()
{
	LockGuard<Mutex> lock(m_mtx);

	while(!m_forDeletion.isEmpty())
	{
		auto it = m_forDeletion.getBegin();
		PhysicsObject* obj = *it;

		// Remove from objects marked for deletion
		m_forDeletion.erase(m_alloc, it);

		// Remove from player controllers
		if(obj->getType() == PhysicsObject::Type::PLAYER_CONTROLLER)
		{

			auto it2 = m_playerControllers.getBegin();
			for(; it2 != m_playerControllers.getEnd(); ++it2)
			{
				PhysicsObject* obj2 = *it2;
				if(obj2 == obj)
				{
					break;
				}
			}

			ANKI_ASSERT(it2 != m_playerControllers.getEnd());
			m_playerControllers.erase(m_alloc, it2);
		}

		// Finaly, delete it
		m_alloc.deleteInstance(obj);
	}
}

//==============================================================================
void PhysicsWorld::postUpdate(F32 dt)
{
	for(PhysicsPlayerController* player : m_playerControllers)
	{
		NewtonDispachThreadJob(m_world,
			PhysicsPlayerController::postUpdateKernelCallback, player);
	}
}

//==============================================================================
void PhysicsWorld::registerObject(PhysicsObject* ptr)
{
	if(isa<PhysicsPlayerController>(ptr))
	{
		LockGuard<Mutex> lock(m_mtx);
		m_playerControllers.pushBack(
			m_alloc, dcast<PhysicsPlayerController*>(ptr));
	}
}

} // end namespace anki
