// Copyright (C) 2009-present, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Gr/D3D/D3DCommandBuffer.h>
#include <AnKi/Gr/D3D/D3DTexture.h>
#include <AnKi/Gr/D3D/D3DSampler.h>
#include <AnKi/Gr/D3D/D3DShaderProgram.h>
#include <AnKi/Gr/D3D/D3DTimestampQuery.h>
#include <AnKi/Gr/D3D/D3DPipelineQuery.h>
#include <AnKi/Gr/D3D/D3DGrManager.h>
#include <AnKi/Util/Tracer.h>

namespace anki {

CommandBuffer* CommandBuffer::newInstance(const CommandBufferInitInfo& init)
{
	ANKI_TRACE_SCOPED_EVENT(D3DNewCommandBuffer);
	CommandBufferImpl* impl = anki::newInstance<CommandBufferImpl>(GrMemoryPool::getSingleton(), init.getName());
	const Error err = impl->init(init);
	if(err)
	{
		deleteInstance(GrMemoryPool::getSingleton(), impl);
		impl = nullptr;
	}
	return impl;
}

void CommandBuffer::endRecording()
{
	ANKI_D3D_SELF(CommandBufferImpl);

	// Write the queries to their result buffers
	for(QueryHandle handle : self.m_timestampQueries)
	{
		const QueryInfo qinfo = TimestampQueryFactory::getSingleton().getQueryInfo(handle);

		self.m_cmdList->ResolveQueryData(qinfo.m_queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, qinfo.m_indexInHeap, 1, qinfo.m_resultsBuffer,
										 qinfo.m_resultsBufferOffset / sizeof(U64));
	}

	for(QueryHandle handle : self.m_pipelineQueries)
	{
		const QueryInfo qinfo = PrimitivesPassedClippingFactory::getSingleton().getQueryInfo(handle);

		self.m_cmdList->ResolveQueryData(qinfo.m_queryHeap, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, qinfo.m_indexInHeap, 1, qinfo.m_resultsBuffer,
										 qinfo.m_resultsBufferOffset / sizeof(U64));
	}

	self.m_cmdList->Close();
}

void CommandBuffer::bindVertexBuffer(U32 binding, const BufferView& buff, U32 stride, VertexStepRate stepRate)
{
	ANKI_ASSERT(stride > 0);
	ANKI_D3D_SELF(CommandBufferImpl);

	self.commandCommon();

	self.m_graphicsState.bindVertexBuffer(binding, stepRate);

	const BufferImpl& impl = static_cast<const BufferImpl&>(buff.getBuffer());

	const D3D12_VERTEX_BUFFER_VIEW d3dView = {.BufferLocation = impl.getGpuAddress() + buff.getOffset(),
											  .SizeInBytes = U32(buff.getRange()),
											  .StrideInBytes = stride};

	self.m_cmdList->IASetVertexBuffers(binding, 1, &d3dView);
}

void CommandBuffer::setVertexAttribute(VertexAttributeSemantic attribute, U32 buffBinding, Format fmt, U32 relativeOffset)
{
	ANKI_ASSERT(attribute < VertexAttributeSemantic::kCount && fmt != Format::kNone);
	ANKI_D3D_SELF(CommandBufferImpl);

	self.commandCommon();
	self.m_graphicsState.setVertexAttribute(attribute, buffBinding, fmt, relativeOffset);
}

void CommandBuffer::bindIndexBuffer(const BufferView& buff, IndexType type)
{
	ANKI_ASSERT(type != IndexType::kCount);
	ANKI_D3D_SELF(CommandBufferImpl);

	const BufferImpl& impl = static_cast<const BufferImpl&>(buff.getBuffer());

	const D3D12_INDEX_BUFFER_VIEW view = {.BufferLocation = impl.getGpuAddress() + buff.getOffset(),
										  .SizeInBytes = U32(buff.getRange()),
										  .Format = (type == IndexType::kU16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT};

	self.m_cmdList->IASetIndexBuffer(&view);
}

void CommandBuffer::setPrimitiveRestart([[maybe_unused]] Bool enable)
{
	ANKI_ASSERT(!"TODO");
}

void CommandBuffer::setViewport(U32 minx, U32 miny, U32 width, U32 height)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setViewport(minx, miny, width, height);
}

void CommandBuffer::setScissor(U32 minx, U32 miny, U32 width, U32 height)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setScissor(minx, miny, width, height);
}

void CommandBuffer::setFillMode(FillMode mode)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setFillMode(mode);
}

void CommandBuffer::setCullMode(FaceSelectionBit mode)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setCullMode(mode);
}

void CommandBuffer::setPolygonOffset(F32 factor, F32 units)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setPolygonOffset(factor, units);
}

void CommandBuffer::setStencilOperations(FaceSelectionBit face, StencilOperation stencilFail, StencilOperation stencilPassDepthFail,
										 StencilOperation stencilPassDepthPass)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setStencilOperations(face, stencilFail, stencilPassDepthFail, stencilPassDepthPass);
}

void CommandBuffer::setStencilCompareOperation(FaceSelectionBit face, CompareOperation comp)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setStencilCompareOperation(face, comp);
}

void CommandBuffer::setStencilCompareMask(FaceSelectionBit face, U32 mask)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setStencilCompareMask(face, mask);
}

void CommandBuffer::setStencilWriteMask(FaceSelectionBit face, U32 mask)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setStencilWriteMask(face, mask);
}

void CommandBuffer::setStencilReference(FaceSelectionBit face, U32 ref)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setStencilReference(face, ref);
}

void CommandBuffer::setDepthWrite(Bool enable)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setDepthWrite(enable);
}

void CommandBuffer::setDepthCompareOperation(CompareOperation op)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setDepthCompareOperation(op);
}

void CommandBuffer::setAlphaToCoverage(Bool enable)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setAlphaToCoverage(enable);
}

void CommandBuffer::setColorChannelWriteMask(U32 attachment, ColorBit mask)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setColorChannelWriteMask(attachment, mask);
}

void CommandBuffer::setBlendFactors(U32 attachment, BlendFactor srcRgb, BlendFactor dstRgb, BlendFactor srcA, BlendFactor dstA)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setBlendFactors(attachment, srcRgb, dstRgb, srcA, dstA);
}

void CommandBuffer::setBlendOperation(U32 attachment, BlendOperation funcRgb, BlendOperation funcA)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();
	self.m_graphicsState.setBlendOperation(attachment, funcRgb, funcA);
}

void CommandBuffer::bindTexture(Register reg, const TextureView& texView)
{
	reg.validate();
	ANKI_D3D_SELF(CommandBufferImpl);

	const TextureImpl& impl = static_cast<const TextureImpl&>(texView.getTexture());

	if(reg.m_resourceType == HlslResourceType::kSrv)
	{
		const DescriptorHeapHandle handle = impl.getOrCreateSrv(texView.getSubresource());
		self.m_descriptors.bindSrv(reg.m_space, reg.m_bindPoint, handle);
	}
	else
	{
		const DescriptorHeapHandle handle = impl.getOrCreateUav(texView.getSubresource());
		self.m_descriptors.bindUav(reg.m_space, reg.m_bindPoint, handle);
	}
}

void CommandBuffer::bindSampler(Register reg, Sampler* sampler)
{
	reg.validate();
	ANKI_ASSERT(sampler);
	ANKI_D3D_SELF(CommandBufferImpl);

	const SamplerImpl& impl = static_cast<const SamplerImpl&>(*sampler);
	self.m_descriptors.bindSampler(reg.m_space, reg.m_bindPoint, impl.m_handle);

	self.m_mcmdb->pushObjectRef(sampler);
}

void CommandBuffer::bindUniformBuffer(Register reg, const BufferView& buff)
{
	reg.validate();
	ANKI_D3D_SELF(CommandBufferImpl);

	const BufferImpl& impl = static_cast<const BufferImpl&>(buff.getBuffer());
	self.m_descriptors.bindCbv(reg.m_space, reg.m_bindPoint, &impl.getD3DResource(), buff.getOffset(), buff.getRange());
}

void CommandBuffer::bindStorageBuffer(Register reg, const BufferView& buff)
{
	reg.validate();
	ANKI_D3D_SELF(CommandBufferImpl);

	const BufferImpl& impl = static_cast<const BufferImpl&>(buff.getBuffer());
	if(reg.m_resourceType == HlslResourceType::kUav)
	{
		self.m_descriptors.bindUav(reg.m_space, reg.m_bindPoint, &impl.getD3DResource(), buff.getOffset(), buff.getRange());
	}
	else
	{
		ANKI_ASSERT(reg.m_resourceType == HlslResourceType::kSrv);
		self.m_descriptors.bindSrv(reg.m_space, reg.m_bindPoint, &impl.getD3DResource(), buff.getOffset(), buff.getRange());
	}
}

void CommandBuffer::bindTexelBuffer(Register reg, const BufferView& buff, Format fmt)
{
	reg.validate();
	ANKI_D3D_SELF(CommandBufferImpl);

	const BufferImpl& impl = static_cast<const BufferImpl&>(buff.getBuffer());
	if(reg.m_resourceType == HlslResourceType::kUav)
	{
		self.m_descriptors.bindUav(reg.m_space, reg.m_bindPoint, &impl.getD3DResource(), buff.getOffset(), buff.getRange(), DXGI_FORMAT(fmt));
	}
	else
	{
		ANKI_ASSERT(reg.m_resourceType == HlslResourceType::kSrv);
		self.m_descriptors.bindSrv(reg.m_space, reg.m_bindPoint, &impl.getD3DResource(), buff.getOffset(), buff.getRange(), DXGI_FORMAT(fmt));
	}
}

void CommandBuffer::bindAccelerationStructure([[maybe_unused]] Register reg, [[maybe_unused]] AccelerationStructure* as)
{
	ANKI_ASSERT(!"TODO");
}

void CommandBuffer::bindShaderProgram(ShaderProgram* prog)
{
	ANKI_ASSERT(prog);
	ANKI_D3D_SELF(CommandBufferImpl);

	self.commandCommon();

	const ShaderProgramImpl& progImpl = static_cast<const ShaderProgramImpl&>(*prog);
	const Bool isCompute = !(progImpl.getShaderTypes() & ShaderTypeBit::kAllGraphics);

	self.m_descriptors.bindRootSignature(progImpl.m_rootSignature, isCompute);

	self.m_mcmdb->pushObjectRef(prog);

	if(isCompute)
	{
		self.m_cmdList->SetPipelineState(progImpl.m_compute.m_pipelineState);
	}
	else
	{
		self.m_graphicsState.bindShaderProgram(prog);
	}

	// Shader program means descriptors so bind the descriptor heaps
	if(!self.m_descriptorHeapsBound)
	{
		self.m_descriptorHeapsBound = true;

		const Array<ID3D12DescriptorHeap*, 2> heaps = DescriptorFactory::getSingleton().getCpuGpuVisibleHeaps();
		self.m_cmdList->SetDescriptorHeaps(2, heaps.getBegin());
	}
}

void CommandBuffer::beginRenderPass(ConstWeakArray<RenderTarget> colorRts, RenderTarget* depthStencilRt, U32 minx, U32 miny, U32 width, U32 height,
									[[maybe_unused]] const TextureView& vrsRt, [[maybe_unused]] U8 vrsRtTexelSizeX,
									[[maybe_unused]] U8 vrsRtTexelSizeY)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();

	U32 rtWidth = 0;
	U32 rtHeight = 0;

	Array<D3D12_RENDER_PASS_RENDER_TARGET_DESC, kMaxColorRenderTargets> colorRtDescs;
	Array<Format, kMaxColorRenderTargets> colorRtFormats;
	for(U32 i = 0; i < colorRts.getSize(); ++i)
	{
		const RenderTarget& rt = colorRts[i];
		D3D12_RENDER_PASS_RENDER_TARGET_DESC& desc = colorRtDescs[i];
		const TextureImpl& tex = static_cast<const TextureImpl&>(rt.m_textureView.getTexture());

		desc = {};
		desc.cpuDescriptor = tex.getOrCreateRtv(rt.m_textureView.getSubresource()).getCpuOffset();
		desc.BeginningAccess.Type = convertLoadOp(rt.m_loadOperation);
		memcpy(&desc.BeginningAccess.Clear.ClearValue.Color, &rt.m_clearValue.m_colorf[0], sizeof(F32) * 4);
		desc.EndingAccess.Type = convertStoreOp(rt.m_storeOperation);

		colorRtFormats[i] = tex.getFormat();

		rtWidth = tex.getWidth() >> rt.m_textureView.getFirstMipmap();
		rtHeight = tex.getHeight() >> rt.m_textureView.getFirstMipmap();
	}

	D3D12_RENDER_PASS_DEPTH_STENCIL_DESC dsDesc;
	Format dsFormat = Format::kNone;
	if(depthStencilRt)
	{
		const TextureImpl& tex = static_cast<const TextureImpl&>(depthStencilRt->m_textureView.getTexture());

		dsDesc = {};
		dsDesc.cpuDescriptor = tex.getOrCreateDsv(depthStencilRt->m_textureView.getSubresource(), depthStencilRt->m_usage).getCpuOffset();

		dsDesc.DepthBeginningAccess.Type = convertLoadOp(depthStencilRt->m_loadOperation);
		dsDesc.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth, depthStencilRt->m_clearValue.m_depthStencil.m_depth;
		dsDesc.DepthEndingAccess.Type = convertStoreOp(depthStencilRt->m_storeOperation);

		dsDesc.StencilBeginningAccess.Type = convertLoadOp(depthStencilRt->m_stencilLoadOperation);
		dsDesc.StencilBeginningAccess.Clear.ClearValue.DepthStencil.Stencil = U8(depthStencilRt->m_clearValue.m_depthStencil.m_stencil);
		dsDesc.StencilEndingAccess.Type = convertStoreOp(depthStencilRt->m_stencilStoreOperation);

		dsFormat = tex.getFormat();

		rtWidth = tex.getWidth() >> depthStencilRt->m_textureView.getFirstMipmap();
		rtHeight = tex.getHeight() >> depthStencilRt->m_textureView.getFirstMipmap();
	}

	self.m_graphicsState.beginRenderPass(ConstWeakArray(colorRtFormats.getBegin(), colorRts.getSize()), dsFormat, UVec2(rtWidth, rtHeight));

	self.m_cmdList->BeginRenderPass(colorRts.getSize(), colorRtDescs.getBegin(), (depthStencilRt) ? &dsDesc : nullptr,
									D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);
}

void CommandBuffer::endRenderPass()
{
	ANKI_D3D_SELF(CommandBufferImpl);

	self.m_cmdList->EndRenderPass();
}

void CommandBuffer::setVrsRate([[maybe_unused]] VrsRate rate)
{
	ANKI_ASSERT(!"TODO");
}

void CommandBuffer::drawIndexed(PrimitiveTopology topology, U32 count, U32 instanceCount, U32 firstIndex, U32 baseVertex, U32 baseInstance)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.m_graphicsState.setPrimitiveTopology(topology);
	self.drawcallCommon();

	self.m_cmdList->DrawIndexedInstanced(count, instanceCount, firstIndex, baseVertex, baseInstance);
}

void CommandBuffer::draw(PrimitiveTopology topology, U32 count, U32 instanceCount, U32 first, U32 baseInstance)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.m_graphicsState.setPrimitiveTopology(topology);
	self.drawcallCommon();

	self.m_cmdList->DrawInstanced(count, instanceCount, first, baseInstance);
}

void CommandBuffer::drawIndirect(PrimitiveTopology topology, const BufferView& buff, U32 drawCount)
{
	ANKI_ASSERT(drawCount > 0);

	ANKI_D3D_SELF(CommandBufferImpl);
	self.m_graphicsState.setPrimitiveTopology(topology);
	self.drawcallCommon();

	ID3D12CommandSignature* signature;
	ANKI_CHECKF(
		IndirectCommandSignatureFactory::getSingleton().getOrCreateSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW, sizeof(DrawIndirectArgs), signature));

	const BufferImpl& buffImpl = static_cast<const BufferImpl&>(buff.getBuffer());
	ANKI_ASSERT(!!(buffImpl.getBufferUsage() & BufferUsageBit::kIndirectDraw));
	ANKI_ASSERT((buff.getOffset() % 4) == 0);
	ANKI_ASSERT((buff.getRange() % sizeof(DrawIndirectArgs)) == 0);
	ANKI_ASSERT(sizeof(DrawIndirectArgs) * drawCount == buff.getRange());

	self.m_cmdList->ExecuteIndirect(signature, drawCount, &buffImpl.getD3DResource(), buff.getOffset(), nullptr, 0);
}

void CommandBuffer::drawIndirectCount(PrimitiveTopology topology, const BufferView& argBuffer, U32 argBufferStride, const BufferView& countBuffer,
									  U32 maxDrawCount)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.m_graphicsState.setPrimitiveTopology(topology);
	self.drawcallCommon();

	ID3D12CommandSignature* signature;
	ANKI_CHECKF(IndirectCommandSignatureFactory::getSingleton().getOrCreateSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW, argBufferStride, signature));

	const BufferImpl& argBuffImpl = static_cast<const BufferImpl&>(argBuffer.getBuffer());
	ANKI_ASSERT(!!(argBuffImpl.getBufferUsage() & BufferUsageBit::kIndirectDraw));
	ANKI_ASSERT((argBuffer.getOffset() % 4) == 0);
	ANKI_ASSERT((argBuffer.getRange() % argBufferStride) == 0);
	ANKI_ASSERT(argBufferStride * maxDrawCount == argBuffer.getRange());

	const BufferImpl& countBuffImpl = static_cast<const BufferImpl&>(countBuffer.getBuffer());
	ANKI_ASSERT(countBuffer.getRange() == sizeof(U32));
	ANKI_ASSERT(!!(countBuffImpl.getBufferUsage() & BufferUsageBit::kIndirectDraw));

	self.m_cmdList->ExecuteIndirect(signature, maxDrawCount, &argBuffImpl.getD3DResource(), argBuffer.getOffset(), &countBuffImpl.getD3DResource(),
									countBuffer.getOffset());
}

void CommandBuffer::drawIndexedIndirect(PrimitiveTopology topology, const BufferView& buff, U32 drawCount)
{
	ANKI_ASSERT(drawCount > 0);

	ANKI_D3D_SELF(CommandBufferImpl);
	self.m_graphicsState.setPrimitiveTopology(topology);
	self.drawcallCommon();

	ID3D12CommandSignature* signature;
	ANKI_CHECKF(IndirectCommandSignatureFactory::getSingleton().getOrCreateSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED,
																					 sizeof(DrawIndexedIndirectArgs), signature));

	const BufferImpl& buffImpl = static_cast<const BufferImpl&>(buff.getBuffer());
	ANKI_ASSERT(!!(buffImpl.getBufferUsage() & BufferUsageBit::kIndirectDraw));
	ANKI_ASSERT((buff.getOffset() % 4) == 0);
	ANKI_ASSERT((buff.getRange() % sizeof(DrawIndexedIndirectArgs)) == 0);
	ANKI_ASSERT(sizeof(DrawIndexedIndirectArgs) * drawCount == buff.getRange());

	self.m_cmdList->ExecuteIndirect(signature, drawCount, &buffImpl.getD3DResource(), buff.getOffset(), nullptr, 0);
}

void CommandBuffer::drawIndexedIndirectCount(PrimitiveTopology topology, const BufferView& argBuffer, U32 argBufferStride,
											 const BufferView& countBuffer, U32 maxDrawCount)
{
	ANKI_ASSERT(maxDrawCount > 0);

	ANKI_D3D_SELF(CommandBufferImpl);
	self.m_graphicsState.setPrimitiveTopology(topology);
	self.drawcallCommon();

	ID3D12CommandSignature* signature;
	ANKI_CHECKF(
		IndirectCommandSignatureFactory::getSingleton().getOrCreateSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, argBufferStride, signature));

	const BufferImpl& argBuffImpl = static_cast<const BufferImpl&>(argBuffer.getBuffer());
	ANKI_ASSERT(!!(argBuffImpl.getBufferUsage() & BufferUsageBit::kIndirectDraw));
	ANKI_ASSERT((argBuffer.getOffset() % 4) == 0);
	ANKI_ASSERT((argBuffer.getRange() % argBufferStride) == 0);
	ANKI_ASSERT(argBufferStride * maxDrawCount == argBuffer.getRange());

	const BufferImpl& countBuffImpl = static_cast<const BufferImpl&>(countBuffer.getBuffer());
	ANKI_ASSERT(countBuffer.getRange() == sizeof(U32));
	ANKI_ASSERT(!!(countBuffImpl.getBufferUsage() & BufferUsageBit::kIndirectDraw));

	self.m_cmdList->ExecuteIndirect(signature, maxDrawCount, &argBuffImpl.getD3DResource(), argBuffer.getOffset(), &countBuffImpl.getD3DResource(),
									countBuffer.getOffset());
}

void CommandBuffer::drawMeshTasks(U32 groupCountX, U32 groupCountY, U32 groupCountZ)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.m_graphicsState.setPrimitiveTopology(PrimitiveTopology::kTriangles);
	self.drawcallCommon();

	self.m_cmdList->DispatchMesh(groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::drawMeshTasksIndirect(const BufferView& argBuffer, U32 drawCount)
{
	ANKI_ASSERT(drawCount > 0);

	ANKI_ASSERT((argBuffer.getOffset() % 4) == 0);
	ANKI_ASSERT(drawCount * sizeof(DispatchIndirectArgs) == argBuffer.getRange());
	const BufferImpl& impl = static_cast<const BufferImpl&>(argBuffer.getBuffer());
	ANKI_ASSERT(!!(impl.getBufferUsage() & BufferUsageBit::kIndirectDraw));

	ANKI_D3D_SELF(CommandBufferImpl);
	self.m_graphicsState.setPrimitiveTopology(PrimitiveTopology::kTriangles);
	self.drawcallCommon();

	ID3D12CommandSignature* signature;
	ANKI_CHECKF(IndirectCommandSignatureFactory::getSingleton().getOrCreateSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH,
																					 sizeof(DispatchIndirectArgs), signature));

	self.m_cmdList->ExecuteIndirect(signature, drawCount, &impl.getD3DResource(), argBuffer.getOffset(), nullptr, 0);
}

void CommandBuffer::dispatchCompute(U32 groupCountX, U32 groupCountY, U32 groupCountZ)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.dispatchCommon();
	self.m_cmdList->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::dispatchComputeIndirect(const BufferView& argBuffer)
{
	ANKI_ASSERT(sizeof(DispatchIndirectArgs) == argBuffer.getRange());
	ANKI_ASSERT(argBuffer.getOffset() % 4 == 0);

	const BufferImpl& impl = static_cast<const BufferImpl&>(argBuffer.getBuffer());
	ANKI_ASSERT(!!(impl.getBufferUsage() & BufferUsageBit::kIndirectDraw));

	ANKI_D3D_SELF(CommandBufferImpl);
	self.dispatchCommon();

	ID3D12CommandSignature* signature;
	ANKI_CHECKF(IndirectCommandSignatureFactory::getSingleton().getOrCreateSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH,
																					 sizeof(DispatchIndirectArgs), signature));

	self.m_cmdList->ExecuteIndirect(signature, 1, &impl.getD3DResource(), argBuffer.getOffset(), nullptr, 0);
}

void CommandBuffer::traceRays([[maybe_unused]] const BufferView& sbtBuffer, [[maybe_unused]] U32 sbtRecordSize,
							  [[maybe_unused]] U32 hitGroupSbtRecordCount, [[maybe_unused]] U32 rayTypeCount, [[maybe_unused]] U32 width,
							  [[maybe_unused]] U32 height, [[maybe_unused]] U32 depth)
{
	ANKI_ASSERT(!"TODO");
}

void CommandBuffer::blitTexture([[maybe_unused]] const TextureView& srcView, [[maybe_unused]] const TextureView& destView)
{
	ANKI_ASSERT(!"TODO");
}

void CommandBuffer::clearTexture([[maybe_unused]] const TextureView& texView, [[maybe_unused]] const ClearValue& clearValue)
{
	ANKI_ASSERT(!"TODO");
}

void CommandBuffer::copyBufferToTexture(const BufferView& buff, const TextureView& texView)
{
	ANKI_D3D_SELF(CommandBufferImpl);

	self.commandCommon();

	const BufferImpl& buffImpl = static_cast<const BufferImpl&>(buff.getBuffer());
	const TextureImpl& texImpl = static_cast<const TextureImpl&>(texView.getTexture());

	const U32 width = texImpl.getWidth() >> texView.getFirstMipmap();
	const U32 height = texImpl.getHeight() >> texView.getFirstMipmap();
	const U32 depth = (texImpl.getTextureType() == TextureType::k3D) ? (texImpl.getDepth() >> texView.getFirstMipmap()) : 1u;
	ANKI_ASSERT(width && height && depth);

	D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
	srcLocation.pResource = &buffImpl.getD3DResource();
	srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	srcLocation.PlacedFootprint.Offset = buff.getOffset();
	srcLocation.PlacedFootprint.Footprint.Format = texImpl.getDxgiFormat();
	srcLocation.PlacedFootprint.Footprint.Width = width;
	srcLocation.PlacedFootprint.Footprint.Height = height;
	srcLocation.PlacedFootprint.Footprint.Depth = depth;
	srcLocation.PlacedFootprint.Footprint.RowPitch = width * getFormatInfo(texImpl.getFormat()).m_texelSize;

	D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
	dstLocation.pResource = &texImpl.getD3DResource();
	dstLocation.SubresourceIndex = texImpl.calcD3DSubresourceIndex(texView.getSubresource());

	self.m_cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
}

void CommandBuffer::fillBuffer(const BufferView& buff, U32 value)
{
	ANKI_ASSERT((buff.getRange() % sizeof(U32)) == 0);
	ANKI_ASSERT(!!(buff.getBuffer().getBufferUsage() & BufferUsageBit::kTransferDestination));

	BufferInitInfo srcBufferInit("FillBufferTmp");
	srcBufferInit.m_mapAccess = BufferMapAccessBit::kWrite;
	srcBufferInit.m_usage = BufferUsageBit::kTransferSource;
	srcBufferInit.m_size = buff.getRange();

	BufferPtr srcBuffer = GrManager::getSingleton().newBuffer(srcBufferInit);

	U32* array = static_cast<U32*>(srcBuffer->map(0, buff.getRange(), BufferMapAccessBit::kWrite));
	for(U32 i = 0; i < buff.getRange() / sizeof(U32); ++i)
	{
		array[i] = value;
	}

	srcBuffer->unmap();

	copyBufferToBuffer(BufferView(srcBuffer.get()), buff);
}

void CommandBuffer::writeOcclusionQueriesResultToBuffer([[maybe_unused]] ConstWeakArray<OcclusionQuery*> queries,
														[[maybe_unused]] const BufferView& buff)
{
	ANKI_ASSERT(!"TODO");
}

void CommandBuffer::copyBufferToBuffer(Buffer* src, Buffer* dst, ConstWeakArray<CopyBufferToBufferInfo> copies)
{
	ANKI_ASSERT(src && dst);
	ANKI_ASSERT(!!(src->getBufferUsage() & BufferUsageBit::kTransferSource));
	ANKI_ASSERT(!!(dst->getBufferUsage() & BufferUsageBit::kTransferDestination));

	ANKI_D3D_SELF(CommandBufferImpl);

	self.commandCommon();

	const BufferImpl& srcImpl = static_cast<const BufferImpl&>(*src);
	const BufferImpl& dstImpl = static_cast<const BufferImpl&>(*dst);

	for(const CopyBufferToBufferInfo& region : copies)
	{
		self.m_cmdList->CopyBufferRegion(&dstImpl.getD3DResource(), region.m_destinationOffset, &srcImpl.getD3DResource(), region.m_sourceOffset,
										 region.m_range);
	}
}

void CommandBuffer::buildAccelerationStructure([[maybe_unused]] AccelerationStructure* as, [[maybe_unused]] const BufferView& scratchBuffer)
{
	ANKI_ASSERT(!"TODO");
}

void CommandBuffer::upscale([[maybe_unused]] GrUpscaler* upscaler, [[maybe_unused]] const TextureView& inColor,
							[[maybe_unused]] const TextureView& outUpscaledColor, [[maybe_unused]] const TextureView& motionVectors,
							[[maybe_unused]] const TextureView& depth, [[maybe_unused]] const TextureView& exposure,
							[[maybe_unused]] Bool resetAccumulation, [[maybe_unused]] const Vec2& jitterOffset,
							[[maybe_unused]] const Vec2& motionVectorsScale)
{
	ANKI_ASSERT(!"TODO");
}

void CommandBuffer::setPipelineBarrier(ConstWeakArray<TextureBarrierInfo> textures, ConstWeakArray<BufferBarrierInfo> buffers,
									   ConstWeakArray<AccelerationStructureBarrierInfo> accelerationStructures)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.commandCommon();

	DynamicArray<D3D12_TEXTURE_BARRIER, MemoryPoolPtrWrapper<StackMemoryPool>> texBarriers(self.m_fastPool);
	DynamicArray<D3D12_BUFFER_BARRIER, MemoryPoolPtrWrapper<StackMemoryPool>> bufferBarriers(self.m_fastPool);

	for(const TextureBarrierInfo& barrier : textures)
	{
		const TextureImpl& impl = static_cast<const TextureImpl&>(barrier.m_textureView.getTexture());
		D3D12_TEXTURE_BARRIER& d3dBarrier = *texBarriers.emplaceBack();
		d3dBarrier = impl.computeBarrierInfo(barrier.m_previousUsage, barrier.m_nextUsage, barrier.m_textureView.getSubresource());
	}

	for(const BufferBarrierInfo& barrier : buffers)
	{
		const BufferImpl& impl = static_cast<const BufferImpl&>(barrier.m_bufferView.getBuffer());
		D3D12_BUFFER_BARRIER& d3dBarrier = *bufferBarriers.emplaceBack();
		d3dBarrier = impl.computeBarrier(barrier.m_previousUsage, barrier.m_nextUsage);
	}

	ANKI_ASSERT(accelerationStructures.getSize() == 0 && "TODO");

	Array<D3D12_BARRIER_GROUP, 3> barrierGroups;
	U32 barrierGroupCount = 0;

	if(texBarriers.getSize())
	{
		barrierGroups[barrierGroupCount++] = {.Type = D3D12_BARRIER_TYPE_TEXTURE,
											  .NumBarriers = texBarriers.getSize(),
											  .pTextureBarriers = texBarriers.getBegin()};
	}

	if(bufferBarriers.getSize())
	{
		barrierGroups[barrierGroupCount++] = {.Type = D3D12_BARRIER_TYPE_BUFFER,
											  .NumBarriers = bufferBarriers.getSize(),
											  .pBufferBarriers = bufferBarriers.getBegin()};
	}

	ANKI_ASSERT(barrierGroupCount > 0);
	self.m_cmdList->Barrier(barrierGroupCount, barrierGroups.getBegin());
}

void CommandBuffer::beginOcclusionQuery([[maybe_unused]] OcclusionQuery* query)
{
	ANKI_ASSERT(!"TODO");
}

void CommandBuffer::endOcclusionQuery([[maybe_unused]] OcclusionQuery* query)
{
	ANKI_ASSERT(!"TODO");
}

void CommandBuffer::beginPipelineQuery(PipelineQuery* query)
{
	ANKI_ASSERT(query);
	ANKI_D3D_SELF(CommandBufferImpl);

	self.commandCommon();
	self.m_mcmdb->pushObjectRef(query);

	const PipelineQueryImpl& impl = static_cast<const PipelineQueryImpl&>(*query);

	const QueryInfo qinfo = PrimitivesPassedClippingFactory::getSingleton().getQueryInfo(impl.m_handle);

	self.m_cmdList->BeginQuery(qinfo.m_queryHeap, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, qinfo.m_indexInHeap);
}

void CommandBuffer::endPipelineQuery(PipelineQuery* query)
{
	ANKI_ASSERT(query);
	ANKI_D3D_SELF(CommandBufferImpl);

	self.commandCommon();
	self.m_mcmdb->pushObjectRef(query);

	const PipelineQueryImpl& impl = static_cast<const PipelineQueryImpl&>(*query);
	self.m_pipelineQueries.emplaceBack(impl.m_handle);

	const QueryInfo qinfo = PrimitivesPassedClippingFactory::getSingleton().getQueryInfo(impl.m_handle);

	self.m_cmdList->EndQuery(qinfo.m_queryHeap, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, qinfo.m_indexInHeap);
}

void CommandBuffer::writeTimestamp(TimestampQuery* query)
{
	ANKI_ASSERT(query);
	ANKI_D3D_SELF(CommandBufferImpl);

	self.commandCommon();
	self.m_mcmdb->pushObjectRef(query);

	const TimestampQueryImpl& impl = static_cast<const TimestampQueryImpl&>(*query);
	self.m_timestampQueries.emplaceBack(impl.m_handle);

	const QueryInfo qinfo = TimestampQueryFactory::getSingleton().getQueryInfo(impl.m_handle);

	// Make sure all the work has finished (mesa's dozen does that)
	const D3D12_GLOBAL_BARRIER barrier = {.SyncBefore = D3D12_BARRIER_SYNC_ALL,
										  .SyncAfter = D3D12_BARRIER_SYNC_NONE,
										  .AccessBefore = D3D12_BARRIER_ACCESS_COMMON,
										  .AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS};

	const D3D12_BARRIER_GROUP barrierGroup = {.Type = D3D12_BARRIER_TYPE_GLOBAL, .NumBarriers = 1, .pGlobalBarriers = &barrier};
	self.m_cmdList->Barrier(1, &barrierGroup);

	self.m_cmdList->EndQuery(qinfo.m_queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, qinfo.m_indexInHeap);
}

Bool CommandBuffer::isEmpty() const
{
	ANKI_D3D_SELF_CONST(CommandBufferImpl);
	return self.m_commandCount == 0;
}

void CommandBuffer::setPushConstants(const void* data, U32 dataSize)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	self.m_descriptors.setRootConstants(data, dataSize);
}

void CommandBuffer::setLineWidth(F32 width)
{
	ANKI_D3D_SELF(CommandBufferImpl);
	if(width != 1.0f && !self.m_lineWidthWarningAlreadyShown)
	{
		ANKI_D3D_LOGW("D3D doesn't support line widths other than 1.0. This warning will not show up again");
		self.m_lineWidthWarningAlreadyShown = true;
	}
}

void CommandBuffer::pushDebugMarker(CString name, Vec3 color)
{
	ANKI_D3D_SELF(CommandBufferImpl);

	if(self.m_debugMarkersEnabled)
	{
		const UVec3 coloru(color * 255.0f);
		const U64 val = (U64(coloru.x()) << 48) | (U64(coloru.x()) << 32) | (U64(coloru.x()) << 16);

		PIXBeginEvent(self.m_cmdList, val, "%s", name.cstr());
	}
}

void CommandBuffer::popDebugMarker()
{
	ANKI_D3D_SELF(CommandBufferImpl);

	if(self.m_debugMarkersEnabled)
	{
		PIXEndEvent(self.m_cmdList);
	}
}

CommandBufferImpl::~CommandBufferImpl()
{
}

Error CommandBufferImpl::init(const CommandBufferInitInfo& init)
{
	ANKI_CHECK(CommandBufferFactory::getSingleton().newCommandBuffer(init.m_flags, m_mcmdb));

	m_cmdList = &m_mcmdb->getCmdList();
	m_fastPool = &m_mcmdb->getFastMemoryPool();

	m_descriptors.init(m_fastPool);

	m_debugMarkersEnabled = g_debugMarkersCVar.get();

	return Error::kNone;
}

void CommandBufferImpl::postSubmitWork(MicroFence* fence)
{
	for(QueryHandle handle : m_timestampQueries)
	{
		TimestampQueryFactory::getSingleton().postSubmitWork(handle, fence);
	}

	for(QueryHandle handle : m_pipelineQueries)
	{
		PrimitivesPassedClippingFactory::getSingleton().postSubmitWork(handle, fence);
	}
}

} // end namespace anki