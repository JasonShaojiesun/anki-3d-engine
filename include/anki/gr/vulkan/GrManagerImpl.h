// Copyright (C) 2009-2016, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <anki/gr/vulkan/Common.h>
#include <anki/gr/vulkan/GpuMemoryAllocator.h>
#include <anki/gr/vulkan/ObjectRecycler.h>
#include <anki/util/HashMap.h>

namespace anki
{

/// @addtogroup vulkan
/// @{

/// Vulkan implementation of GrManager.
class GrManagerImpl
{
public:
	GrManagerImpl(GrManager* manager)
		: m_manager(manager)
	{
		ANKI_ASSERT(manager);
	}

	~GrManagerImpl();

	ANKI_USE_RESULT Error init(const GrManagerInitInfo& cfg);

	/// Get or create a compatible render pass for a pipeline.
	VkRenderPass getOrCreateCompatibleRenderPass(const PipelineInitInfo& init);

	GrAllocator<U8> getAllocator() const;

	void beginFrame();

	void endFrame();

	VkDevice getDevice() const
	{
		ANKI_ASSERT(m_device);
		return m_device;
	}

	VkPipelineLayout getGlobalPipelineLayout() const
	{
		ANKI_ASSERT(m_globalPipelineLayout);
		return m_globalPipelineLayout;
	}

	VkCommandBuffer newCommandBuffer(Thread::Id tid, Bool secondLevel);

	void deleteCommandBuffer(
		VkCommandBuffer cmdb, Bool secondLevel, Thread::Id tid);

private:
	GrManager* m_manager = nullptr;

	U64 m_frame = 0;

	VkInstance m_instance = VK_NULL_HANDLE;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	U32 m_queueIdx = MAX_U32;
	VkQueue m_queue = VK_NULL_HANDLE;

	/// @name Surface_related
	/// @{
	class PerFrame
	{
	public:
		VkImage m_image = VK_NULL_HANDLE;
		VkImageView m_imageView = VK_NULL_HANDLE;
		VkFramebuffer m_fb = VK_NULL_HANDLE;

		VkFence m_presentFence = VK_NULL_HANDLE;
		VkSemaphore m_acquireSemaphore = VK_NULL_HANDLE;

		/// The semaphores that of those submits that render to the default FB.
		DynamicArray<VkSemaphore> m_renderSemaphores;
		U32 m_renderSemaphoresCount = 0;
	};

	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	U32 m_surfaceWidth = 0, m_surfaceHeight = 0;
	VkFormat m_surfaceFormat = VK_FORMAT_UNDEFINED;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	Array<PerFrame, MAX_FRAMES_IN_FLIGHT> m_perFrame;
	/// @}

	VkDescriptorSetLayout m_globalDescriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout m_globalPipelineLayout = VK_NULL_HANDLE;

	/// Map for compatible render passes.
	class CompatibleRenderPassHashMap;
	CompatibleRenderPassHashMap* m_renderPasses = nullptr;

	/// @name Memory
	/// @{
	VkPhysicalDeviceMemoryProperties m_memoryProperties;

	/// One for each mem type.
	DynamicArray<GpuMemoryAllocator> m_gpuMemAllocs;
	/// @}

	/// @name Per_thread_cache
	/// @{

	class PerThreadHasher
	{
	public:
		U64 operator()(const Thread::Id& b) const
		{
			return b;
		}
	};

	class PerThreadCompare
	{
	public:
		Bool operator()(const Thread::Id& a, const Thread::Id& b) const
		{
			return a == b;
		}
	};

	/// Per thread cache.
	class PerThread
	{
	public:
		CommandBufferObjectRecycler m_cmdbs;
	};

	HashMap<Thread::Id, PerThread, PerThreadHasher, PerThreadCompare>
		m_perThread;
	SpinLock m_perThreadMtx;
	/// @}

	ANKI_USE_RESULT Error initInternal(const GrManagerInitInfo& init);
	ANKI_USE_RESULT Error initInstance(const GrManagerInitInfo& init);
	ANKI_USE_RESULT Error initSurface(const GrManagerInitInfo& init);
	ANKI_USE_RESULT Error initDevice(const GrManagerInitInfo& init);
	ANKI_USE_RESULT Error initSwapchain(const GrManagerInitInfo& init);
	ANKI_USE_RESULT Error initFramebuffers(const GrManagerInitInfo& init);
	ANKI_USE_RESULT Error initGlobalDsetLayout();
	ANKI_USE_RESULT Error initGlobalPplineLayout();
	void initMemory();

	/// Find a suitable memory type.
	U findMemoryType(U resourceMemTypeBits,
		VkMemoryPropertyFlags preferFlags,
		VkMemoryPropertyFlags avoidFlags) const;

	static void* allocateCallback(void* userData,
		size_t size,
		size_t alignment,
		VkSystemAllocationScope allocationScope);

	static void* reallocateCallback(void* userData,
		void* original,
		size_t size,
		size_t alignment,
		VkSystemAllocationScope allocationScope);

	static void freeCallback(void* userData, void* ptr);

	void resetFrame(PerFrame& frame);

	PerThread& getPerThreadCache(Thread::Id tid);
};
/// @}

} // end namespace anki
