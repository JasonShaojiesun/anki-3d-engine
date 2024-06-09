// Copyright (C) 2009-present, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

// This file contains common code for all shaders. It's optional but it's recomended to include it

#pragma once

#if defined(__INTELLISENSE__)
#	include <AnKi/Shaders/Intellisense.hlsl>
#else
#	include <AnKi/Shaders/Include/Common.h>
#endif

#if ANKI_GR_BACKEND_VULKAN
#	define ANKI_PUSH_CONSTANTS(type, var) [[vk::push_constant]] ConstantBuffer<type> var;
#else
#	define ANKI_PUSH_CONSTANTS(type, var) ConstantBuffer<type> var : register(b0, space3000);
#endif

#if ANKI_GR_BACKEND_VULKAN
#	define ANKI_BINDLESS(texType, compType) \
		[[vk::binding(0, 1000000)]] Texture##texType<compType> g_bindlessTextures##texType##compType[]; \
		Texture##texType<compType> getBindlessTexture##texType##compType(U32 idx) \
		{ \
			return g_bindlessTextures##texType##compType[idx]; \
		} \
		Texture##texType<compType> getBindlessTextureNonUniformIndex##texType##compType(U32 idx) \
		{ \
			return g_bindlessTextures##texType##compType[NonUniformResourceIndex(idx)]; \
		}
#else
#	define ANKI_BINDLESS(texType, compType) \
		Texture##texType<compType> getBindlessTexture##texType##compType(U32 idx) \
		{ \
			Texture##texType<compType> tex = ResourceDescriptorHeap[idx]; \
			return tex; \
		} \
		Texture##texType<compType> getBindlessTextureNonUniformIndex##texType##compType(U32 idx) \
		{ \
			Texture##texType<compType> tex = ResourceDescriptorHeap[NonUniformResourceIndex(idx)]; \
			return tex; \
		}
#endif

#if ANKI_SUPPORTS_16BIT_TYPES
#	define ANKI_BINDLESS2(texType) \
		ANKI_BINDLESS(texType, UVec4) \
		ANKI_BINDLESS(texType, IVec4) \
		ANKI_BINDLESS(texType, Vec4)

#	define ANKI_BINDLESS3() \
		ANKI_BINDLESS2(2D) \
		ANKI_BINDLESS2(Cube) \
		ANKI_BINDLESS2(2DArray) \
		ANKI_BINDLESS2(3D)
#else
#	define ANKI_BINDLESS2(texType) \
		ANKI_BINDLESS(texType, UVec4) \
		ANKI_BINDLESS(texType, IVec4) \
		ANKI_BINDLESS(texType, Vec4) \
		ANKI_BINDLESS(texType, RVec4)

#	define ANKI_BINDLESS3() \
		ANKI_BINDLESS2(2D) \
		ANKI_BINDLESS2(Cube) \
		ANKI_BINDLESS2(2DArray) \
		ANKI_BINDLESS2(3D)
#endif

ANKI_BINDLESS3()

template<typename T>
T uvToNdc(T x)
{
	return x * 2.0f - 1.0f;
}

template<typename T>
T ndcToUv(T x)
{
	return x * 0.5f + 0.5f;
}

// Define min3, max3, min4, max4 functions

#define DEFINE_COMPARISON(func, scalarType, vectorType) \
	scalarType func##4(vectorType##4 v) \
	{ \
		return func(v.x, func(v.y, func(v.z, v.w))); \
	} \
	scalarType func##4(scalarType x, scalarType y, scalarType z, scalarType w) \
	{ \
		return func(x, func(y, func(z, w))); \
	} \
	scalarType func##3(vectorType##3 v) \
	{ \
		return func(v.x, func(v.y, v.z)); \
	} \
	scalarType func##3(scalarType x, scalarType y, scalarType z) \
	{ \
		return func(x, func(y, z)); \
	}

#define DEFINE_COMPARISON2(func) \
	DEFINE_COMPARISON(func, F32, Vec) \
	DEFINE_COMPARISON(func, I32, IVec) \
	DEFINE_COMPARISON(func, U32, UVec)

DEFINE_COMPARISON2(min)
DEFINE_COMPARISON2(max)

#undef DEFINE_COMPARISON2
#undef DEFINE_COMPARISON
