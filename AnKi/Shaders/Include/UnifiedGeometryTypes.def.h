// Copyright (C) 2009-present, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

// File includes the defines of the various texel buffer formats of the unified geometry buffer

#if !defined(ANKI_UNIFIED_GEOM_FORMAT_SEPERATOR)
#	define ANKI_UNIFIED_GEOM_FORMAT_SEPERATOR
#endif

ANKI_UNIFIED_GEOM_FORMAT(R32_Sfloat, F32, t14)
ANKI_UNIFIED_GEOM_FORMAT_SEPERATOR
ANKI_UNIFIED_GEOM_FORMAT(R32G32_Sfloat, Vec2, t15)
ANKI_UNIFIED_GEOM_FORMAT_SEPERATOR
ANKI_UNIFIED_GEOM_FORMAT(R32G32B32_Sfloat, Vec3, t16)
ANKI_UNIFIED_GEOM_FORMAT_SEPERATOR
ANKI_UNIFIED_GEOM_FORMAT(R32G32B32A32_Sfloat, Vec4, t17)
ANKI_UNIFIED_GEOM_FORMAT_SEPERATOR
ANKI_UNIFIED_GEOM_FORMAT(R16G16B16A16_Unorm, Vec4, t18)
ANKI_UNIFIED_GEOM_FORMAT_SEPERATOR
ANKI_UNIFIED_GEOM_FORMAT(R8G8B8A8_Snorm, Vec4, t19)
ANKI_UNIFIED_GEOM_FORMAT_SEPERATOR
ANKI_UNIFIED_GEOM_FORMAT(R8G8B8A8_Uint, UVec4, t20)
ANKI_UNIFIED_GEOM_FORMAT_SEPERATOR
ANKI_UNIFIED_GEOM_FORMAT(R16_Uint, U32, t21)
ANKI_UNIFIED_GEOM_FORMAT_SEPERATOR
ANKI_UNIFIED_GEOM_FORMAT(R8_Uint, U32, t22)

#undef ANKI_UNIFIED_GEOM_FORMAT
#undef ANKI_UNIFIED_GEOM_FORMAT_SEPERATOR