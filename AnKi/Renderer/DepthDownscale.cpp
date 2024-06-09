// Copyright (C) 2009-present, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Renderer/DepthDownscale.h>
#include <AnKi/Renderer/Renderer.h>
#include <AnKi/Renderer/GBuffer.h>
#include <AnKi/Core/CVarSet.h>
#include <AnKi/Util/Tracer.h>

#if ANKI_COMPILER_GCC_COMPATIBLE
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wunused-function"
#	pragma GCC diagnostic ignored "-Wignored-qualifiers"
#elif ANKI_COMPILER_MSVC
#	pragma warning(push)
#	pragma warning(disable : 4505)
#endif
#define A_CPU
#include <ThirdParty/FidelityFX/ffx_a.h>
#include <ThirdParty/FidelityFX/ffx_spd.h>
#if ANKI_COMPILER_GCC_COMPATIBLE
#	pragma GCC diagnostic pop
#elif ANKI_COMPILER_MSVC
#	pragma warning(pop)
#endif

namespace anki {

DepthDownscale::~DepthDownscale()
{
}

Error DepthDownscale::initInternal()
{
	const U32 width = getRenderer().getInternalResolution().x() / 2;
	const U32 height = getRenderer().getInternalResolution().y() / 2;

	m_mipCount = 2;

	const UVec2 lastMipSize = UVec2(width, height) >> (m_mipCount - 1);

	ANKI_R_LOGV("Initializing HiZ. Mip count %u, last mip size %ux%u", m_mipCount, lastMipSize.x(), lastMipSize.y());

	const Bool preferCompute = g_preferComputeCVar.get();

	// Create RT descr
	{
		m_rtDescr = getRenderer().create2DRenderTargetDescription(width, height, Format::kR32_Sfloat, "Downscaled depth");
		m_rtDescr.m_mipmapCount = U8(m_mipCount);
		m_rtDescr.bake();
	}

	// Progs
	ANKI_CHECK(loadShaderProgram("ShaderBinaries/DepthDownscale.ankiprogbin", {{"WAVE_OPERATIONS", 1}}, m_prog, m_grProg));

	// Counter buffer
	if(preferCompute)
	{
		BufferInitInfo buffInit("Depth downscale counter buffer");
		buffInit.m_size = sizeof(U32);
		buffInit.m_usage = BufferUsageBit::kStorageComputeWrite | BufferUsageBit::kTransferDestination;
		m_counterBuffer = GrManager::getSingleton().newBuffer(buffInit);

		// Zero it
		CommandBufferInitInfo cmdbInit;
		cmdbInit.m_flags |= CommandBufferFlag::kSmallBatch;
		CommandBufferPtr cmdb = GrManager::getSingleton().newCommandBuffer(cmdbInit);

		cmdb->fillBuffer(BufferView(m_counterBuffer.get()), 0);

		FencePtr fence;
		cmdb->endRecording();
		GrManager::getSingleton().submit(cmdb.get(), {}, &fence);

		fence->clientWait(6.0_sec);
	}

	return Error::kNone;
}

Error DepthDownscale::init()
{
	const Error err = initInternal();
	if(err)
	{
		ANKI_R_LOGE("Failed to initialize depth downscale passes");
	}

	return err;
}

void DepthDownscale::populateRenderGraph(RenderingContext& ctx)
{
	ANKI_TRACE_SCOPED_EVENT(DepthDownscale);
	RenderGraphDescription& rgraph = ctx.m_renderGraphDescr;

	m_runCtx.m_rt = rgraph.newRenderTarget(m_rtDescr);

	if(g_preferComputeCVar.get())
	{
		// Do it with compute

		ComputeRenderPassDescription& pass = rgraph.newComputeRenderPass("Depth downscale");

		pass.newTextureDependency(getRenderer().getGBuffer().getDepthRt(), TextureUsageBit::kSampledCompute);

		pass.newTextureDependency(m_runCtx.m_rt, TextureUsageBit::kStorageComputeWrite);

		pass.setWork([this](RenderPassWorkContext& rgraphCtx) {
			CommandBuffer& cmdb = *rgraphCtx.m_commandBuffer;

			cmdb.bindShaderProgram(m_grProg.get());

			varAU2(dispatchThreadGroupCountXY);
			varAU2(workGroupOffset); // needed if Left and Top are not 0,0
			varAU2(numWorkGroupsAndMips);
			varAU4(rectInfo) = initAU4(0, 0, getRenderer().getInternalResolution().x(), getRenderer().getInternalResolution().y());
			SpdSetup(dispatchThreadGroupCountXY, workGroupOffset, numWorkGroupsAndMips, rectInfo, m_mipCount);

			DepthDownscaleUniforms pc;
			pc.m_threadgroupCount = numWorkGroupsAndMips[0];
			pc.m_mipmapCount = numWorkGroupsAndMips[1];
			pc.m_srcTexSizeOverOne = 1.0f / Vec2(getRenderer().getInternalResolution());

			cmdb.setPushConstants(&pc, sizeof(pc));

			for(U32 mip = 0; mip < kMaxMipsSinglePassDownsamplerCanProduce; ++mip)
			{
				TextureSubresourceDescriptor surface = TextureSubresourceDescriptor::firstSurface();
				if(mip < m_mipCount)
				{
					surface.m_mipmap = mip;
				}
				else
				{
					surface.m_mipmap = 0; // Put something random
				}

				rgraphCtx.bindTexture(Register(HlslResourceType::kUav, mip + 1), m_runCtx.m_rt, surface);
			}

			cmdb.bindStorageBuffer(ANKI_REG(u0), BufferView(m_counterBuffer.get(), 0, sizeof(U32)));

			cmdb.bindSampler(ANKI_REG(s0), getRenderer().getSamplers().m_trilinearClamp.get());
			rgraphCtx.bindTexture(ANKI_REG(t0), getRenderer().getGBuffer().getDepthRt());

			cmdb.dispatchCompute(dispatchThreadGroupCountXY[0], dispatchThreadGroupCountXY[1], 1);
		});
	}
	else
	{
		// Do it with raster

		for(U32 mip = 0; mip < m_mipCount; ++mip)
		{
			static constexpr Array<CString, 4> passNames = {"Depth downscale #1", "Depth downscale #2", "Depth downscale #3", "Depth downscale #4"};
			GraphicsRenderPassDescription& pass = rgraph.newGraphicsRenderPass(passNames[mip]);

			RenderTargetInfo rti(m_runCtx.m_rt);
			rti.m_subresource.m_mipmap = mip;
			pass.setRenderpassInfo({rti});

			if(mip == 0)
			{
				pass.newTextureDependency(getRenderer().getGBuffer().getDepthRt(), TextureUsageBit::kSampledFragment);
			}
			else
			{
				pass.newTextureDependency(m_runCtx.m_rt, TextureUsageBit::kSampledFragment, TextureSubresourceDescriptor::surface(mip - 1, 0, 0));
			}

			pass.newTextureDependency(m_runCtx.m_rt, TextureUsageBit::kFramebufferWrite, TextureSubresourceDescriptor::surface(mip, 0, 0));

			pass.setWork([this, mip](RenderPassWorkContext& rgraphCtx) {
				CommandBuffer& cmdb = *rgraphCtx.m_commandBuffer;

				cmdb.bindShaderProgram(m_grProg.get());
				cmdb.bindSampler(ANKI_REG(s0), getRenderer().getSamplers().m_trilinearClamp.get());

				if(mip == 0)
				{
					rgraphCtx.bindTexture(ANKI_REG(t0), getRenderer().getGBuffer().getDepthRt());
				}
				else
				{
					rgraphCtx.bindTexture(ANKI_REG(t0), m_runCtx.m_rt, TextureSubresourceDescriptor::surface(mip - 1, 0, 0));
				}

				const UVec2 size = (getRenderer().getInternalResolution() / 2) >> mip;
				cmdb.setViewport(0, 0, size.x(), size.y());
				cmdb.draw(PrimitiveTopology::kTriangles, 3);
			});
		}
	}
}

} // end namespace anki
