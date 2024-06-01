// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Renderer/DownscaleBlur.h>
#include <AnKi/Renderer/Renderer.h>
#include <AnKi/Renderer/LightShading.h>
#include <AnKi/Renderer/Tonemapping.h>
#include <AnKi/Core/CVarSet.h>
#include <AnKi/Util/Tracer.h>

namespace anki {

Error DownscaleBlur::init()
{
	const Error err = initInternal();
	if(err)
	{
		ANKI_R_LOGE("Failed to initialize downscale blur");
	}

	return err;
}

Error DownscaleBlur::initInternal()
{
	m_passCount =
		computeMaxMipmapCount2d(getRenderer().getInternalResolution().x(), getRenderer().getInternalResolution().y(), kDownscaleBurDownTo) - 1;

	const UVec2 rez = getRenderer().getInternalResolution() / 2;
	ANKI_R_LOGV("Initializing downscale pyramid. Resolution %ux%u, mip count %u", rez.x(), rez.y(), m_passCount);

	const Bool preferCompute = g_preferComputeCVar.get();

	// Create the miped texture
	TextureInitInfo texinit = getRenderer().create2DRenderTargetDescription(rez.x(), rez.y(), getRenderer().getHdrFormat(), "DownscaleBlur");
	texinit.m_usage = TextureUsageBit::kSampledFragment | TextureUsageBit::kSampledCompute;
	if(preferCompute)
	{
		texinit.m_usage |= TextureUsageBit::kStorageComputeWrite;
	}
	else
	{
		texinit.m_usage |= TextureUsageBit::kFramebufferWrite;
	}
	texinit.m_mipmapCount = U8(m_passCount);
	m_rtTex = getRenderer().createAndClearRenderTarget(texinit, TextureUsageBit::kSampledCompute);

	// Shader programs
	ANKI_CHECK(loadShaderProgram("ShaderBinaries/DownscaleBlur.ankiprogbin", m_prog, m_grProg));

	return Error::kNone;
}

void DownscaleBlur::importRenderTargets(RenderingContext& ctx)
{
	RenderGraphDescription& rgraph = ctx.m_renderGraphDescr;
	m_runCtx.m_rt = rgraph.importRenderTarget(m_rtTex.get(), TextureUsageBit::kSampledCompute);
}

void DownscaleBlur::populateRenderGraph(RenderingContext& ctx)
{
	ANKI_TRACE_SCOPED_EVENT(DownscaleBlur);
	RenderGraphDescription& rgraph = ctx.m_renderGraphDescr;

	// Create passes
	static constexpr Array<CString, 8> passNames = {"Down/Blur #0", "Down/Blur #1", "Down/Blur #2", "Down/Blur #3",
													"Down/Blur #4", "Down/Blur #5", "Down/Blur #6", "Down/Blur #7"};
	const RenderTargetHandle inRt = getRenderer().getLightShading().getRt();

	for(U32 i = 0; i < m_passCount; ++i)
	{
		RenderPassDescriptionBase* ppass;
		if(g_preferComputeCVar.get())
		{
			ppass = &rgraph.newComputeRenderPass(passNames[i]);
		}
		else
		{
			GraphicsRenderPassDescription& pass = rgraph.newGraphicsRenderPass(passNames[i]);

			RenderTargetInfo rtInf(m_runCtx.m_rt);
			rtInf.m_subresource.m_mipmap = i;
			pass.setRenderpassInfo({rtInf});

			ppass = &pass;
		}

		const TextureUsageBit readUsage = (g_preferComputeCVar.get()) ? TextureUsageBit::kSampledCompute : TextureUsageBit::kSampledFragment;
		const TextureUsageBit writeUsage = (g_preferComputeCVar.get()) ? TextureUsageBit::kStorageComputeWrite : TextureUsageBit::kFramebufferWrite;

		if(i > 0)
		{
			const TextureSubresourceDescriptor sampleSubresource = TextureSubresourceDescriptor::surface(i - 1, 0, 0);
			const TextureSubresourceDescriptor renderSubresource = TextureSubresourceDescriptor::surface(i, 0, 0);

			ppass->newTextureDependency(m_runCtx.m_rt, writeUsage, renderSubresource);
			ppass->newTextureDependency(m_runCtx.m_rt, readUsage, sampleSubresource);
		}
		else
		{
			ppass->newTextureDependency(m_runCtx.m_rt, writeUsage, TextureSubresourceDescriptor::firstSurface());
			ppass->newTextureDependency(inRt, readUsage);
		}

		ppass->setWork([this, i](RenderPassWorkContext& rgraphCtx) {
			run(i, rgraphCtx);
		});
	}
}

void DownscaleBlur::run(U32 passIdx, RenderPassWorkContext& rgraphCtx)
{
	CommandBuffer& cmdb = *rgraphCtx.m_commandBuffer;

	cmdb.bindShaderProgram(m_grProg.get());

	const U32 vpWidth = m_rtTex->getWidth() >> passIdx;
	const U32 vpHeight = m_rtTex->getHeight() >> passIdx;

	cmdb.bindSampler(ANKI_REG(s0), getRenderer().getSamplers().m_trilinearClamp.get());

	if(passIdx > 0)
	{
		rgraphCtx.bindTexture(ANKI_REG(t0), m_runCtx.m_rt, TextureSubresourceDescriptor::surface(passIdx - 1, 0, 0));
	}
	else
	{
		rgraphCtx.bindTexture(ANKI_REG(t0), getRenderer().getLightShading().getRt());
	}

	rgraphCtx.bindTexture(ANKI_REG(u0), getRenderer().getTonemapping().getRt());

	if(g_preferComputeCVar.get())
	{
		const Vec4 fbSize(F32(vpWidth), F32(vpHeight), 0.0f, 0.0f);
		cmdb.setPushConstants(&fbSize, sizeof(fbSize));

		rgraphCtx.bindTexture(ANKI_REG(u1), m_runCtx.m_rt, TextureSubresourceDescriptor::surface(passIdx, 0, 0));

		dispatchPPCompute(cmdb, 8, 8, vpWidth, vpHeight);
	}
	else
	{
		cmdb.setViewport(0, 0, vpWidth, vpHeight);

		cmdb.draw(PrimitiveTopology::kTriangles, 3);
	}
}

} // end namespace anki
