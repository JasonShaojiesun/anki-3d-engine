// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Importer/GltfImporter.h>
#include <AnKi/Importer/ImageImporter.h>
#include <AnKi/Resource/ImageLoader.h>
#include <AnKi/Util/WeakArray.h>
#include <AnKi/Util/Xml.h>
#include <AnKi/Util/Filesystem.h>

namespace anki {

inline constexpr const Char* kMaterialTemplate = R"(<!-- This file is auto generated by ImporterMaterial.cpp -->
<material shadows="1">
	<shaderPrograms>
		<shaderProgram name="GBufferGeneric">
			<mutation>
				<mutator name="DIFFUSE_TEX" value="%diffTexMutator%"/>
				<mutator name="SPECULAR_TEX" value="%specTexMutator%"/>
				<mutator name="ROUGHNESS_TEX" value="%roughnessTexMutator%"/>
				<mutator name="METAL_TEX" value="%metalTexMutator%"/>
				<mutator name="NORMAL_TEX" value="%normalTexMutator%"/>
				<mutator name="PARALLAX" value="%parallaxMutator%"/>
				<mutator name="EMISSIVE_TEX" value="%emissiveTexMutator%"/>
				<mutator name="ALPHA_TEST" value="%alphaTestMutator%"/>
			</mutation>
		</shaderProgram>
		%rayTracing%
	</shaderPrograms>

	<inputs>
		%parallaxInput%
		%diff%
		%spec%
		%roughness%
		%metallic%
		%normal%
		%emission%
		%subsurface%
		%height%
	</inputs>
</material>
)";

inline constexpr const Char* kRtMaterialTemplate = R"(
		<shaderProgram name="RtShadowsHit">
			<mutation>
				<mutator name="ALPHA_TEXTURE" value="%rtAlphaTestMutator%"/>
			</mutation>
		</shaderProgram>
)";

static CString getTextureUri(const cgltf_texture_view& view)
{
	ANKI_ASSERT(view.texture);
	ANKI_ASSERT(view.texture->image);
	ANKI_ASSERT(view.texture->image->uri);
	return view.texture->image->uri;
}

/// Read the texture and find out if
static Error findConstantColorsInImage(CString fname, Vec4& constantColor, BaseMemoryPool& pool)
{
	ImageLoader iloader(&pool);
	ANKI_CHECK(iloader.load(fname));
	ANKI_ASSERT(iloader.getColorFormat() == ImageBinaryColorFormat::kRgba8);
	ANKI_ASSERT(iloader.getCompression() == ImageBinaryDataCompression::kRaw);

	const U8Vec4* data = reinterpret_cast<const U8Vec4*>(&iloader.getSurface(0, 0, 0).m_data[0]);
	ConstWeakArray<U8Vec4> pixels(data, iloader.getWidth() * iloader.getHeight());

	const F32 epsilon = 1.0f / 255.0f;
	for(U32 y = 0; y < iloader.getHeight(); ++y)
	{
		for(U32 x = 0; x < iloader.getWidth(); ++x)
		{
			const U8Vec4& pixel = pixels[y * iloader.getWidth() + x];
			const Vec4 pixelf = Vec4(pixel) / 255.0f;

			if(x == 0 && y == 0)
			{
				// Initialize
				constantColor = pixelf;
			}
			else
			{
				for(U32 i = 0; i < 4; ++i)
				{
					if(absolute(pixelf[i] - constantColor[i]) > epsilon)
					{
						constantColor[i] = -1.0f;
					}
				}
			}
		}
	}

	return Error::kNone;
}

static Error importImage(BaseMemoryPool& pool, CString in, CString out, Bool alpha)
{
	ImageImporterConfig config;

	config.m_pool = &pool;

	Array<CString, 1> inputFnames = {in};
	config.m_inputFilenames = inputFnames;

	config.m_outFilename = out;
	config.m_compressions = ImageBinaryDataCompression::kS3tc | ImageBinaryDataCompression::kAstc;
	config.m_minMipmapDimension = 8;
	config.m_noAlpha = !alpha;

	String tmp;
	if(getTempDirectory(tmp))
	{
		ANKI_IMPORTER_LOGE("getTempDirectory() failed");
		return 1;
	}
	config.m_tempDirectory = tmp;

#if ANKI_OS_WINDOWS
	config.m_compressonatorFilename = ANKI_SOURCE_DIRECTORY "/ThirdParty/Bin/Windows64/Compressonator/compressonatorcli.exe";
	config.m_astcencFilename = ANKI_SOURCE_DIRECTORY "/ThirdParty/Bin/Windows64/astcenc-avx2.exe";
#elif ANKI_OS_LINUX
	config.m_compressonatorFilename = ANKI_SOURCE_DIRECTORY "/ThirdParty/Bin/Linux64/Compressonator/compressonatorcli";
	config.m_astcencFilename = ANKI_SOURCE_DIRECTORY "/ThirdParty/Bin/Linux64/astcenc-avx2";
#else
#	error "Unupported"
#endif

	config.m_flipImage = false;

	ANKI_IMPORTER_LOGV("Importing image \"%s\" as \"%s\"", in.cstr(), out.cstr());
	ANKI_CHECK(importImage(config));

	return Error::kNone;
}

static void fixImageUri(ImporterString& uri)
{
	uri.replaceAll(".tga", ".ankitex");
	uri.replaceAll(".png", ".ankitex");
	uri.replaceAll(".jpg", ".ankitex");
	uri.replaceAll(".jpeg", ".ankitex");
}

Error GltfImporter::writeMaterial(const cgltf_material& mtl, Bool writeRayTracing) const
{
	ImporterString fname(m_pool);
	fname.sprintf("%s%s", m_outDir.cstr(), computeMaterialResourceFilename(mtl).cstr());
	ANKI_IMPORTER_LOGV("Importing material %s", fname.cstr());

	if(!mtl.has_pbr_metallic_roughness)
	{
		ANKI_IMPORTER_LOGE("Expecting PBR metallic roughness");
		return Error::kUserData;
	}

	ImporterHashMap<CString, ImporterString> extras(m_pool);
	ANKI_CHECK(getExtras(mtl.extras, extras));

	ImporterString xml(m_pool);
	xml += XmlDocument<MemoryPoolPtrWrapper<BaseMemoryPool>>::kXmlHeader;
	xml += "\n";
	xml += kMaterialTemplate;

	if(writeRayTracing)
	{
		xml.replaceAll("%rayTracing%", kRtMaterialTemplate);
	}

	// Diffuse
	Bool alphaTested = false;
	if(mtl.pbr_metallic_roughness.base_color_texture.texture)
	{
		const CString fname = getTextureUri(mtl.pbr_metallic_roughness.base_color_texture);

		ImporterString uri(m_pool);
		uri.sprintf("%s%s", m_texrpath.cstr(), fname.cstr());

		xml.replaceAll("%diff%", ImporterString(m_pool).sprintf("<input name=\"m_diffTex\" value=\"%s\"/>", uri.cstr()));
		xml.replaceAll("%diffTexMutator%", "1");

		Vec4 constantColor;
		ANKI_CHECK(findConstantColorsInImage(fname, constantColor, *m_pool));

		const Bool constantAlpha = constantColor.w() >= 0.0f;
		xml.replaceAll("%alphaTestMutator%", (constantAlpha) ? "0" : "1");
		alphaTested = !constantAlpha;

		if(m_importTextures)
		{
			ImporterString out = m_outDir;
			out += fname;
			fixImageUri(out);
			ANKI_CHECK(importImage(*m_pool, fname, out, !constantAlpha));
		}
	}
	else
	{
		const F32* diffCol = &mtl.pbr_metallic_roughness.base_color_factor[0];

		xml.replaceAll("%diff%",
					   ImporterString(m_pool).sprintf("<input name=\"m_diffColor\" value=\"%f %f %f\"/>", diffCol[0], diffCol[1], diffCol[2]));

		xml.replaceAll("%diffTexMutator%", "0");
		xml.replaceAll("%alphaTestMutator%", "0");
	}

	xml.replaceAll("%rtAlphaTestMutator%", (alphaTested) ? "1" : "0");

	// Specular color (freshnel)
	{
		Vec3 specular;
		auto it = extras.find("specular");
		if(it != extras.getEnd())
		{
			ImporterStringList tokens(m_pool);
			tokens.splitString(it->toCString(), ' ');
			if(tokens.getSize() != 3)
			{
				ANKI_IMPORTER_LOGE("Wrong specular: %s", it->cstr());
				return Error::kUserData;
			}

			auto token = tokens.getBegin();
			ANKI_CHECK(token->toNumber(specular.x()));
			++token;
			ANKI_CHECK(token->toNumber(specular.y()));
			++token;
			ANKI_CHECK(token->toNumber(specular.z()));
		}
		else
		{
			specular = Vec3(0.04f);
		}

		xml.replaceAll("%spec%",
					   ImporterString(m_pool).sprintf("<input name=\"m_specColor\" value=\"%f %f %f\"/>", specular.x(), specular.y(), specular.z()));

		xml.replaceAll("%specTexMutator%", "0");
	}

	// Identify metallic/roughness texture
	F32 constantMetaliness = -1.0f, constantRoughness = -1.0f;
	if(mtl.pbr_metallic_roughness.metallic_roughness_texture.texture)
	{
		const CString fname = getTextureUri(mtl.pbr_metallic_roughness.metallic_roughness_texture);

		Vec4 constantColor;
		ANKI_CHECK(findConstantColorsInImage(fname, constantColor, *m_pool));
		constantRoughness = constantColor.y();
		constantMetaliness = constantColor.z();
	}

	// Roughness
	Bool bRoughnessMetalicTexture = false;
	if(mtl.pbr_metallic_roughness.metallic_roughness_texture.texture && constantRoughness < 0.0f)
	{
		ImporterString uri(m_pool);
		uri.sprintf("%s%s", m_texrpath.cstr(), getTextureUri(mtl.pbr_metallic_roughness.metallic_roughness_texture).cstr());

		xml.replaceAll("%roughness%", ImporterString(m_pool).sprintf("<input name=\"m_roughnessTex\" value=\"%s\"/>", uri.cstr()));

		xml.replaceAll("%roughnessTexMutator%", "1");

		bRoughnessMetalicTexture = true;
	}
	else
	{
		const F32 roughness = (constantRoughness >= 0.0f) ? constantRoughness * mtl.pbr_metallic_roughness.roughness_factor
														  : mtl.pbr_metallic_roughness.roughness_factor;

		xml.replaceAll("%roughness%", ImporterString(m_pool).sprintf("<input name=\"m_roughness\" value=\"%f\"/>", roughness));

		xml.replaceAll("%roughnessTexMutator%", "0");
	}

	// Metallic
	if(mtl.pbr_metallic_roughness.metallic_roughness_texture.texture && constantMetaliness < 0.0f)
	{
		ImporterString uri(m_pool);
		uri.sprintf("%s%s", m_texrpath.cstr(), getTextureUri(mtl.pbr_metallic_roughness.metallic_roughness_texture).cstr());

		xml.replaceAll("%metallic%", ImporterString(m_pool).sprintf("<input name=\"m_metallicTex\" value=\"%s\"/>", uri.cstr()));

		xml.replaceAll("%metalTexMutator%", "1");

		bRoughnessMetalicTexture = true;
	}
	else
	{
		const F32 metalines = (constantMetaliness >= 0.0f) ? constantMetaliness * mtl.pbr_metallic_roughness.metallic_factor
														   : mtl.pbr_metallic_roughness.metallic_factor;

		xml.replaceAll("%metallic%", ImporterString(m_pool).sprintf("<input name=\"m_metallic\" value=\"%f\"/>", metalines));

		xml.replaceAll("%metalTexMutator%", "0");
	}

	if(bRoughnessMetalicTexture && m_importTextures)
	{
		CString in = getTextureUri(mtl.pbr_metallic_roughness.metallic_roughness_texture);
		ImporterString out = m_outDir;
		out += in;
		fixImageUri(out);
		ANKI_CHECK(importImage(*m_pool, in, out, false));
	}

	// Normal texture
	if(mtl.normal_texture.texture)
	{
		Vec4 constantColor;
		ANKI_CHECK(findConstantColorsInImage(getTextureUri(mtl.normal_texture).cstr(), constantColor, *m_pool));
		if(constantColor.xyz() == -1.0f)
		{
			ImporterString uri(m_pool);
			uri.sprintf("%s%s", m_texrpath.cstr(), getTextureUri(mtl.normal_texture).cstr());

			xml.replaceAll("%normal%", ImporterString(m_pool).sprintf("<input name=\"m_normalTex\" value=\"%s\"/>", uri.cstr()));

			xml.replaceAll("%normalTexMutator%", "1");

			if(m_importTextures)
			{
				CString in = getTextureUri(mtl.normal_texture);
				ImporterString out = m_outDir;
				out += in;
				fixImageUri(out);
				ANKI_CHECK(importImage(*m_pool, in, out, false));
			}
		}
		else
		{
			xml.replaceAll("%normal%", "");
			xml.replaceAll("%normalTexMutator%", "0");
		}
	}
	else
	{
		xml.replaceAll("%normal%", "");
		xml.replaceAll("%normalTexMutator%", "0");
	}

	// Emissive texture
	if(mtl.emissive_texture.texture)
	{
		ImporterString uri(m_pool);
		uri.sprintf("%s%s", m_texrpath.cstr(), getTextureUri(mtl.emissive_texture).cstr());

		xml.replaceAll("%emission%", ImporterString(m_pool).sprintf("<input name=\"m_emissiveTex\" value=\"%s\"/>", uri.cstr()));

		xml.replaceAll("%emissiveTexMutator%", "1");

		if(m_importTextures)
		{
			CString in = getTextureUri(mtl.emissive_texture);
			ImporterString out = m_outDir;
			out += in;
			fixImageUri(out);
			ANKI_CHECK(importImage(*m_pool, in, out, false));
		}
	}
	else
	{
		const F32* emissionCol = &mtl.emissive_factor[0];

		xml.replaceAll("%emission%", ImporterString(m_pool).sprintf("<input name=\"m_emission\" value=\"%f %f %f\"/>", emissionCol[0], emissionCol[1],
																	emissionCol[2]));

		xml.replaceAll("%emissiveTexMutator%", "0");
	}

	// Subsurface
	{
		F32 subsurface;
		auto it = extras.find("subsurface");
		if(it != extras.getEnd())
		{
			ANKI_CHECK(it->toNumber(subsurface));
		}
		else
		{
			subsurface = 0.0f;
		}

		xml.replaceAll("%subsurface%", ImporterString(m_pool).sprintf("<input name=\"m_subsurface\" value=\"%f\"/>", subsurface));
	}

	// Height texture
	auto it = extras.find("height_map");
	if(it != extras.getEnd())
	{
		ImporterString uri(m_pool);
		uri.sprintf("%s%s", m_texrpath.cstr(), it->cstr());

		xml.replaceAll("%height%", ImporterString(m_pool).sprintf("<input name=\"m_heightTex\" value=\"%s\" \"/>\n"
																  "\t\t<input name=\"m_heightmapScale\" value=\"0.05\"/>",
																  uri.cstr()));

		xml.replaceAll("%parallaxMutator%", "1");
	}
	else
	{
		xml.replaceAll("%height%", "");
		xml.replaceAll("%parallaxInput%", "");
		xml.replaceAll("%parallaxMutator%", "0");
	}

	// Replace texture extensions with .anki
	fixImageUri(xml);

	// Write file
	File file;
	ANKI_CHECK(file.open(fname.toCString(), FileOpenFlag::kWrite));
	ANKI_CHECK(file.writeText(xml));

	return Error::kNone;
}

} // end namespace anki
