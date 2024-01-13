// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Renderer/Utils/GpuVisibility.h>
#include <AnKi/Renderer/Renderer.h>
#include <AnKi/Scene/RenderStateBucket.h>
#include <AnKi/Scene/GpuSceneArray.h>
#include <AnKi/Core/GpuMemory/GpuVisibleTransientMemoryPool.h>
#include <AnKi/Core/GpuMemory/RebarTransientMemoryPool.h>
#include <AnKi/Core/GpuMemory/GpuSceneBuffer.h>
#include <AnKi/Collision/Functions.h>
#include <AnKi/Shaders/Include/GpuVisibilityTypes.h>
#include <AnKi/Core/GpuMemory/UnifiedGeometryBuffer.h>
#include <AnKi/Core/StatsSet.h>

namespace anki {

GpuVisibilityCommonBase::Counts GpuVisibilityCommonBase::countTechnique(RenderingTechnique t)
{
	Counts out = {};

	switch(t)
	{
	case RenderingTechnique::kGBuffer:
		out.m_aabbCount = GpuSceneArrays::RenderableBoundingVolumeGBuffer::getSingleton().getElementCount();
		break;
	case RenderingTechnique::kDepth:
		out.m_aabbCount = GpuSceneArrays::RenderableBoundingVolumeDepth::getSingleton().getElementCount();
		break;
	case RenderingTechnique::kForward:
		out.m_aabbCount = GpuSceneArrays::RenderableBoundingVolumeForward::getSingleton().getElementCount();
		break;
	default:
		ANKI_ASSERT(0);
	}

	out.m_bucketCount = RenderStateBucketContainer::getSingleton().getBucketCount(t);

	RenderStateBucketContainer::getSingleton().iterateBuckets(t, [&](const RenderStateInfo&, U32 userCount, U32 meshletGroupCount_) {
		if(meshletGroupCount_)
		{
			out.m_modernGeometryFlowUserCount += userCount;
			out.m_meshletGroupCount += min(meshletGroupCount_, kMaxMeshletGroupCountPerRenderStateBucket);
		}
		else
		{
			out.m_legacyGeometryFlowUserCount += userCount;
		}
	});

	out.m_allUserCount = out.m_legacyGeometryFlowUserCount + out.m_modernGeometryFlowUserCount;

	return out;
}

Error GpuVisibility::init()
{
	for(MutatorValue hzb = 0; hzb < 2; ++hzb)
	{
		for(MutatorValue gatherAabbs = 0; gatherAabbs < 2; ++gatherAabbs)
		{
			for(MutatorValue genHash = 0; genHash < 2; ++genHash)
			{
				ANKI_CHECK(loadShaderProgram("ShaderBinaries/GpuVisibility.ankiprogbin",
											 {{"HZB_TEST", hzb}, {"DISTANCE_TEST", 0}, {"GATHER_AABBS", gatherAabbs}, {"HASH_VISIBLES", genHash}},
											 m_prog, m_frustumGrProgs[hzb][gatherAabbs][genHash]));
			}
		}
	}

	for(MutatorValue gatherAabbs = 0; gatherAabbs < 2; ++gatherAabbs)
	{
		for(MutatorValue genHash = 0; genHash < 2; ++genHash)
		{
			ANKI_CHECK(loadShaderProgram("ShaderBinaries/GpuVisibility.ankiprogbin",
										 {{"HZB_TEST", 0}, {"DISTANCE_TEST", 1}, {"GATHER_AABBS", gatherAabbs}, {"HASH_VISIBLES", genHash}}, m_prog,
										 m_distGrProgs[gatherAabbs][genHash]));
		}
	}

	return Error::kNone;
}

void GpuVisibility::populateRenderGraphInternal(Bool distanceBased, BaseGpuVisibilityInput& in, GpuVisibilityOutput& out)
{
	ANKI_ASSERT(in.m_lodReferencePoint.x() != kMaxF32);
	RenderGraphDescription& rgraph = *in.m_rgraph;

	class DistanceTestData
	{
	public:
		Vec3 m_pointOfTest;
		F32 m_testRadius;
	};

	class FrustumTestData
	{
	public:
		RenderTargetHandle m_hzbRt;
		Mat4 m_viewProjMat;
		UVec2 m_finalRenderTargetSize;
	};

	FrustumTestData* frustumTestData = nullptr;
	DistanceTestData* distTestData = nullptr;

	if(distanceBased)
	{
		distTestData = newInstance<DistanceTestData>(getRenderer().getFrameMemoryPool());
		const DistanceGpuVisibilityInput& din = static_cast<DistanceGpuVisibilityInput&>(in);
		distTestData->m_pointOfTest = din.m_pointOfTest;
		distTestData->m_testRadius = din.m_testRadius;
	}
	else
	{
		frustumTestData = newInstance<FrustumTestData>(getRenderer().getFrameMemoryPool());
		const FrustumGpuVisibilityInput& fin = static_cast<FrustumGpuVisibilityInput&>(in);
		frustumTestData->m_viewProjMat = fin.m_viewProjectionMatrix;
		frustumTestData->m_finalRenderTargetSize = fin.m_finalRenderTargetSize;
	}

	const Counts counts = countTechnique(in.m_technique);

	if(counts.m_allUserCount == 0) [[unlikely]]
	{
		// Early exit
		return;
	}

	// Allocate memory
	const Bool firstFrame = m_runCtx.m_frameIdx != getRenderer().getFrameCount();
	if(firstFrame)
	{
		// Allocate the big buffers once at the beginning of the frame

		m_runCtx.m_frameIdx = getRenderer().getFrameCount();
		m_runCtx.m_populateRenderGraphFrameCallCount = 0;

		// Find the max counts of all techniques
		Counts maxCounts = {};
		for(RenderingTechnique t : EnumBitsIterable<RenderingTechnique, RenderingTechniqueBit>(RenderingTechniqueBit::kAllRaster))
		{
			maxCounts = maxCounts.max((in.m_technique == t) ? counts : countTechnique(t));
		}

		// Allocate memory
		for(PersistentMemory& mem : m_runCtx.m_persistentMem)
		{
			mem = {};

			mem.m_drawIndexedIndirectArgsBuffer = GpuVisibleTransientMemoryPool::getSingleton().allocate(
				max(1u, maxCounts.m_legacyGeometryFlowUserCount) * sizeof(DrawIndexedIndirectArgs));
			mem.m_renderableInstancesBuffer = GpuVisibleTransientMemoryPool::getSingleton().allocate(max(1u, maxCounts.m_legacyGeometryFlowUserCount)
																									 * sizeof(GpuSceneRenderableVertex));

			mem.m_taskShaderPayloadBuffer =
				GpuVisibleTransientMemoryPool::getSingleton().allocate(max(1u, maxCounts.m_meshletGroupCount) * sizeof(GpuSceneTaskShaderPayload));
		}
	}

	PersistentMemory& mem = m_runCtx.m_persistentMem[m_runCtx.m_populateRenderGraphFrameCallCount % m_runCtx.m_persistentMem.getSize()];
	++m_runCtx.m_populateRenderGraphFrameCallCount;

	out.m_legacy.m_drawIndexedIndirectArgsBuffer = mem.m_drawIndexedIndirectArgsBuffer;
	out.m_legacy.m_drawIndexedIndirectArgsBuffer.m_range = max(1u, counts.m_legacyGeometryFlowUserCount) * sizeof(DrawIndexedIndirectArgs);

	out.m_legacy.m_renderableInstancesBuffer = mem.m_renderableInstancesBuffer;
	out.m_legacy.m_renderableInstancesBuffer.m_range = max(1u, counts.m_legacyGeometryFlowUserCount) * sizeof(GpuSceneRenderableVertex);

	out.m_legacy.m_mdiDrawCountsBuffer = GpuVisibleTransientMemoryPool::getSingleton().allocate(sizeof(U32) * counts.m_bucketCount);

	out.m_mesh.m_taskShaderPayloadBuffer = mem.m_taskShaderPayloadBuffer;
	out.m_mesh.m_taskShaderPayloadBuffer.m_range = max(1u, counts.m_meshletGroupCount) * sizeof(GpuSceneTaskShaderPayload);

	out.m_mesh.m_taskShaderIndirectArgsBuffer =
		GpuVisibleTransientMemoryPool::getSingleton().allocate(sizeof(DispatchIndirectArgs) * counts.m_bucketCount);

	if(in.m_hashVisibles)
	{
		out.m_visiblesHashBuffer = GpuVisibleTransientMemoryPool::getSingleton().allocate(sizeof(GpuVisibilityHash));
	}

	if(in.m_gatherAabbIndices)
	{
		out.m_visibleAaabbIndicesBuffer = GpuVisibleTransientMemoryPool::getSingleton().allocate((counts.m_allUserCount + 1) * sizeof(U32));
	}

	// Zero some stuff
	const BufferHandle zeroStuffDependency = rgraph.importBuffer(BufferUsageBit::kNone, out.m_legacy.m_mdiDrawCountsBuffer);
	{
		Array<Char, 128> passName;
		snprintf(passName.getBegin(), passName.getSizeInBytes(), "GPU vis zero: %s", in.m_passesName.cstr());

		ComputeRenderPassDescription& pass = rgraph.newComputeRenderPass(passName.getBegin());
		pass.newBufferDependency(zeroStuffDependency, BufferUsageBit::kTransferDestination);

		pass.setWork([out](RenderPassWorkContext& rpass) {
			CommandBuffer& cmdb = *rpass.m_commandBuffer;

			cmdb.pushDebugMarker("MDI counts", Vec3(1.0f, 1.0f, 1.0f));
			cmdb.fillBuffer(out.m_legacy.m_mdiDrawCountsBuffer, 0);
			cmdb.popDebugMarker();

			if(out.m_mesh.m_taskShaderIndirectArgsBuffer.m_buffer)
			{
				cmdb.pushDebugMarker("Task shader indirect args", Vec3(1.0f, 1.0f, 1.0f));
				cmdb.fillBuffer(out.m_mesh.m_taskShaderIndirectArgsBuffer, 0);
				cmdb.popDebugMarker();
			}

			if(out.m_visiblesHashBuffer.m_buffer)
			{
				cmdb.pushDebugMarker("Visibles hash", Vec3(1.0f, 1.0f, 1.0f));
				cmdb.fillBuffer(out.m_visiblesHashBuffer, 0);
				cmdb.popDebugMarker();
			}

			if(out.m_visibleAaabbIndicesBuffer.m_buffer)
			{
				cmdb.pushDebugMarker("Visible AABB indices", Vec3(1.0f, 1.0f, 1.0f));
				cmdb.fillBuffer(out.m_visibleAaabbIndicesBuffer.m_buffer, out.m_visibleAaabbIndicesBuffer.m_offset, sizeof(U32), 0);
				cmdb.popDebugMarker();
			}
		});
	}

	// Set the out dependency. Use one of the big buffers.
	if(!mem.m_bufferDepedency.isValid())
	{
		mem.m_bufferDepedency = rgraph.importBuffer(BufferUsageBit::kNone, mem.m_drawIndexedIndirectArgsBuffer);
	}
	out.m_someBufferHandle = mem.m_bufferDepedency;

	// Create the renderpass
	Array<Char, 128> passName;
	snprintf(passName.getBegin(), passName.getSizeInBytes(), "GPU vis: %s", in.m_passesName.cstr());

	ComputeRenderPassDescription& pass = rgraph.newComputeRenderPass(passName.getBegin());

	pass.newBufferDependency(getRenderer().getGpuSceneBufferHandle(), BufferUsageBit::kUavComputeRead);
	pass.newBufferDependency(zeroStuffDependency, BufferUsageBit::kUavComputeWrite);
	pass.newBufferDependency(out.m_someBufferHandle, BufferUsageBit::kUavComputeWrite);

	if(!distanceBased && static_cast<FrustumGpuVisibilityInput&>(in).m_hzbRt)
	{
		frustumTestData->m_hzbRt = *static_cast<FrustumGpuVisibilityInput&>(in).m_hzbRt;
		pass.newTextureDependency(frustumTestData->m_hzbRt, TextureUsageBit::kSampledCompute);
	}

	pass.setWork([this, frustumTestData, distTestData, lodReferencePoint = in.m_lodReferencePoint, lodDistances = in.m_lodDistances,
				  technique = in.m_technique, aabbCount = counts.m_aabbCount, out](RenderPassWorkContext& rpass) {
		CommandBuffer& cmdb = *rpass.m_commandBuffer;

		const Bool gatherAabbIndices = out.m_visibleAaabbIndicesBuffer.m_buffer != nullptr;
		const Bool genHash = out.m_visiblesHashBuffer.m_buffer != nullptr;

		if(frustumTestData)
		{
			cmdb.bindShaderProgram(m_frustumGrProgs[frustumTestData->m_hzbRt.isValid()][gatherAabbIndices][genHash].get());
		}
		else
		{
			cmdb.bindShaderProgram(m_distGrProgs[gatherAabbIndices][genHash].get());
		}

		BufferOffsetRange aabbsBuffer;
		switch(technique)
		{
		case RenderingTechnique::kGBuffer:
			aabbsBuffer = GpuSceneArrays::RenderableBoundingVolumeGBuffer::getSingleton().getBufferOffsetRange();
			break;
		case RenderingTechnique::kDepth:
			aabbsBuffer = GpuSceneArrays::RenderableBoundingVolumeDepth::getSingleton().getBufferOffsetRange();
			break;
		case RenderingTechnique::kForward:
			aabbsBuffer = GpuSceneArrays::RenderableBoundingVolumeForward::getSingleton().getBufferOffsetRange();
			break;
		default:
			ANKI_ASSERT(0);
		}

		cmdb.bindUavBuffer(0, 0, aabbsBuffer);
		cmdb.bindUavBuffer(0, 1, GpuSceneArrays::Renderable::getSingleton().getBufferOffsetRange());
		cmdb.bindUavBuffer(0, 2, GpuSceneArrays::MeshLod::getSingleton().getBufferOffsetRange());
		cmdb.bindUavBuffer(0, 3, GpuSceneBuffer::getSingleton().getBufferOffsetRange());
		cmdb.bindUavBuffer(0, 4, out.m_legacy.m_renderableInstancesBuffer);
		cmdb.bindUavBuffer(0, 5, out.m_legacy.m_drawIndexedIndirectArgsBuffer);
		cmdb.bindUavBuffer(0, 6, out.m_legacy.m_mdiDrawCountsBuffer);
		cmdb.bindUavBuffer(0, 7, out.m_mesh.m_taskShaderIndirectArgsBuffer);
		cmdb.bindUavBuffer(0, 8, out.m_mesh.m_taskShaderPayloadBuffer);

		U32* drawIndirectArgsIndexOrTaskPayloadIndex =
			allocateAndBindUav<U32>(cmdb, 0, 9, RenderStateBucketContainer::getSingleton().getBucketCount(technique));
		U32 bucketCount = 0;
		U32 legacyGeometryFlowDrawCount = 0;
		U32 taskPayloadCount = 0;
		RenderStateBucketContainer::getSingleton().iterateBuckets(technique, [&](const RenderStateInfo&, U32 userCount, U32 meshletGroupCount) {
			if(userCount == 0)
			{
				// Empty bucket
				drawIndirectArgsIndexOrTaskPayloadIndex[bucketCount] = kMaxU32;
			}
			else if(meshletGroupCount)
			{
				drawIndirectArgsIndexOrTaskPayloadIndex[bucketCount] = taskPayloadCount;
				taskPayloadCount += min(meshletGroupCount, kMaxMeshletGroupCountPerRenderStateBucket);
			}
			else
			{
				drawIndirectArgsIndexOrTaskPayloadIndex[bucketCount] = legacyGeometryFlowDrawCount;
				legacyGeometryFlowDrawCount += userCount;
			}

			++bucketCount;
		});

		if(frustumTestData)
		{
			FrustumGpuVisibilityConstants* unis = allocateAndBindConstants<FrustumGpuVisibilityConstants>(cmdb, 0, 10);

			Array<Plane, 6> planes;
			extractClipPlanes(frustumTestData->m_viewProjMat, planes);
			for(U32 i = 0; i < 6; ++i)
			{
				unis->m_clipPlanes[i] = Vec4(planes[i].getNormal().xyz(), planes[i].getOffset());
			}

			ANKI_ASSERT(kMaxLodCount == 3);
			unis->m_maxLodDistances[0] = lodDistances[0];
			unis->m_maxLodDistances[1] = lodDistances[1];
			unis->m_maxLodDistances[2] = kMaxF32;
			unis->m_maxLodDistances[3] = kMaxF32;

			unis->m_lodReferencePoint = lodReferencePoint;
			unis->m_viewProjectionMat = frustumTestData->m_viewProjMat;
			unis->m_finalRenderTargetSize = Vec2(frustumTestData->m_finalRenderTargetSize);

			if(frustumTestData->m_hzbRt.isValid())
			{
				rpass.bindColorTexture(0, 11, frustumTestData->m_hzbRt);
				cmdb.bindSampler(0, 12, getRenderer().getSamplers().m_nearestNearestClamp.get());
			}
		}
		else
		{
			DistanceGpuVisibilityConstants unis;
			unis.m_pointOfTest = distTestData->m_pointOfTest;
			unis.m_testRadius = distTestData->m_testRadius;

			unis.m_maxLodDistances[0] = lodDistances[0];
			unis.m_maxLodDistances[1] = lodDistances[1];
			unis.m_maxLodDistances[2] = kMaxF32;
			unis.m_maxLodDistances[3] = kMaxF32;

			unis.m_lodReferencePoint = lodReferencePoint;

			cmdb.setPushConstants(&unis, sizeof(unis));
		}

		if(gatherAabbIndices)
		{
			cmdb.bindUavBuffer(0, 13, out.m_visibleAaabbIndicesBuffer);
		}

		if(genHash)
		{
			cmdb.bindUavBuffer(0, 14, out.m_visiblesHashBuffer);
		}

		dispatchPPCompute(cmdb, 64, 1, aabbCount, 1);
	});
}

Error GpuMeshletVisibility::init()
{
	for(MutatorValue hzb = 0; hzb < 2; ++hzb)
	{
		ANKI_CHECK(loadShaderProgram("ShaderBinaries/GpuVisibilityMeshlet.ankiprogbin", {{"HZB_TEST", hzb}}, m_meshletCullingProg,
									 m_meshletCullingGrProgs[hzb]));
	}

	return Error::kNone;
}

void GpuMeshletVisibility::populateRenderGraph(GpuMeshletVisibilityInput& in, GpuMeshletVisibilityOutput& out)
{
	RenderGraphDescription& rgraph = *in.m_rgraph;

	const Counts counts = countTechnique(in.m_technique);

	if(counts.m_allUserCount == 0) [[unlikely]]
	{
		// Early exit
		return;
	}

	// Allocate memory
	const Bool firstFrame = m_runCtx.m_frameIdx != getRenderer().getFrameCount();
	if(firstFrame)
	{
		// Allocate the big buffers once at the beginning of the frame

		m_runCtx.m_frameIdx = getRenderer().getFrameCount();
		m_runCtx.m_populateRenderGraphFrameCallCount = 0;

		// Find the max counts of all techniques
		Counts maxCounts = {};
		for(RenderingTechnique t : EnumBitsIterable<RenderingTechnique, RenderingTechniqueBit>(RenderingTechniqueBit::kAllRaster))
		{
			maxCounts = maxCounts.max((in.m_technique == t) ? counts : countTechnique(t));
		}

		// Allocate memory
		for(PersistentMemory& mem : m_runCtx.m_persistentMem)
		{
			mem = {};

			mem.m_meshletInstancesBuffer =
				GpuVisibleTransientMemoryPool::getSingleton().allocate(max(1u, maxCounts.m_meshletGroupCount) * sizeof(UVec4));
		}
	}

	PersistentMemory& mem = m_runCtx.m_persistentMem[m_runCtx.m_populateRenderGraphFrameCallCount % m_runCtx.m_persistentMem.getSize()];
	++m_runCtx.m_populateRenderGraphFrameCallCount;

	out.m_drawIndirectArgsBuffer = GpuVisibleTransientMemoryPool::getSingleton().allocate(sizeof(DrawIndirectArgs) * counts.m_bucketCount);

	out.m_meshletInstancesBuffer = mem.m_meshletInstancesBuffer;
	out.m_meshletInstancesBuffer.m_range = max(1u, counts.m_meshletGroupCount) * sizeof(UVec4);

	// Zero some stuff
	const BufferHandle zeroStuffDependency = rgraph.importBuffer(BufferUsageBit::kNone, out.m_drawIndirectArgsBuffer);
	{
		Array<Char, 128> passName;
		snprintf(passName.getBegin(), passName.getSizeInBytes(), "GPU meshlet vis zero: %s", in.m_passesName.cstr());

		ComputeRenderPassDescription& pass = rgraph.newComputeRenderPass(passName.getBegin());
		pass.newBufferDependency(zeroStuffDependency, BufferUsageBit::kTransferDestination);

		pass.setWork([drawIndirectArgsBuffer = out.m_drawIndirectArgsBuffer](RenderPassWorkContext& rpass) {
			CommandBuffer& cmdb = *rpass.m_commandBuffer;

			cmdb.pushDebugMarker("Draw indirect args", Vec3(1.0f, 1.0f, 1.0f));
			cmdb.fillBuffer(drawIndirectArgsBuffer, 0);
			cmdb.popDebugMarker();
		});
	}

	// Set the out dependency. Use one of the big buffers.
	if(!mem.m_bufferDepedency.isValid())
	{
		mem.m_bufferDepedency = rgraph.importBuffer(BufferUsageBit::kNone, mem.m_meshletInstancesBuffer);
	}
	out.m_dependency = mem.m_bufferDepedency;

	// Create the renderpass
	Array<Char, 128> passName;
	snprintf(passName.getBegin(), passName.getSizeInBytes(), "GPU meshlet vis: %s", in.m_passesName.cstr());

	ComputeRenderPassDescription& pass = rgraph.newComputeRenderPass(passName.getBegin());

	pass.newBufferDependency(out.m_dependency, BufferUsageBit::kUavComputeWrite);
	pass.newBufferDependency(zeroStuffDependency, BufferUsageBit::kUavComputeRead);

	pass.setWork([this, technique = in.m_technique, hzbRt = in.m_hzbRt, taskShaderPayloadsBuff = in.m_taskShaderPayloadBuffer,
				  viewProjMat = in.m_viewProjectionMatrix, camTrf = in.m_cameraTransform, viewportSize = in.m_viewportSize,
				  out](RenderPassWorkContext& rpass) {
		CommandBuffer& cmdb = *rpass.m_commandBuffer;

		U32 bucketIdx = 0;
		U32 firstPayload = 0;
		RenderStateBucketContainer::getSingleton().iterateBuckets(technique, [&](const RenderStateInfo&, U32 userCount, U32 meshletGroupCount) {
			if(!meshletGroupCount)
			{
				++bucketIdx;
				return;
			}

			// Create a depedency to a part of the indirect args buffer
			const BufferOffsetRange drawIndirectArgsBufferChunk = {out.m_drawIndirectArgsBuffer.m_buffer,
																   out.m_drawIndirectArgsBuffer.m_offset + sizeof(DrawIndirectArgs) * bucketIdx,
																   sizeof(DrawIndirectArgs)};

			const Bool hasHzb = hzbRt.isValid();

			cmdb.bindShaderProgram(m_meshletCullingGrProgs[hasHzb].get());

			cmdb.bindUavBuffer(0, 0, taskShaderPayloadsBuff);
			cmdb.bindUavBuffer(0, 1, GpuSceneArrays::Renderable::getSingleton().getBufferOffsetRange());
			cmdb.bindUavBuffer(0, 2, GpuSceneArrays::MeshLod::getSingleton().getBufferOffsetRange());
			cmdb.bindUavBuffer(0, 3, GpuSceneBuffer::getSingleton().getBufferOffsetRange());
			cmdb.bindUavBuffer(0, 4, UnifiedGeometryBuffer::getSingleton().getBufferOffsetRange());
			cmdb.bindUavBuffer(0, 5, out.m_drawIndirectArgsBuffer);
			cmdb.bindUavBuffer(0, 6, out.m_meshletInstancesBuffer);
			if(hasHzb)
			{
				rpass.bindColorTexture(0, 7, hzbRt);
				cmdb.bindSampler(0, 8, getRenderer().getSamplers().m_nearestNearestClamp.get());
			}

			class MaterialGlobalConstants
			{
			public:
				Mat4 m_viewProjectionMatrix;
				Mat3x4 m_cameraTransform;

				Vec2 m_viewportSizef;
				U32 m_firstPayload;
				U32 m_padding;
			} consts;
			consts.m_viewProjectionMatrix = viewProjMat;
			consts.m_cameraTransform = camTrf;
			consts.m_viewportSizef = Vec2(viewportSize);
			consts.m_firstPayload = firstPayload;
			cmdb.setPushConstants(&consts, sizeof(consts));

			cmdb.dispatchCompute(meshletGroupCount, 1, 1);

			firstPayload += min(meshletGroupCount, kMaxMeshletGroupCountPerRenderStateBucket);
			++bucketIdx;
		});
	});
}

Error GpuVisibilityNonRenderables::init()
{
	ANKI_CHECK(ResourceManager::getSingleton().loadResource("ShaderBinaries/GpuVisibilityNonRenderables.ankiprogbin", m_prog));

	for(MutatorValue hzb = 0; hzb < 2; ++hzb)
	{
		for(GpuSceneNonRenderableObjectType type : EnumIterable<GpuSceneNonRenderableObjectType>())
		{
			for(MutatorValue cpuFeedback = 0; cpuFeedback < 2; ++cpuFeedback)
			{
				ANKI_CHECK(loadShaderProgram("ShaderBinaries/GpuVisibilityNonRenderables.ankiprogbin",
											 {{"HZB_TEST", hzb}, {"OBJECT_TYPE", MutatorValue(type)}, {"CPU_FEEDBACK", cpuFeedback}}, m_prog,
											 m_grProgs[hzb][type][cpuFeedback]));
			}
		}
	}

	return Error::kNone;
}

void GpuVisibilityNonRenderables::populateRenderGraph(GpuVisibilityNonRenderablesInput& in, GpuVisibilityNonRenderablesOutput& out)
{
	ANKI_ASSERT(in.m_viewProjectionMat != Mat4::getZero());
	RenderGraphDescription& rgraph = *in.m_rgraph;

	U32 objCount = 0;
	switch(in.m_objectType)
	{
	case GpuSceneNonRenderableObjectType::kLight:
		objCount = GpuSceneArrays::Light::getSingleton().getElementCount();
		break;
	case GpuSceneNonRenderableObjectType::kDecal:
		objCount = GpuSceneArrays::Decal::getSingleton().getElementCount();
		break;
	case GpuSceneNonRenderableObjectType::kFogDensityVolume:
		objCount = GpuSceneArrays::FogDensityVolume::getSingleton().getElementCount();
		break;
	case GpuSceneNonRenderableObjectType::kGlobalIlluminationProbe:
		objCount = GpuSceneArrays::GlobalIlluminationProbe::getSingleton().getElementCount();
		break;
	case GpuSceneNonRenderableObjectType::kReflectionProbe:
		objCount = GpuSceneArrays::ReflectionProbe::getSingleton().getElementCount();
		break;
	default:
		ANKI_ASSERT(0);
	}

	if(objCount == 0)
	{
		U32* count;
		out.m_visiblesBuffer = RebarTransientMemoryPool::getSingleton().allocateFrame(sizeof(U32), count);
		*count = 0;
		out.m_visiblesBufferHandle = rgraph.importBuffer(BufferUsageBit::kNone, out.m_visiblesBuffer);

		return;
	}

	if(in.m_cpuFeedbackBuffer.m_buffer)
	{
		ANKI_ASSERT(in.m_cpuFeedbackBuffer.m_range == sizeof(U32) * (objCount * 2 + 1));
	}

	const Bool firstRunInFrame = m_lastFrameIdx != getRenderer().getFrameCount();
	if(firstRunInFrame)
	{
		// 1st run in this frame, do some bookkeeping
		m_lastFrameIdx = getRenderer().getFrameCount();
		m_counterBufferOffset = 0;
		m_counterBufferZeroingHandle = {};
	}

	constexpr U32 kCountersPerDispatch = 3; // 1 for the threadgroup, 1 for the visbile object count and 1 for objects with feedback
	const U32 counterBufferElementSize =
		getAlignedRoundUp(GrManager::getSingleton().getDeviceCapabilities().m_uavBufferBindOffsetAlignment, U32(kCountersPerDispatch * sizeof(U32)));
	if(!m_counterBuffer.isCreated() || m_counterBufferOffset + counterBufferElementSize > m_counterBuffer->getSize()) [[unlikely]]
	{
		// Counter buffer not created or not big enough, create a new one

		BufferInitInfo buffInit("GpuVisibilityNonRenderablesCounters");
		buffInit.m_size = (m_counterBuffer.isCreated()) ? m_counterBuffer->getSize() * 2
														: kCountersPerDispatch * counterBufferElementSize * kInitialCounterArraySize;
		buffInit.m_usage = BufferUsageBit::kUavComputeWrite | BufferUsageBit::kUavComputeRead | BufferUsageBit::kTransferDestination;
		m_counterBuffer = GrManager::getSingleton().newBuffer(buffInit);

		m_counterBufferZeroingHandle = rgraph.importBuffer(m_counterBuffer.get(), buffInit.m_usage, 0, kMaxPtrSize);

		ComputeRenderPassDescription& pass = rgraph.newComputeRenderPass("GpuVisibilityNonRenderablesClearCounterBuffer");

		pass.newBufferDependency(m_counterBufferZeroingHandle, BufferUsageBit::kTransferDestination);

		pass.setWork([counterBuffer = m_counterBuffer](RenderPassWorkContext& rgraph) {
			rgraph.m_commandBuffer->fillBuffer(counterBuffer.get(), 0, kMaxPtrSize, 0);
		});

		m_counterBufferOffset = 0;
	}
	else if(!firstRunInFrame)
	{
		m_counterBufferOffset += counterBufferElementSize;
	}

	// Allocate memory for the result
	out.m_visiblesBuffer = GpuVisibleTransientMemoryPool::getSingleton().allocate((objCount + 1) * sizeof(U32));
	out.m_visiblesBufferHandle = rgraph.importBuffer(BufferUsageBit::kNone, out.m_visiblesBuffer);

	// Create the renderpass
	ComputeRenderPassDescription& pass = rgraph.newComputeRenderPass(in.m_passesName);

	pass.newBufferDependency(getRenderer().getGpuSceneBufferHandle(), BufferUsageBit::kUavComputeRead);
	pass.newBufferDependency(out.m_visiblesBufferHandle, BufferUsageBit::kUavComputeWrite);

	if(in.m_hzbRt)
	{
		pass.newTextureDependency(*in.m_hzbRt, TextureUsageBit::kSampledCompute);
	}

	if(m_counterBufferZeroingHandle.isValid()) [[unlikely]]
	{
		pass.newBufferDependency(m_counterBufferZeroingHandle, BufferUsageBit::kUavComputeRead | BufferUsageBit::kUavComputeWrite);
	}

	pass.setWork([this, objType = in.m_objectType, feedbackBuffer = in.m_cpuFeedbackBuffer, viewProjectionMat = in.m_viewProjectionMat,
				  visibleIndicesBuffHandle = out.m_visiblesBufferHandle, counterBuffer = m_counterBuffer, counterBufferOffset = m_counterBufferOffset,
				  objCount](RenderPassWorkContext& rgraph) {
		CommandBuffer& cmdb = *rgraph.m_commandBuffer;

		const Bool needsFeedback = feedbackBuffer.m_buffer != nullptr;

		cmdb.bindShaderProgram(m_grProgs[0][objType][needsFeedback].get());

		BufferOffsetRange objBuffer;
		switch(objType)
		{
		case GpuSceneNonRenderableObjectType::kLight:
			objBuffer = GpuSceneArrays::Light::getSingleton().getBufferOffsetRange();
			break;
		case GpuSceneNonRenderableObjectType::kDecal:
			objBuffer = GpuSceneArrays::Decal::getSingleton().getBufferOffsetRange();
			break;
		case GpuSceneNonRenderableObjectType::kFogDensityVolume:
			objBuffer = GpuSceneArrays::FogDensityVolume::getSingleton().getBufferOffsetRange();
			break;
		case GpuSceneNonRenderableObjectType::kGlobalIlluminationProbe:
			objBuffer = GpuSceneArrays::GlobalIlluminationProbe::getSingleton().getBufferOffsetRange();
			break;
		case GpuSceneNonRenderableObjectType::kReflectionProbe:
			objBuffer = GpuSceneArrays::ReflectionProbe::getSingleton().getBufferOffsetRange();
			break;
		default:
			ANKI_ASSERT(0);
		}
		cmdb.bindUavBuffer(0, 0, objBuffer);

		GpuVisibilityNonRenderableConstants unis;
		Array<Plane, 6> planes;
		extractClipPlanes(viewProjectionMat, planes);
		for(U32 i = 0; i < 6; ++i)
		{
			unis.m_clipPlanes[i] = Vec4(planes[i].getNormal().xyz(), planes[i].getOffset());
		}
		cmdb.setPushConstants(&unis, sizeof(unis));

		rgraph.bindUavBuffer(0, 1, visibleIndicesBuffHandle);
		cmdb.bindUavBuffer(0, 2, counterBuffer.get(), counterBufferOffset, sizeof(U32) * kCountersPerDispatch);

		if(needsFeedback)
		{
			cmdb.bindUavBuffer(0, 3, feedbackBuffer.m_buffer, feedbackBuffer.m_offset, feedbackBuffer.m_range);
		}

		dispatchPPCompute(cmdb, 64, 1, objCount, 1);
	});
}

Error GpuVisibilityAccelerationStructures::init()
{
	ANKI_CHECK(loadShaderProgram("ShaderBinaries/GpuVisibilityAccelerationStructures.ankiprogbin", m_visibilityProg, m_visibilityGrProg));
	ANKI_CHECK(loadShaderProgram("ShaderBinaries/GpuVisibilityAccelerationStructuresZeroRemainingInstances.ankiprogbin", m_zeroRemainingInstancesProg,
								 m_zeroRemainingInstancesGrProg));

	BufferInitInfo inf("GpuVisibilityAccelerationStructuresCounters");
	inf.m_size = sizeof(U32) * 2;
	inf.m_usage = BufferUsageBit::kUavComputeWrite | BufferUsageBit::kUavComputeRead | BufferUsageBit::kTransferDestination;
	m_counterBuffer = GrManager::getSingleton().newBuffer(inf);

	zeroBuffer(m_counterBuffer.get());

	return Error::kNone;
}

void GpuVisibilityAccelerationStructures::pupulateRenderGraph(GpuVisibilityAccelerationStructuresInput& in,
															  GpuVisibilityAccelerationStructuresOutput& out)
{
	in.validate();
	RenderGraphDescription& rgraph = *in.m_rgraph;

#if ANKI_ASSERTIONS_ENABLED
	ANKI_ASSERT(m_lastFrameIdx != getRenderer().getFrameCount());
	m_lastFrameIdx = getRenderer().getFrameCount();
#endif

	// Allocate the transient buffers
	const U32 aabbCount = GpuSceneArrays::RenderableBoundingVolumeRt::getSingleton().getElementCount();

	out.m_instancesBuffer = GpuVisibleTransientMemoryPool::getSingleton().allocate(aabbCount * sizeof(AccelerationStructureInstance));
	out.m_someBufferHandle = rgraph.importBuffer(BufferUsageBit::kUavComputeWrite, out.m_instancesBuffer);

	out.m_renderableIndicesBuffer = GpuVisibleTransientMemoryPool::getSingleton().allocate((aabbCount + 1) * sizeof(U32));

	const BufferOffsetRange zeroInstancesDispatchArgsBuff = GpuVisibleTransientMemoryPool::getSingleton().allocate(sizeof(DispatchIndirectArgs));

	// Create vis pass
	{
		ComputeRenderPassDescription& pass = rgraph.newComputeRenderPass(in.m_passesName);

		pass.newBufferDependency(getRenderer().getGpuSceneBufferHandle(), BufferUsageBit::kUavComputeRead);
		pass.newBufferDependency(out.m_someBufferHandle, BufferUsageBit::kUavComputeWrite);

		pass.setWork([this, viewProjMat = in.m_viewProjectionMatrix, lodDistances = in.m_lodDistances, pointOfTest = in.m_pointOfTest,
					  testRadius = in.m_testRadius, instancesBuff = out.m_instancesBuffer, indicesBuff = out.m_renderableIndicesBuffer,
					  zeroInstancesDispatchArgsBuff](RenderPassWorkContext& rgraph) {
			CommandBuffer& cmdb = *rgraph.m_commandBuffer;

			cmdb.bindShaderProgram(m_visibilityGrProg.get());

			GpuVisibilityAccelerationStructuresConstants unis;
			Array<Plane, 6> planes;
			extractClipPlanes(viewProjMat, planes);
			for(U32 i = 0; i < 6; ++i)
			{
				unis.m_clipPlanes[i] = Vec4(planes[i].getNormal().xyz(), planes[i].getOffset());
			}

			unis.m_pointOfTest = pointOfTest;
			unis.m_testRadius = testRadius;

			ANKI_ASSERT(kMaxLodCount == 3);
			unis.m_maxLodDistances[0] = lodDistances[0];
			unis.m_maxLodDistances[1] = lodDistances[1];
			unis.m_maxLodDistances[2] = kMaxF32;
			unis.m_maxLodDistances[3] = kMaxF32;

			cmdb.setPushConstants(&unis, sizeof(unis));

			cmdb.bindUavBuffer(0, 0, GpuSceneArrays::RenderableBoundingVolumeRt::getSingleton().getBufferOffsetRange());
			cmdb.bindUavBuffer(0, 1, GpuSceneArrays::Renderable::getSingleton().getBufferOffsetRange());
			cmdb.bindUavBuffer(0, 2, GpuSceneArrays::MeshLod::getSingleton().getBufferOffsetRange());
			cmdb.bindUavBuffer(0, 3, &GpuSceneBuffer::getSingleton().getBuffer(), 0, kMaxPtrSize);
			cmdb.bindUavBuffer(0, 4, instancesBuff);
			cmdb.bindUavBuffer(0, 5, indicesBuff);
			cmdb.bindUavBuffer(0, 6, m_counterBuffer.get(), 0, sizeof(U32) * 2);
			cmdb.bindUavBuffer(0, 7, zeroInstancesDispatchArgsBuff);

			const U32 aabbCount = GpuSceneArrays::RenderableBoundingVolumeRt::getSingleton().getElementCount();
			dispatchPPCompute(cmdb, 64, 1, aabbCount, 1);
		});
	}

	// Zero remaining instances
	{
		Array<Char, 64> passName;
		snprintf(passName.getBegin(), sizeof(passName), "%s: Zero remaining instances", in.m_passesName.cstr());

		ComputeRenderPassDescription& pass = rgraph.newComputeRenderPass(passName.getBegin());

		pass.newBufferDependency(out.m_someBufferHandle, BufferUsageBit::kUavComputeWrite);

		pass.setWork([this, zeroInstancesDispatchArgsBuff, instancesBuff = out.m_instancesBuffer,
					  indicesBuff = out.m_renderableIndicesBuffer](RenderPassWorkContext& rgraph) {
			CommandBuffer& cmdb = *rgraph.m_commandBuffer;

			cmdb.bindShaderProgram(m_zeroRemainingInstancesGrProg.get());

			cmdb.bindUavBuffer(0, 0, indicesBuff);
			cmdb.bindUavBuffer(0, 1, instancesBuff);

			cmdb.dispatchComputeIndirect(zeroInstancesDispatchArgsBuff.m_buffer, zeroInstancesDispatchArgsBuff.m_offset);
		});
	}
}

} // end namespace anki
