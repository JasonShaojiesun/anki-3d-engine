// Copyright (C) 2009-present, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

// Tonemapping resources

#pragma once

#include <AnKi/Shaders/Common.hlsl>

RWTexture2D<RVec4> g_tonemappingStorageTex : register(TONEMAPPING_REGISTER);

void writeExposureAndAverageLuminance(RF32 exposure, RF32 avgLuminance)
{
	g_tonemappingStorageTex[UVec2(0, 0)] = Vec4(exposure, avgLuminance, 0.0f, 0.0f);
}

RVec2 readExposureAndAverageLuminance()
{
	return g_tonemappingStorageTex[UVec2(0, 0)].xy;
}
