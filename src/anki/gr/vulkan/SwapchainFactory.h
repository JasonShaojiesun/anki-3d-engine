// Copyright (C) 2009-2017, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <anki/gr/vulkan/FenceFactory.h>
#include <anki/util/Ptr.h>

namespace anki
{

// Forward
class SwapchainFactory;

/// @addtogroup vulkan
/// @{

/// A wrapper for the swapchain.
class MicroSwapchain
{
	friend class MicroSwapchainPtrDeleter;
	friend class SwapchainFactory;

public:
	VkSwapchainKHR m_swapchain = {};

	Array<VkImage, MAX_FRAMES_IN_FLIGHT> m_images = {};
	Array<VkImageView, MAX_FRAMES_IN_FLIGHT> m_imageViews = {};

	VkFormat m_surfaceFormat = {};

	U32 m_surfaceWidth = 0;
	U32 m_surfaceHeight = 0;

	Array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> m_framebuffers = {};

	U8 m_currentBackbufferIndex = 0;

	MicroSwapchain(SwapchainFactory* factory);

	~MicroSwapchain();

	Atomic<U32>& getRefcount()
	{
		return m_refcount;
	}

	GrAllocator<U8> getAllocator() const;

	void setFence(MicroFencePtr fence)
	{
		m_fence = fence;
	}

	VkRenderPass getRenderPass(VkAttachmentLoadOp loadOp) const
	{
		const U idx = (loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE) ? RPASS_LOAD_DONT_CARE : RPASS_LOAD_CLEAR;
		return m_rpasses[idx];
	}

private:
	Atomic<U32> m_refcount = {0};
	SwapchainFactory* m_factory = nullptr;

	enum
	{
		RPASS_LOAD_CLEAR,
		RPASS_LOAD_DONT_CARE,
		RPASS_COUNT
	};

	Array<VkRenderPass, RPASS_COUNT> m_rpasses = {};

	MicroFencePtr m_fence;

	ANKI_USE_RESULT Error initInternal();
};

/// Deleter for MicroSwapchainPtr smart pointer.
class MicroSwapchainPtrDeleter
{
public:
	void operator()(MicroSwapchain* x);
};

/// MicroSwapchain smart pointer.
using MicroSwapchainPtr = IntrusivePtr<MicroSwapchain, MicroSwapchainPtrDeleter>;

/// Swapchain factory.
class SwapchainFactory
{
	friend class MicroSwapchainPtrDeleter;
	friend class MicroSwapchain;

public:
	void init(GrManagerImpl* manager, Bool vsync)
	{
		ANKI_ASSERT(manager);
		m_gr = manager;
		m_vsync = vsync;
	}

	void destroy();

	MicroSwapchainPtr newInstance();

private:
	GrManagerImpl* m_gr = nullptr;
	Bool8 m_vsync = false;

	Mutex m_mtx;

	DynamicArray<MicroSwapchain*> m_swapchains;
	U32 m_swapchainCount = 0;
#if ANKI_EXTRA_CHECKS
	U32 m_swapchainsInFlight = 0;
#endif

	void destroySwapchain(MicroSwapchain* schain);

	void releaseFences();
};
/// @}

inline void MicroSwapchainPtrDeleter::operator()(MicroSwapchain* s)
{
	ANKI_ASSERT(s);
	s->m_factory->destroySwapchain(s);
}

} // end namespace anki