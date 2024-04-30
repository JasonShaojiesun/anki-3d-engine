// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Renderer/ForwardShading.h>
#include <AnKi/Renderer/Renderer.h>
#include <AnKi/Renderer/GBuffer.h>
#include <AnKi/Renderer/LightShading.h>
#include <AnKi/Renderer/ShadowMapping.h>
#include <AnKi/Renderer/DepthDownscale.h>
#include <AnKi/Renderer/LensFlare.h>
#include <AnKi/Renderer/ClusterBinning.h>
#include <AnKi/Renderer/LensFlare.h>
#include <AnKi/Renderer/GBuffer.h>
#include <AnKi/Renderer/Dbg.h>
#include <AnKi/Renderer/VolumetricLightingAccumulation.h>
#include <AnKi/Shaders/Include/MaterialTypes.h>
#include <AnKi/Core/App.h>
#include <AnKi/Util/Tracer.h>

namespace anki {

void ForwardShading::populateRenderGraph(RenderingContext& ctx)
{
	m_runCtx = {};
	RenderGraphDescription& rgraph = ctx.m_renderGraphDescr;

	const Array<F32, kMaxLodCount - 1> lodDistances = {g_lod0MaxDistanceCVar.get(), g_lod1MaxDistanceCVar.get()};

	FrustumGpuVisibilityInput visIn;
	visIn.m_passesName = "FW shading";
	visIn.m_technique = RenderingTechnique::kForward;
	visIn.m_viewProjectionMatrix = ctx.m_matrices.m_viewProjection;
	visIn.m_lodReferencePoint = ctx.m_matrices.m_cameraTransform.getTranslationPart().xyz();
	visIn.m_lodDistances = lodDistances;
	visIn.m_rgraph = &rgraph;
	visIn.m_gatherAabbIndices = g_dbgCVar.get();
	RenderTargetHandle hzb = getRenderer().getGBuffer().getHzbRt();
	visIn.m_hzbRt = &hzb;
	visIn.m_viewportSize = getRenderer().getInternalResolution();

	getRenderer().getGpuVisibility().populateRenderGraph(visIn, m_runCtx.m_visOut);

	if(getRenderer().runSoftwareMeshletRendering())
	{
		GpuMeshletVisibilityInput meshIn;
		meshIn.m_passesName = "FW shading";
		meshIn.m_technique = RenderingTechnique::kForward;
		meshIn.m_viewProjectionMatrix = ctx.m_matrices.m_viewProjection;
		meshIn.m_cameraTransform = ctx.m_matrices.m_cameraTransform;
		meshIn.m_viewportSize = getRenderer().getInternalResolution();
		meshIn.m_rgraph = &rgraph;
		meshIn.m_hzbRt = getRenderer().getGBuffer().getHzbRt();
		meshIn.fillBuffers(m_runCtx.m_visOut);

		getRenderer().getGpuVisibility().populateRenderGraph(meshIn, m_runCtx.m_meshVisOut);
	}
}

void ForwardShading::run(const RenderingContext& ctx, RenderPassWorkContext& rgraphCtx)
{
	ANKI_TRACE_SCOPED_EVENT(ForwardShading);

	CommandBuffer& cmdb = *rgraphCtx.m_commandBuffer;

	if(m_runCtx.m_visOut.containsDrawcalls())
	{
		// Set state
		cmdb.setDepthWrite(false);
		cmdb.setBlendFactors(0, BlendFactor::kSrcAlpha, BlendFactor::kOneMinusSrcAlpha);

		// Bind stuff
		const U32 set = U32(MaterialSet::kGlobal);
		cmdb.bindSampler(set, U32(MaterialBinding::kLinearClampSampler), getRenderer().getSamplers().m_trilinearClamp.get());
		cmdb.bindSampler(set, U32(MaterialBinding::kShadowSampler), getRenderer().getSamplers().m_trilinearClampShadow.get());

		rgraphCtx.bindTexture(set, U32(MaterialBinding::kDepthRt), getRenderer().getDepthDownscale().getRt(),
							  DepthDownscale::kQuarterInternalResolution);
		rgraphCtx.bindTexture(set, U32(MaterialBinding::kLightVolume), getRenderer().getVolumetricLightingAccumulation().getRt());

		cmdb.bindUniformBuffer(set, U32(MaterialBinding::kClusterShadingUniforms), ctx.m_globalRenderingUniformsBuffer);

		cmdb.bindStorageBuffer(set, U32(MaterialBinding::kClusterShadingLights),
							   getRenderer().getClusterBinning().getPackedObjectsBuffer(GpuSceneNonRenderableObjectType::kLight));

		rgraphCtx.bindTexture(set, U32(MaterialBinding::kClusterShadingLights) + 1, getRenderer().getShadowMapping().getShadowmapRt());

		cmdb.bindStorageBuffer(set, U32(MaterialBinding::kClusters), getRenderer().getClusterBinning().getClustersBuffer());

		// Draw
		RenderableDrawerArguments args;
		args.m_viewMatrix = ctx.m_matrices.m_view;
		args.m_cameraTransform = ctx.m_matrices.m_cameraTransform;
		args.m_viewProjectionMatrix = ctx.m_matrices.m_viewProjectionJitter;
		args.m_previousViewProjectionMatrix = ctx.m_matrices.m_jitter * ctx.m_prevMatrices.m_viewProjection;
		args.m_sampler = getRenderer().getSamplers().m_trilinearRepeatAnisoResolutionScalingBias.get();
		args.m_renderingTechinuqe = RenderingTechnique::kForward;
		args.m_viewport = UVec4(0, 0, getRenderer().getInternalResolution());
		args.fill(m_runCtx.m_visOut);

		if(m_runCtx.m_meshVisOut.isFilled())
		{
			args.fill(m_runCtx.m_meshVisOut);
		}

		getRenderer().getSceneDrawer().drawMdi(args, cmdb);

		// Restore state
		cmdb.setDepthWrite(true);
		cmdb.setBlendFactors(0, BlendFactor::kOne, BlendFactor::kZero);
	}

	// Do lens flares
	getRenderer().getLensFlare().runDrawFlares(ctx, cmdb);
}

void ForwardShading::setDependencies(GraphicsRenderPassDescription& pass)
{
	pass.newTextureDependency(getRenderer().getDepthDownscale().getRt(), TextureUsageBit::kSampledFragment,
							  DepthDownscale::kQuarterInternalResolution);
	pass.newTextureDependency(getRenderer().getVolumetricLightingAccumulation().getRt(), TextureUsageBit::kSampledFragment);

	if(getRenderer().getLensFlare().getIndirectDrawBuffer().isValid())
	{
		pass.newBufferDependency(getRenderer().getLensFlare().getIndirectDrawBuffer(), BufferUsageBit::kIndirectDraw);
	}

	if(m_runCtx.m_visOut.containsDrawcalls())
	{
		pass.newBufferDependency((m_runCtx.m_meshVisOut.isFilled()) ? m_runCtx.m_meshVisOut.m_dependency : m_runCtx.m_visOut.m_dependency,
								 BufferUsageBit::kIndirectDraw);
	}
}

} // end namespace anki
