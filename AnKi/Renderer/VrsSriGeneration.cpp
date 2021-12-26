// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Renderer/VrsSriGeneration.h>
#include <AnKi/Renderer/Renderer.h>
#include <AnKi/Renderer/DownscaleBlur.h>
#include <AnKi/Core/ConfigSet.h>

namespace anki {

VrsSriGeneration::VrsSriGeneration(Renderer* r)
	: RendererObject(r)
{
	registerDebugRenderTarget("VRS");
}

VrsSriGeneration::~VrsSriGeneration()
{
}

Error VrsSriGeneration::init()
{
	const Error err = initInternal();
	if(err)
	{
		ANKI_R_LOGE("Failed to initialize VRS SRI generation");
	}
	return err;
}

Error VrsSriGeneration::initInternal()
{
	const UVec2 rez = (m_r->getInternalResolution() + m_sriTexelDimension - 1) / m_sriTexelDimension;

	ANKI_R_LOGV("Intializing VRS SRI generation. SRI resolution %ux%u", rez.x(), rez.y());

	// Bake descriptors
	m_rtDescr = m_r->create2DRenderTargetDescription(rez.x(), rez.y(), Format::R8_UINT, "VRS SRI");
	m_rtDescr.bake();

	m_fbDescr.m_colorAttachmentCount = 1;
	m_fbDescr.bake();

	// Load programs
	ANKI_CHECK(getResourceManager().loadResource("AnKi/Shaders/VrsSriGenerationCompute.ankiprog", m_prog));
	ShaderProgramResourceVariantInitInfo variantInit(m_prog);
	// SRI_TEXEL_DIMENSION is half because the input image is quarter resolution
	variantInit.addMutation("SRI_TEXEL_DIMENSION", m_sriTexelDimension / 2u);
	const ShaderProgramResourceVariant* variant;
	m_prog->getOrCreateVariant(variantInit, variant);
	m_grProg = variant->getProgram();

	ANKI_CHECK(getResourceManager().loadResource("AnKi/Shaders/VrsSriVisualizeRenderTarget.ankiprog", m_visualizeProg));
	m_visualizeProg->getOrCreateVariant(variant);
	m_visualizeGrProg = variant->getProgram();

	return Error::NONE;
}

void VrsSriGeneration::getDebugRenderTarget(CString rtName, RenderTargetHandle& handle,
											ShaderProgramPtr& optionalShaderProgram) const
{
	ANKI_ASSERT(rtName == "VRS");
	handle = m_runCtx.m_rt;
	optionalShaderProgram = m_visualizeGrProg;
}

void VrsSriGeneration::populateRenderGraph(RenderingContext& ctx)
{
	const Bool enableVrs = getGrManager().getDeviceCapabilities().m_vrs && getConfig().getRVrs();
	if(!enableVrs)
	{
		return;
	}

	RenderGraphDescription& rgraph = ctx.m_renderGraphDescr;
	m_runCtx.m_rt = rgraph.newRenderTarget(m_rtDescr);

	ComputeRenderPassDescription& pass = rgraph.newComputeRenderPass("VRS SRI generation");

	pass.newDependency(RenderPassDependency(m_runCtx.m_rt, TextureUsageBit::IMAGE_COMPUTE_WRITE));
	pass.newDependency(RenderPassDependency(m_r->getDownscaleBlur().getRt(), TextureUsageBit::SAMPLED_COMPUTE));

	pass.setWork([this](RenderPassWorkContext& rgraphCtx) {
		CommandBufferPtr& cmdb = rgraphCtx.m_commandBuffer;

		cmdb->bindShaderProgram(m_grProg);

		rgraphCtx.bindColorTexture(0, 0, m_r->getDownscaleBlur().getRt());
		rgraphCtx.bindImage(0, 1, m_runCtx.m_rt);

		const U32 workgroupSize = m_sriTexelDimension / 2u;
		dispatchPPCompute(cmdb, workgroupSize, workgroupSize, m_r->getInternalResolution().x() / 2,
						  m_r->getInternalResolution().y() / 2);
	});
}

} // end namespace anki
