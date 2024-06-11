// Copyright (C) 2009-present, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Renderer/TemporalAA.h>
#include <AnKi/Renderer/Renderer.h>
#include <AnKi/Renderer/GBuffer.h>
#include <AnKi/Renderer/LightShading.h>
#include <AnKi/Renderer/Tonemapping.h>
#include <AnKi/Renderer/MotionVectors.h>
#include <AnKi/Core/CVarSet.h>
#include <AnKi/Util/Tracer.h>

namespace anki {

Error TemporalAA::init()
{
	const Error err = initInternal();
	if(err)
	{
		ANKI_R_LOGE("Failed to init TAA");
	}

	return Error::kNone;
}

Error TemporalAA::initInternal()
{
	ANKI_R_LOGV("Initializing TAA");

	ANKI_CHECK(loadShaderProgram("ShaderBinaries/TemporalAA.ankiprogbin", {{"VARIANCE_CLIPPING", 1}, {"YCBCR", 0}}, m_prog, m_grProg));

	for(U32 i = 0; i < 2; ++i)
	{
		TextureUsageBit usage = TextureUsageBit::kSampledFragment | TextureUsageBit::kSampledCompute;
		usage |= (g_preferComputeCVar.get()) ? TextureUsageBit::kStorageComputeWrite : TextureUsageBit::kFramebufferWrite;

		TextureInitInfo texinit =
			getRenderer().create2DRenderTargetInitInfo(getRenderer().getInternalResolution().x(), getRenderer().getInternalResolution().y(),
													   getRenderer().getHdrFormat(), usage, String().sprintf("TemporalAA #%u", i).cstr());

		m_rtTextures[i] = getRenderer().createAndClearRenderTarget(texinit, TextureUsageBit::kSampledFragment);
	}

	m_tonemappedRtDescr = getRenderer().create2DRenderTargetDescription(
		getRenderer().getInternalResolution().x(), getRenderer().getInternalResolution().y(),
		(GrManager::getSingleton().getDeviceCapabilities().m_unalignedBbpTextureFormats) ? Format::kR8G8B8_Unorm : Format::kR8G8B8A8_Unorm,
		"TemporalAA Tonemapped");
	m_tonemappedRtDescr.bake();

	return Error::kNone;
}

void TemporalAA::populateRenderGraph(RenderingContext& ctx)
{
	ANKI_TRACE_SCOPED_EVENT(TemporalAA);
	RenderGraphBuilder& rgraph = ctx.m_renderGraphDescr;

	const U32 historyRtIdx = (getRenderer().getFrameCount() + 1) & 1;
	const U32 renderRtIdx = !historyRtIdx;
	const Bool preferCompute = g_preferComputeCVar.get();

	// Import RTs
	if(m_rtTexturesImportedOnce) [[likely]]
	{
		m_runCtx.m_historyRt = rgraph.importRenderTarget(m_rtTextures[historyRtIdx].get());
	}
	else
	{
		m_runCtx.m_historyRt = rgraph.importRenderTarget(m_rtTextures[historyRtIdx].get(), TextureUsageBit::kSampledFragment);
		m_rtTexturesImportedOnce = true;
	}

	m_runCtx.m_renderRt = rgraph.importRenderTarget(m_rtTextures[renderRtIdx].get(), TextureUsageBit::kNone);
	m_runCtx.m_tonemappedRt = rgraph.newRenderTarget(m_tonemappedRtDescr);

	// Create pass
	TextureUsageBit readUsage;
	RenderPassBase* prpass;
	if(preferCompute)
	{
		NonGraphicsRenderPass& pass = rgraph.newNonGraphicsRenderPass("TemporalAA");

		pass.newTextureDependency(m_runCtx.m_renderRt, TextureUsageBit::kStorageComputeWrite);
		pass.newTextureDependency(m_runCtx.m_tonemappedRt, TextureUsageBit::kStorageComputeWrite);

		readUsage = TextureUsageBit::kSampledCompute;

		prpass = &pass;
	}
	else
	{
		GraphicsRenderPass& pass = rgraph.newGraphicsRenderPass("TemporalAA");
		pass.setRenderpassInfo({GraphicsRenderPassTargetDesc(m_runCtx.m_renderRt), GraphicsRenderPassTargetDesc(m_runCtx.m_tonemappedRt)});

		pass.newTextureDependency(m_runCtx.m_renderRt, TextureUsageBit::kFramebufferWrite);
		pass.newTextureDependency(m_runCtx.m_tonemappedRt, TextureUsageBit::kFramebufferWrite);

		readUsage = TextureUsageBit::kSampledFragment;

		prpass = &pass;
	}

	prpass->newTextureDependency(getRenderer().getGBuffer().getDepthRt(), readUsage);
	prpass->newTextureDependency(getRenderer().getLightShading().getRt(), readUsage);
	prpass->newTextureDependency(m_runCtx.m_historyRt, readUsage);
	prpass->newTextureDependency(getRenderer().getMotionVectors().getMotionVectorsRt(), readUsage);

	prpass->setWork([this](RenderPassWorkContext& rgraphCtx) {
		ANKI_TRACE_SCOPED_EVENT(TemporalAA);
		CommandBuffer& cmdb = *rgraphCtx.m_commandBuffer;

		cmdb.bindShaderProgram(m_grProg.get());

		cmdb.bindSampler(ANKI_REG(s0), getRenderer().getSamplers().m_trilinearClamp.get());
		rgraphCtx.bindTexture(ANKI_REG(t0), getRenderer().getLightShading().getRt());
		rgraphCtx.bindTexture(ANKI_REG(t1), m_runCtx.m_historyRt);
		rgraphCtx.bindTexture(ANKI_REG(t2), getRenderer().getMotionVectors().getMotionVectorsRt());
		rgraphCtx.bindTexture(ANKI_REG(u0), getRenderer().getTonemapping().getRt());

		if(g_preferComputeCVar.get())
		{
			rgraphCtx.bindTexture(ANKI_REG(u1), m_runCtx.m_renderRt);
			rgraphCtx.bindTexture(ANKI_REG(u2), m_runCtx.m_tonemappedRt);

			dispatchPPCompute(cmdb, 8, 8, getRenderer().getInternalResolution().x(), getRenderer().getInternalResolution().y());
		}
		else
		{
			cmdb.setViewport(0, 0, getRenderer().getInternalResolution().x(), getRenderer().getInternalResolution().y());

			cmdb.draw(PrimitiveTopology::kTriangles, 3);
		}
	});
}

} // end namespace anki
