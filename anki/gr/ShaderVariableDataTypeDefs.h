// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

// ShaderVariableDataType defines

// ANKI_SVDT_MACRO(capital, varType, baseType, rowCount, columnCount)
#if defined(ANKI_SVDT_MACRO)
ANKI_SVDT_MACRO(I8, I8, I8, 1, 1)
ANKI_SVDT_MACRO(U8, U8, U8, 1, 1)
ANKI_SVDT_MACRO(I16, I16, I16, 1, 1)
ANKI_SVDT_MACRO(U16, U16, U16, 1, 1)
ANKI_SVDT_MACRO(I32, I32, I32, 1, 1)
ANKI_SVDT_MACRO(U32, U32, U32, 1, 1)
ANKI_SVDT_MACRO(I64, I64, I64, 1, 1)
ANKI_SVDT_MACRO(U64, U64, U64, 1, 1)
ANKI_SVDT_MACRO(F16, F16, F16, 1, 1)
ANKI_SVDT_MACRO(F32, F32, F32, 1, 1)

ANKI_SVDT_MACRO(I8VEC2, I8Vec2, I8, 2, 1)
ANKI_SVDT_MACRO(U8VEC2, U8Vec2, U8, 2, 1)
ANKI_SVDT_MACRO(I16VEC2, I16Vec2, I16, 2, 1)
ANKI_SVDT_MACRO(U16VEC2, U16Vec2, U16, 2, 1)
ANKI_SVDT_MACRO(IVEC2, IVec2, I32, 2, 1)
ANKI_SVDT_MACRO(UVEC2, UVec2, U32, 2, 1)
ANKI_SVDT_MACRO(HVEC2, HVec2, F16, 2, 1)
ANKI_SVDT_MACRO(VEC2, Vec2, F32, 2, 1)

ANKI_SVDT_MACRO(I8VEC3, I8Vec3, I8, 3, 1)
ANKI_SVDT_MACRO(U8VEC3, U8Vec3, U8, 3, 1)
ANKI_SVDT_MACRO(I16VEC3, I16Vec3, I16, 3, 1)
ANKI_SVDT_MACRO(U16VEC3, U16Vec3, U16, 3, 1)
ANKI_SVDT_MACRO(IVEC3, IVec3, I32, 3, 1)
ANKI_SVDT_MACRO(UVEC3, UVec3, U32, 3, 1)
ANKI_SVDT_MACRO(HVEC3, HVec3, F16, 3, 1)
ANKI_SVDT_MACRO(VEC3, Vec3, F32, 3, 1)

ANKI_SVDT_MACRO(I8VEC4, I8Vec4, I8, 4, 1)
ANKI_SVDT_MACRO(U8VEC4, U8Vec4, U8, 4, 1)
ANKI_SVDT_MACRO(I16VEC4, I16Vec4, I16, 4, 1)
ANKI_SVDT_MACRO(U16VEC4, U16Vec4, U16, 4, 1)
ANKI_SVDT_MACRO(IVEC4, IVec4, I32, 4, 1)
ANKI_SVDT_MACRO(UVEC4, UVec4, U32, 4, 1)
ANKI_SVDT_MACRO(HVEC4, HVec4, F16, 4, 1)
ANKI_SVDT_MACRO(VEC4, Vec4, F32, 4, 1)

ANKI_SVDT_MACRO(MAT3, Mat3, F32, 3, 3)
ANKI_SVDT_MACRO(MAT3X4, Mat3x4, F32, 3, 4)
ANKI_SVDT_MACRO(MAT4, Mat4, F32, 4, 4)
#endif

#if defined(ANKI_SVDT_MACRO_OPAQUE)
ANKI_SVDT_MACRO_OPAQUE(TEXTURE_1D, texture1D)
ANKI_SVDT_MACRO_OPAQUE(TEXTURE_1D_ARRAY, texture1DArray)
ANKI_SVDT_MACRO_OPAQUE(TEXTURE_2D, texture2D)
ANKI_SVDT_MACRO_OPAQUE(TEXTURE_2D_ARRAY, texture2DArray)
ANKI_SVDT_MACRO_OPAQUE(TEXTURE_3D, texture3D)
ANKI_SVDT_MACRO_OPAQUE(TEXTURE_CUBE, textureCube)
ANKI_SVDT_MACRO_OPAQUE(TEXTURE_CUBE_ARRAY, textureCubeArray)

ANKI_SVDT_MACRO_OPAQUE(IMAGE_1D, image1D)
ANKI_SVDT_MACRO_OPAQUE(IMAGE_1D_ARRAY, image1DArray)
ANKI_SVDT_MACRO_OPAQUE(IMAGE_2D, image2D)
ANKI_SVDT_MACRO_OPAQUE(IMAGE_2D_ARRAY, image2DArray)
ANKI_SVDT_MACRO_OPAQUE(IMAGE_3D, image3D)
ANKI_SVDT_MACRO_OPAQUE(IMAGE_CUBE, imageCube)
ANKI_SVDT_MACRO_OPAQUE(IMAGE_CUBE_ARRAY, imageCubeArray)

ANKI_SVDT_MACRO_OPAQUE(SAMPLER, sampler)
#endif
