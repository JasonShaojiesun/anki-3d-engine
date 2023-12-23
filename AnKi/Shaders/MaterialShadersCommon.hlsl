// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

// Common stuff for GBuffer, Forward shading and the rest of shaders that appear in materials.

#pragma once

#include <AnKi/Shaders/Include/MaterialTypes.h>
#include <AnKi/Shaders/Include/MeshTypes.h>
#include <AnKi/Shaders/Include/GpuSceneFunctions.h>
#include <AnKi/Shaders/PackFunctions.hlsl>
#include <AnKi/Shaders/VisibilityAndCollisionFunctions.hlsl>

ANKI_BINDLESS_SET(MaterialSet::kBindless)

[[vk::binding(MaterialBinding::kTrilinearRepeatSampler, MaterialSet::kGlobal)]] SamplerState g_globalSampler;
[[vk::binding(MaterialBinding::kGlobalConstants, MaterialSet::kGlobal)]] ConstantBuffer<MaterialGlobalConstants> g_globalConstants;
[[vk::binding(MaterialBinding::kGpuScene, MaterialSet::kGlobal)]] ByteAddressBuffer g_gpuScene;

// Unified geom:
#define ANKI_UNIFIED_GEOM_FORMAT(fmt, shaderType) \
	[[vk::binding(MaterialBinding::kUnifiedGeometry_##fmt, MaterialSet::kGlobal)]] Buffer<shaderType> g_unifiedGeom_##fmt;
#include <AnKi/Shaders/Include/UnifiedGeometryTypes.defs.h>

[[vk::binding(MaterialBinding::kMeshletBoundingVolumes, MaterialSet::kGlobal)]] StructuredBuffer<MeshletBoundingVolume> g_meshletBoundingVolumes;
[[vk::binding(MaterialBinding::kMeshletGeometryDescriptors, MaterialSet::kGlobal)]] StructuredBuffer<MeshletGeometryDescriptor>
	g_meshletGeometryDescriptors;
[[vk::binding(MaterialBinding::kTaskShaderPayloads, MaterialSet::kGlobal)]] StructuredBuffer<GpuSceneTaskShaderPayload> g_taskShaderPayloads;
[[vk::binding(MaterialBinding::kRenderables, MaterialSet::kGlobal)]] StructuredBuffer<GpuSceneRenderable> g_renderables;
[[vk::binding(MaterialBinding::kHzbTexture, MaterialSet::kGlobal)]] Texture2D<Vec4> g_hzbTexture;
[[vk::binding(MaterialBinding::kNearestClampSampler, MaterialSet::kGlobal)]] SamplerState g_nearestClampSampler;

// FW shading specific
#if defined(FORWARD_SHADING)
#	include <AnKi/Shaders/ClusteredShadingFunctions.hlsl>

[[vk::binding(MaterialBinding::kLinearClampSampler, MaterialSet::kGlobal)]] SamplerState g_linearAnyClampSampler;
[[vk::binding(MaterialBinding::kDepthRt, MaterialSet::kGlobal)]] Texture2D g_gbufferDepthTex;
[[vk::binding(MaterialBinding::kLightVolume, MaterialSet::kGlobal)]] Texture3D<RVec4> g_lightVol;
[[vk::binding(MaterialBinding::kShadowSampler, MaterialSet::kGlobal)]] SamplerComparisonState g_shadowSampler;

[[vk::binding(MaterialBinding::kClusterShadingConstants, MaterialSet::kGlobal)]] ConstantBuffer<ClusteredShadingConstants> g_clusteredShading;
[[vk::binding(MaterialBinding::kClusters, MaterialSet::kGlobal)]] StructuredBuffer<Cluster> g_clusters;
[[vk::binding(MaterialBinding::kClusterShadingLights, MaterialSet::kGlobal)]] StructuredBuffer<PointLight> g_pointLights;
[[vk::binding(MaterialBinding::kClusterShadingLights, MaterialSet::kGlobal)]] StructuredBuffer<SpotLight> g_spotLights;
[[vk::binding((U32)MaterialBinding::kClusterShadingLights + 1u, MaterialSet::kGlobal)]] Texture2D<Vec4> g_shadowAtlasTex;
#endif

UnpackedMeshVertex loadVertex(GpuSceneMeshLod mlod, U32 svVertexId, Bool bones)
{
	UnpackedMeshVertex v;
	v.m_position = g_unifiedGeom_R16G16B16A16_Unorm[mlod.m_vertexOffsets[(U32)VertexStreamId::kPosition] + svVertexId];
	v.m_position = v.m_position * mlod.m_positionScale + mlod.m_positionTranslation;

	v.m_normal = g_unifiedGeom_R8G8B8A8_Snorm[mlod.m_vertexOffsets[(U32)VertexStreamId::kNormal] + svVertexId].xyz;
	v.m_uv = g_unifiedGeom_R32G32_Sfloat[mlod.m_vertexOffsets[(U32)VertexStreamId::kUv] + svVertexId];

	if(bones)
	{
		v.m_boneIndices = g_unifiedGeom_R8G8B8A8_Uint[mlod.m_vertexOffsets[(U32)VertexStreamId::kBoneIds] + svVertexId];
		v.m_boneWeights = g_unifiedGeom_R8G8B8A8_Snorm[mlod.m_vertexOffsets[(U32)VertexStreamId::kBoneWeights] + svVertexId];
	}

	return v;
}

UnpackedMeshVertex loadVertex(MeshletGeometryDescriptor meshlet, U32 vertexIndex, Bool bones, F32 positionScale, Vec3 positionTranslation)
{
	UnpackedMeshVertex v;
	v.m_position = g_unifiedGeom_R16G16B16A16_Unorm[meshlet.m_vertexOffsets[(U32)VertexStreamId::kPosition] + vertexIndex];
	v.m_position = v.m_position * positionScale + positionTranslation;

	v.m_normal = g_unifiedGeom_R8G8B8A8_Snorm[meshlet.m_vertexOffsets[(U32)VertexStreamId::kNormal] + vertexIndex].xyz;
	v.m_uv = g_unifiedGeom_R32G32_Sfloat[meshlet.m_vertexOffsets[(U32)VertexStreamId::kUv] + vertexIndex];

	if(bones)
	{
		v.m_boneIndices = g_unifiedGeom_R8G8B8A8_Uint[meshlet.m_vertexOffsets[(U32)VertexStreamId::kBoneIds] + vertexIndex];
		v.m_boneWeights = g_unifiedGeom_R8G8B8A8_Snorm[meshlet.m_vertexOffsets[(U32)VertexStreamId::kBoneWeights] + vertexIndex];
	}

	return v;
}

Bool cullBackfaceMeshlet(MeshletBoundingVolume meshlet, Mat3x4 worldTransform, Vec3 cameraWorldPos)
{
	const Vec4 coneData = unpackSnorm4x8(meshlet.m_coneDirection_R8G8B8_Snorm_cosHalfAngle_R8_Snorm);

	Vec3 center = (meshlet.m_aabbMin + meshlet.m_aabbMax) / 2.0f;

	center = mul(worldTransform, Vec4(center, 1.0f));
	const Vec3 coneAxisWspace = normalize(mul(worldTransform, Vec4(coneData.xyz, 0.0f)));

	// Extract uniform scale
	const Vec3 xAxis = Vec3(worldTransform.m_row0.x, worldTransform.m_row1.x, worldTransform.m_row2.x);
	const F32 uniformScale = length(xAxis);

	meshlet.m_sphereRadius *= uniformScale;

	const Vec3 dir = center - cameraWorldPos;
	return dot(dir, coneAxisWspace) >= coneData.w * length(dir) + meshlet.m_sphereRadius;
}
