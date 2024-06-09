// Copyright (C) 2009-present, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

// ANKI_RENDERER_OBJECT_DEF(Name, name, initCondition)

ANKI_RENDERER_OBJECT_DEF(GBuffer, gbuffer, 1)
ANKI_RENDERER_OBJECT_DEF(GBufferPost, gbufferPost, 1)
ANKI_RENDERER_OBJECT_DEF(ShadowMapping, shadowMapping, 1)
ANKI_RENDERER_OBJECT_DEF(LightShading, lightShading, 1)
ANKI_RENDERER_OBJECT_DEF(ForwardShading, forwardShading, 1)
ANKI_RENDERER_OBJECT_DEF(LensFlare, lensFlare, 1)
ANKI_RENDERER_OBJECT_DEF(DownscaleBlur, downscaleBlur, 1)
ANKI_RENDERER_OBJECT_DEF(Tonemapping, tonemapping, 1)
ANKI_RENDERER_OBJECT_DEF(Bloom, bloom, 1)
ANKI_RENDERER_OBJECT_DEF(FinalComposite, finalComposite, 1)
ANKI_RENDERER_OBJECT_DEF(Dbg, dbg, 1)
ANKI_RENDERER_OBJECT_DEF(ProbeReflections, probeReflections, 1)
ANKI_RENDERER_OBJECT_DEF(VolumetricFog, volumetricFog, 1)
ANKI_RENDERER_OBJECT_DEF(DepthDownscale, depthDownscale, 1)
ANKI_RENDERER_OBJECT_DEF(TemporalAA, temporalAA, 1)
ANKI_RENDERER_OBJECT_DEF(UiStage, uiStage, 1)
ANKI_RENDERER_OBJECT_DEF(VolumetricLightingAccumulation, volumetricLightingAccumulation, 1)
ANKI_RENDERER_OBJECT_DEF(IndirectDiffuseProbes, indirectDiffuseProbes, 1)
ANKI_RENDERER_OBJECT_DEF(ShadowmapsResolve, shadowmapsResolve, 1)
ANKI_RENDERER_OBJECT_DEF(RtShadows, rtShadows, GrManager::getSingleton().getDeviceCapabilities().m_rayTracingEnabled&& g_rayTracedShadowsCVar.get())
ANKI_RENDERER_OBJECT_DEF(AccelerationStructureBuilder, accelerationStructureBuilder,
						 GrManager::getSingleton().getDeviceCapabilities().m_rayTracingEnabled&& g_rayTracedShadowsCVar.get())
ANKI_RENDERER_OBJECT_DEF(MotionVectors, motionVectors, 1)
ANKI_RENDERER_OBJECT_DEF(Scale, scale, 1)
ANKI_RENDERER_OBJECT_DEF(VrsSriGeneration, vrsSriGeneration, 1)
ANKI_RENDERER_OBJECT_DEF(PrimaryNonRenderableVisibility, primaryNonRenderableVisibility, 1)
ANKI_RENDERER_OBJECT_DEF(ClusterBinning, clusterBinning2, 1)
ANKI_RENDERER_OBJECT_DEF(Ssao, ssao, 1)
ANKI_RENDERER_OBJECT_DEF(Ssr, ssr, 1)
ANKI_RENDERER_OBJECT_DEF(Sky, sky, 1)

#undef ANKI_RENDERER_OBJECT_DEF
