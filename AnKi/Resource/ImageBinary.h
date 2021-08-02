// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

// WARNING: This file is auto generated.

#pragma once

#include <AnKi/Resource/Common.h>

namespace anki
{

/// @addtogroup resource
/// @{

static constexpr const char* IMAGE_MAGIC = "ANKITEX1";

/// Image type.
/// @memberof ImageBinaryHeader
enum class ImageBinaryType : U32
{
	NONE,
	_2D,
	CUBE,
	_3D,
	_2D_ARRAY
};

/// The acceptable color types.
/// @memberof ImageBinaryHeader
enum class ImageBinaryColorFormat : U32
{
	NONE,
	RGB8,
	RGBA8,
	SRGB8,
};

/// The available data compressions.
/// @memberof ImageBinaryHeader
enum class ImageBinaryDataCompression : U32
{
	NONE,
	RAW = 1 << 0,
	S3TC = 1 << 1,
	ETC = 1 << 2
};
ANKI_ENUM_ALLOW_NUMERIC_OPERATIONS(ImageBinaryDataCompression)

/// The 1st things that appears in a image binary.
class ImageBinaryHeader
{
public:
	Array<U8, 8> m_magic;
	U32 m_width;
	U32 m_height;
	U32 m_depthOrLayerCount;
	ImageBinaryType m_type;
	ImageBinaryColorFormat m_colorFormat;
	ImageBinaryDataCompression m_compressionMask;
	U32 m_isNormal;
	U32 m_mipmapCount;
	Array<U8, 88> m_padding;

	template<typename TSerializer, typename TClass>
	static void serializeCommon(TSerializer& s, TClass self)
	{
		s.doArray("m_magic", offsetof(ImageBinaryHeader, m_magic), &self.m_magic[0], self.m_magic.getSize());
		s.doValue("m_width", offsetof(ImageBinaryHeader, m_width), self.m_width);
		s.doValue("m_height", offsetof(ImageBinaryHeader, m_height), self.m_height);
		s.doValue("m_depthOrLayerCount", offsetof(ImageBinaryHeader, m_depthOrLayerCount), self.m_depthOrLayerCount);
		s.doValue("m_type", offsetof(ImageBinaryHeader, m_type), self.m_type);
		s.doValue("m_colorFormat", offsetof(ImageBinaryHeader, m_colorFormat), self.m_colorFormat);
		s.doValue("m_compressionMask", offsetof(ImageBinaryHeader, m_compressionMask), self.m_compressionMask);
		s.doValue("m_isNormal", offsetof(ImageBinaryHeader, m_isNormal), self.m_isNormal);
		s.doValue("m_mipmapCount", offsetof(ImageBinaryHeader, m_mipmapCount), self.m_mipmapCount);
		s.doArray("m_padding", offsetof(ImageBinaryHeader, m_padding), &self.m_padding[0], self.m_padding.getSize());
	}

	template<typename TDeserializer>
	void deserialize(TDeserializer& deserializer)
	{
		serializeCommon<TDeserializer, ImageBinaryHeader&>(deserializer, *this);
	}

	template<typename TSerializer>
	void serialize(TSerializer& serializer) const
	{
		serializeCommon<TSerializer, const ImageBinaryHeader&>(serializer, *this);
	}
};

/// @}

} // end namespace anki