// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Gr/CommandBuffer.h>
#include <AnKi/Gr/Vulkan/CommandBufferImpl.h>
#include <AnKi/Gr/Vulkan/GrManagerImpl.h>
#include <AnKi/Gr/AccelerationStructure.h>
#include <AnKi/Gr/Vulkan/FenceImpl.h>

namespace anki {

CommandBuffer* CommandBuffer::newInstance(const CommandBufferInitInfo& init)
{
	ANKI_TRACE_SCOPED_EVENT(VkNewCommandBuffer);
	CommandBufferImpl* impl = anki::newInstance<CommandBufferImpl>(GrMemoryPool::getSingleton(), init.getName());
	const Error err = impl->init(init);
	if(err)
	{
		deleteInstance(GrMemoryPool::getSingleton(), impl);
		impl = nullptr;
	}
	return impl;
}

void CommandBuffer::flush(ConstWeakArray<FencePtr> waitFences, FencePtr* signalFence)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.endRecording();

	if(!self.isSecondLevel())
	{
		Array<MicroSemaphorePtr, 8> waitSemaphores;
		for(U32 i = 0; i < waitFences.getSize(); ++i)
		{
			waitSemaphores[i] = static_cast<const FenceImpl&>(*waitFences[i]).m_semaphore;
		}

		MicroSemaphorePtr signalSemaphore;
		getGrManagerImpl().flushCommandBuffer(self.getMicroCommandBuffer(), self.renderedToDefaultFramebuffer(),
											  WeakArray<MicroSemaphorePtr>(waitSemaphores.getBegin(), waitFences.getSize()),
											  (signalFence) ? &signalSemaphore : nullptr);

		if(signalFence)
		{
			FenceImpl* fenceImpl = anki::newInstance<FenceImpl>(GrMemoryPool::getSingleton(), "SignalFence");
			fenceImpl->m_semaphore = signalSemaphore;
			signalFence->reset(fenceImpl);
		}
	}
	else
	{
		ANKI_ASSERT(signalFence == nullptr);
		ANKI_ASSERT(waitFences.getSize() == 0);
	}
}

void CommandBuffer::bindVertexBuffer(U32 binding, Buffer* buff, PtrSize offset, PtrSize stride, VertexStepRate stepRate)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.bindVertexBufferInternal(binding, buff, offset, stride, stepRate);
}

void CommandBuffer::setVertexAttribute(U32 location, U32 buffBinding, Format fmt, PtrSize relativeOffset)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setVertexAttributeInternal(location, buffBinding, fmt, relativeOffset);
}

void CommandBuffer::bindIndexBuffer(Buffer* buff, PtrSize offset, IndexType type)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.bindIndexBufferInternal(buff, offset, type);
}

void CommandBuffer::setPrimitiveRestart(Bool enable)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setPrimitiveRestartInternal(enable);
}

void CommandBuffer::setViewport(U32 minx, U32 miny, U32 width, U32 height)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setViewportInternal(minx, miny, width, height);
}

void CommandBuffer::setScissor(U32 minx, U32 miny, U32 width, U32 height)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setScissorInternal(minx, miny, width, height);
}

void CommandBuffer::setFillMode(FillMode mode)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setFillModeInternal(mode);
}

void CommandBuffer::setCullMode(FaceSelectionBit mode)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setCullModeInternal(mode);
}

void CommandBuffer::setPolygonOffset(F32 factor, F32 units)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setPolygonOffsetInternal(factor, units);
}

void CommandBuffer::setStencilOperations(FaceSelectionBit face, StencilOperation stencilFail, StencilOperation stencilPassDepthFail,
										 StencilOperation stencilPassDepthPass)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setStencilOperationsInternal(face, stencilFail, stencilPassDepthFail, stencilPassDepthPass);
}

void CommandBuffer::setStencilCompareOperation(FaceSelectionBit face, CompareOperation comp)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setStencilCompareOperationInternal(face, comp);
}

void CommandBuffer::setStencilCompareMask(FaceSelectionBit face, U32 mask)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setStencilCompareMaskInternal(face, mask);
}

void CommandBuffer::setStencilWriteMask(FaceSelectionBit face, U32 mask)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setStencilWriteMaskInternal(face, mask);
}

void CommandBuffer::setStencilReference(FaceSelectionBit face, U32 ref)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setStencilReferenceInternal(face, ref);
}

void CommandBuffer::setDepthWrite(Bool enable)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setDepthWriteInternal(enable);
}

void CommandBuffer::setDepthCompareOperation(CompareOperation op)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setDepthCompareOperationInternal(op);
}

void CommandBuffer::setAlphaToCoverage(Bool enable)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setAlphaToCoverageInternal(enable);
}

void CommandBuffer::setColorChannelWriteMask(U32 attachment, ColorBit mask)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setColorChannelWriteMaskInternal(attachment, mask);
}

void CommandBuffer::setBlendFactors(U32 attachment, BlendFactor srcRgb, BlendFactor dstRgb, BlendFactor srcA, BlendFactor dstA)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setBlendFactorsInternal(attachment, srcRgb, dstRgb, srcA, dstA);
}

void CommandBuffer::setBlendOperation(U32 attachment, BlendOperation funcRgb, BlendOperation funcA)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setBlendOperationInternal(attachment, funcRgb, funcA);
}

void CommandBuffer::bindTextureAndSampler(U32 set, U32 binding, TextureView* texView, Sampler* sampler, U32 arrayIdx)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.bindTextureAndSamplerInternal(set, binding, texView, sampler, arrayIdx);
}

void CommandBuffer::bindTexture(U32 set, U32 binding, TextureView* texView, U32 arrayIdx)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.bindTextureInternal(set, binding, texView, arrayIdx);
}

void CommandBuffer::bindSampler(U32 set, U32 binding, Sampler* sampler, U32 arrayIdx)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.bindSamplerInternal(set, binding, sampler, arrayIdx);
}

void CommandBuffer::bindConstantBuffer(U32 set, U32 binding, Buffer* buff, PtrSize offset, PtrSize range, U32 arrayIdx)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.bindConstantBufferInternal(set, binding, buff, offset, range, arrayIdx);
}

void CommandBuffer::bindUavBuffer(U32 set, U32 binding, Buffer* buff, PtrSize offset, PtrSize range, U32 arrayIdx)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.bindUavBufferInternal(set, binding, buff, offset, range, arrayIdx);
}

void CommandBuffer::bindUavTexture(U32 set, U32 binding, TextureView* img, U32 arrayIdx)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.bindUavTextureInternal(set, binding, img, arrayIdx);
}

void CommandBuffer::bindAccelerationStructure(U32 set, U32 binding, AccelerationStructure* as, U32 arrayIdx)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.bindAccelerationStructureInternal(set, binding, as, arrayIdx);
}

void CommandBuffer::bindReadOnlyTextureBuffer(U32 set, U32 binding, Buffer* buff, PtrSize offset, PtrSize range, Format fmt, U32 arrayIdx)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.bindReadOnlyTextureBufferInternal(set, binding, buff, offset, range, fmt, arrayIdx);
}

void CommandBuffer::bindAllBindless(U32 set)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.bindAllBindlessInternal(set);
}

void CommandBuffer::bindShaderProgram(ShaderProgram* prog)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.bindShaderProgramInternal(prog);
}

void CommandBuffer::beginRenderPass(Framebuffer* fb, const Array<TextureUsageBit, kMaxColorRenderTargets>& colorAttachmentUsages,
									TextureUsageBit depthStencilAttachmentUsage, U32 minx, U32 miny, U32 width, U32 height)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.beginRenderPassInternal(fb, colorAttachmentUsages, depthStencilAttachmentUsage, minx, miny, width, height);
}

void CommandBuffer::endRenderPass()
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.endRenderPassInternal();
}

void CommandBuffer::setVrsRate(VrsRate rate)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setVrsRateInternal(rate);
}

void CommandBuffer::drawIndexed(PrimitiveTopology topology, U32 count, U32 instanceCount, U32 firstIndex, U32 baseVertex, U32 baseInstance)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.drawIndexedInternal(topology, count, instanceCount, firstIndex, baseVertex, baseInstance);
}

void CommandBuffer::draw(PrimitiveTopology topology, U32 count, U32 instanceCount, U32 first, U32 baseInstance)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.drawInternal(topology, count, instanceCount, first, baseInstance);
}

void CommandBuffer::drawIndirect(PrimitiveTopology topology, U32 drawCount, PtrSize offset, Buffer* buff)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.drawIndirectInternal(topology, drawCount, offset, buff);
}

void CommandBuffer::drawIndexedIndirectCount(PrimitiveTopology topology, Buffer* argBuffer, PtrSize argBufferOffset, U32 argBufferStride,
											 Buffer* countBuffer, PtrSize countBufferOffset, U32 maxDrawCount)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.drawIndexedIndirectCountInternal(topology, argBuffer, argBufferOffset, argBufferStride, countBuffer, countBufferOffset, maxDrawCount);
}

void CommandBuffer::drawIndirectCount(PrimitiveTopology topology, Buffer* argBuffer, PtrSize argBufferOffset, U32 argBufferStride,
									  Buffer* countBuffer, PtrSize countBufferOffset, U32 maxDrawCount)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.drawIndirectCountInternal(topology, argBuffer, argBufferOffset, argBufferStride, countBuffer, countBufferOffset, maxDrawCount);
}

void CommandBuffer::drawIndexedIndirect(PrimitiveTopology topology, U32 drawCount, PtrSize offset, Buffer* buff)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.drawIndexedIndirectInternal(topology, drawCount, offset, buff);
}

void CommandBuffer::drawMeshTasks(U32 groupCountX, U32 groupCountY, U32 groupCountZ)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.drawMeshTasksInternal(groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::drawMeshTasksIndirect(Buffer* argBuffer, PtrSize argBufferOffset)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.drawMeshTasksIndirectInternal(argBuffer, argBufferOffset);
}

void CommandBuffer::dispatchCompute(U32 groupCountX, U32 groupCountY, U32 groupCountZ)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.dispatchComputeInternal(groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::dispatchComputeIndirect(Buffer* argBuffer, PtrSize argBufferOffset)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.dispatchComputeIndirectInternal(argBuffer, argBufferOffset);
}

void CommandBuffer::traceRays(Buffer* sbtBuffer, PtrSize sbtBufferOffset, U32 sbtRecordSize, U32 hitGroupSbtRecordCount, U32 rayTypeCount, U32 width,
							  U32 height, U32 depth)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.traceRaysInternal(sbtBuffer, sbtBufferOffset, sbtRecordSize, hitGroupSbtRecordCount, rayTypeCount, width, height, depth);
}

void CommandBuffer::generateMipmaps2d(TextureView* texView)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.generateMipmaps2dInternal(texView);
}

void CommandBuffer::generateMipmaps3d([[maybe_unused]] TextureView* texView)
{
	ANKI_ASSERT(!"TODO");
}

void CommandBuffer::blitTextureViews([[maybe_unused]] TextureView* srcView, [[maybe_unused]] TextureView* destView)
{
	ANKI_ASSERT(!"TODO");
}

void CommandBuffer::clearTextureView(TextureView* texView, const ClearValue& clearValue)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.clearTextureViewInternal(texView, clearValue);
}

void CommandBuffer::copyBufferToTextureView(Buffer* buff, PtrSize offset, PtrSize range, TextureView* texView)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.copyBufferToTextureViewInternal(buff, offset, range, texView);
}

void CommandBuffer::fillBuffer(Buffer* buff, PtrSize offset, PtrSize size, U32 value)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.fillBufferInternal(buff, offset, size, value);
}

void CommandBuffer::writeOcclusionQueriesResultToBuffer(ConstWeakArray<OcclusionQuery*> queries, PtrSize offset, Buffer* buff)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.writeOcclusionQueriesResultToBufferInternal(queries, offset, buff);
}

void CommandBuffer::copyBufferToBuffer(Buffer* src, Buffer* dst, ConstWeakArray<CopyBufferToBufferInfo> copies)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.copyBufferToBufferInternal(src, dst, copies);
}

void CommandBuffer::buildAccelerationStructure(AccelerationStructure* as, Buffer* scratchBuffer, PtrSize scratchBufferOffset)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.buildAccelerationStructureInternal(as, scratchBuffer, scratchBufferOffset);
}

void CommandBuffer::upscale(GrUpscaler* upscaler, TextureView* inColor, TextureView* outUpscaledColor, TextureView* motionVectors, TextureView* depth,
							TextureView* exposure, Bool resetAccumulation, const Vec2& jitterOffset, const Vec2& motionVectorsScale)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.upscaleInternal(upscaler, inColor, outUpscaledColor, motionVectors, depth, exposure, resetAccumulation, jitterOffset, motionVectorsScale);
}

void CommandBuffer::setPipelineBarrier(ConstWeakArray<TextureBarrierInfo> textures, ConstWeakArray<BufferBarrierInfo> buffers,
									   ConstWeakArray<AccelerationStructureBarrierInfo> accelerationStructures)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setPipelineBarrierInternal(textures, buffers, accelerationStructures);
}

void CommandBuffer::resetOcclusionQueries(ConstWeakArray<OcclusionQuery*> queries)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.resetOcclusionQueriesInternal(queries);
}

void CommandBuffer::beginOcclusionQuery(OcclusionQuery* query)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.beginOcclusionQueryInternal(query);
}

void CommandBuffer::endOcclusionQuery(OcclusionQuery* query)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.endOcclusionQueryInternal(query);
}

void CommandBuffer::beginPipelineQuery(PipelineQuery* query)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.beginPipelineQueryInternal(query);
}

void CommandBuffer::endPipelineQuery(PipelineQuery* query)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.endPipelineQueryInternal(query);
}

void CommandBuffer::pushSecondLevelCommandBuffers(ConstWeakArray<CommandBuffer*> cmdbs)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.pushSecondLevelCommandBuffersInternal(cmdbs);
}

void CommandBuffer::resetTimestampQueries(ConstWeakArray<TimestampQuery*> queries)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.resetTimestampQueriesInternal(queries);
}

void CommandBuffer::writeTimestamp(TimestampQuery* query)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.writeTimestampInternal(query);
}

Bool CommandBuffer::isEmpty() const
{
	ANKI_VK_SELF_CONST(CommandBufferImpl);
	return self.isEmpty();
}

void CommandBuffer::setPushConstants(const void* data, U32 dataSize)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setPushConstantsInternal(data, dataSize);
}

void CommandBuffer::setRasterizationOrder(RasterizationOrder order)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setRasterizationOrderInternal(order);
}

void CommandBuffer::setLineWidth(F32 width)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.setLineWidthInternal(width);
}

void CommandBuffer::pushDebugMarker(CString name, Vec3 color)
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.pushDebugMarkerInternal(name, color);
}

void CommandBuffer::popDebugMarker()
{
	ANKI_VK_SELF(CommandBufferImpl);
	self.popDebugMarkerInternal();
}

} // end namespace anki
