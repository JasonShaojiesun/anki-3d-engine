// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Renderer/RendererObject.h>

namespace anki {

/// @addtogroup renderer
/// @{

/// Bins clusterer objects to the clusterer.
class ClusterBinning2 : public RendererObject
{
public:
	ClusterBinning2();

	~ClusterBinning2();

	Error init();

	/// Populate the rendergraph.
	void populateRenderGraph(RenderingContext& ctx);

	BufferOffsetRange getClusteredShadingUniforms() const
	{
		ANKI_ASSERT(m_runCtx.m_clusterUniformsOffset != kMaxPtrSize);
		return BufferOffsetRange{&RebarTransientMemoryPool::getSingleton().getBuffer(), m_runCtx.m_clusterUniformsOffset,
								 sizeof(ClusteredShadingUniforms)};
	}

	const BufferOffsetRange& getPackedObjectsBuffer(GpuSceneNonRenderableObjectType type) const
	{
		return m_runCtx.m_packedObjectsBuffers[type];
	}

	BufferHandle getPackedObjectsBufferHandle(GpuSceneNonRenderableObjectType type) const
	{
		return m_runCtx.m_packedObjectsHandles[type];
	}

	const BufferOffsetRange& getClustersBuffer() const
	{
		return m_runCtx.m_clustersBuffer;
	}

	BufferHandle getClustersBufferHandle() const
	{
		return m_runCtx.m_clustersHandle;
	}

private:
	ShaderProgramResourcePtr m_jobSetupProg;
	ShaderProgramPtr m_jobSetupGrProg;

	ShaderProgramResourcePtr m_binningProg;
	Array<ShaderProgramPtr, U32(GpuSceneNonRenderableObjectType::kCount)> m_binningGrProgs;

	ShaderProgramResourcePtr m_packingProg;
	Array<ShaderProgramPtr, U32(GpuSceneNonRenderableObjectType::kCount)> m_packingGrProgs;

	class
	{
	public:
		BufferHandle m_clustersHandle;
		BufferOffsetRange m_clustersBuffer;

		Array<BufferHandle, U32(GpuSceneNonRenderableObjectType::kCount)> m_packedObjectsHandles;
		Array<BufferOffsetRange, U32(GpuSceneNonRenderableObjectType::kCount)> m_packedObjectsBuffers;

		PtrSize m_clusterUniformsOffset = kMaxPtrSize; ///< Offset into the ReBAR buffer.
		ClusteredShadingUniforms* m_uniformsCpu = nullptr;
		RenderingContext* m_rctx = nullptr;
	} m_runCtx;

	void writeClusterUniformsInternal();
};
/// @}

} // end namespace anki
