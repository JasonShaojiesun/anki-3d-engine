// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Renderer/GpuVisibility.h>
#include <AnKi/Renderer/Renderer.h>
#include <AnKi/Renderer/HiZ.h>
#include <AnKi/Scene/RenderStateBucket.h>
#include <AnKi/Scene/ContiguousArrayAllocator.h>
#include <AnKi/Core/GpuMemory/GpuVisibleTransientMemoryPool.h>
#include <AnKi/Core/GpuMemory/RebarTransientMemoryPool.h>
#include <AnKi/Core/GpuMemory/GpuSceneBuffer.h>
#include <AnKi/Collision/Functions.h>
#include <AnKi/Shaders/Include/MiscRendererTypes.h>
#include <AnKi/Core/ConfigSet.h>

namespace anki {

Error GpuVisibility::init()
{
	ANKI_CHECK(loadShaderProgram("ShaderBinaries/GpuVisibility.ankiprogbin", m_prog, m_grProg));

	return Error::kNone;
}

void GpuVisibility::populateRenderGraph(RenderingContext& ctx)
{
	RenderGraphDescription& rgraph = ctx.m_renderGraphDescr;

	const U32 aabbCount = GpuSceneContiguousArrays::getSingleton().getElementCount(GpuSceneContiguousArrayType::kRenderableBoundingVolumesGBuffer);
	const U32 bucketCount = RenderStateBucketContainer::getSingleton().getBucketCount(RenderingTechnique::kGBuffer);

	// Allocate memory for the indirect commands
	const GpuVisibleTransientMemoryAllocation indirectArgs =
		GpuVisibleTransientMemoryPool::getSingleton().allocate(aabbCount * sizeof(DrawIndexedIndirectArgs));
	const GpuVisibleTransientMemoryAllocation instanceRateRenderables =
		GpuVisibleTransientMemoryPool::getSingleton().allocate(aabbCount * sizeof(GpuSceneRenderable));

	// Allocate and zero the MDI counts
	RebarAllocation mdiDrawCounts;
	U32* atomics = RebarTransientMemoryPool::getSingleton().allocateFrame<U32>(bucketCount, mdiDrawCounts);
	memset(atomics, 0, mdiDrawCounts.m_range);

	// Import buffers
	m_runCtx.m_instanceRateRenderables = rgraph.importBuffer(instanceRateRenderables.m_buffer, BufferUsageBit::kNone,
															 instanceRateRenderables.m_offset, instanceRateRenderables.m_size);
	m_runCtx.m_drawIndexedIndirectArgs =
		rgraph.importBuffer(indirectArgs.m_buffer, BufferUsageBit::kNone, indirectArgs.m_offset, indirectArgs.m_size);
	m_runCtx.m_mdiDrawCounts = rgraph.importBuffer(&RebarTransientMemoryPool::getSingleton().getBuffer(), BufferUsageBit::kNone,
												   mdiDrawCounts.m_offset, mdiDrawCounts.m_range);

	// Create the renderpass
	ComputeRenderPassDescription& pass = rgraph.newComputeRenderPass("GPU occlusion GBuffer");

	pass.newBufferDependency(getRenderer().getGpuSceneBufferHandle(), BufferUsageBit::kStorageComputeRead);
	pass.newTextureDependency(getRenderer().getHiZ().getHiZRt(), TextureUsageBit::kSampledCompute);
	pass.newBufferDependency(m_runCtx.m_instanceRateRenderables, BufferUsageBit::kStorageComputeWrite);
	pass.newBufferDependency(m_runCtx.m_drawIndexedIndirectArgs, BufferUsageBit::kStorageComputeWrite);
	pass.newBufferDependency(m_runCtx.m_mdiDrawCounts, BufferUsageBit::kStorageComputeWrite);

	pass.setWork([this, &ctx](RenderPassWorkContext& rpass) {
		CommandBuffer& cmdb = *rpass.m_commandBuffer;

		cmdb.bindShaderProgram(m_grProg.get());

		cmdb.bindStorageBuffer(
			0, 0, &GpuSceneBuffer::getSingleton().getBuffer(),
			GpuSceneContiguousArrays::getSingleton().getArrayBase(GpuSceneContiguousArrayType::kRenderableBoundingVolumesGBuffer),
			GpuSceneContiguousArrays::getSingleton().getElementCount(GpuSceneContiguousArrayType::kRenderableBoundingVolumesGBuffer)
				* sizeof(GpuSceneRenderableAabb));

		cmdb.bindStorageBuffer(0, 1, &GpuSceneBuffer::getSingleton().getBuffer(),
							   GpuSceneContiguousArrays::getSingleton().getArrayBase(GpuSceneContiguousArrayType::kRenderables),
							   GpuSceneContiguousArrays::getSingleton().getElementCount(GpuSceneContiguousArrayType::kRenderables)
								   * sizeof(GpuSceneRenderable));

		cmdb.bindStorageBuffer(0, 2, &GpuSceneBuffer::getSingleton().getBuffer(), 0, kMaxPtrSize);

		rpass.bindColorTexture(0, 3, getRenderer().getHiZ().getHiZRt());

		rpass.bindStorageBuffer(0, 4, m_runCtx.m_instanceRateRenderables);
		rpass.bindStorageBuffer(0, 5, m_runCtx.m_drawIndexedIndirectArgs);

		U32* offsets = allocateAndBindStorage<U32*>(
			sizeof(U32) * RenderStateBucketContainer::getSingleton().getBucketCount(RenderingTechnique::kGBuffer), cmdb, 0, 6);
		U32 bucketCount = 0;
		U32 userCount = 0;
		RenderStateBucketContainer::getSingleton().iterateBuckets(RenderingTechnique::kGBuffer, [&](const RenderStateInfo&, U32 userCount_) {
			offsets[bucketCount] = userCount;
			userCount += userCount_;
			++bucketCount;
		});
		ANKI_ASSERT(userCount == RenderStateBucketContainer::getSingleton().getBucketsItemCount(RenderingTechnique::kGBuffer));

		rpass.bindStorageBuffer(0, 7, m_runCtx.m_mdiDrawCounts);

		GpuVisibilityUniforms* unis = allocateAndBindUniforms<GpuVisibilityUniforms*>(sizeof(GpuVisibilityUniforms), cmdb, 0, 8);

		Array<Plane, 6> planes;
		extractClipPlanes(ctx.m_matrices.m_viewProjection, planes);
		for(U32 i = 0; i < 6; ++i)
		{
			unis->m_clipPlanes[i] = Vec4(planes[i].getNormal().xyz(), planes[i].getOffset());
		}

		const U32 aabbCount =
			GpuSceneContiguousArrays::getSingleton().getElementCount(GpuSceneContiguousArrayType::kRenderableBoundingVolumesGBuffer);
		unis->m_aabbCount = aabbCount;

		ANKI_ASSERT(kMaxLodCount == 3);
		unis->m_maxLodDistances[0] = ConfigSet::getSingleton().getLod0MaxDistance();
		unis->m_maxLodDistances[1] = ConfigSet::getSingleton().getLod1MaxDistance();
		unis->m_maxLodDistances[2] = kMaxF32;
		unis->m_maxLodDistances[3] = kMaxF32;

		unis->m_cameraOrigin = ctx.m_matrices.m_cameraTransform.getTranslationPart().xyz();

		dispatchPPCompute(cmdb, 64, 1, aabbCount, 1);
	});
}

} // end namespace anki
