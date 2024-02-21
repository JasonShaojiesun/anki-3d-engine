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
	<shaderProgram name="GBufferGeneric">
		<mutation>
			<mutator name="DIFFUSE_TEX" value="%diffTexMutator%"/>
			<mutator name="SPECULAR_TEX" value="%specTexMutator%"/>
			<mutator name="ROUGHNESS_METALNESS_TEX" value="%roughnessMetalnessTexMutator%"/>
			<mutator name="NORMAL_TEX" value="%normalTexMutator%"/>
			<mutator name="PARALLAX" value="%parallaxMutator%"/>
			<mutator name="EMISSIVE_TEX" value="%emissiveTexMutator%"/>
			<mutator name="ALPHA_TEST" value="%alphaTestMutator%"/>
		</mutation>
	</shaderProgram>

	<inputs>
		%parallaxInput%
		%diff%
		%spec%
		%roughnessMetalness%
		%normal%
		%emission%
		%subsurface%
		%height%
	</inputs>
</material>
)";

static ImporterString getTextureUri(const cgltf_texture_view& view)
{
	ANKI_ASSERT(view.texture);
	ANKI_ASSERT(view.texture->image);
	ANKI_ASSERT(view.texture->image->uri);
	ImporterString out = view.texture->image->uri;
	out.replaceAll("%20", " ");
	return out;
}

/// Read the texture and find out if it has constant color. If a component is not constant return -1
static Error findConstantColorsInImage(CString fname, Vec4& constantColor)
{
	ImageLoader iloader(&ImporterMemoryPool::getSingleton());
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

static Error importImage(CString in, CString out, Bool alpha)
{
	ImageImporterConfig config;

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
	const Error err = writeMaterialInternal(mtl, writeRayTracing);
	if(err)
	{
		ANKI_IMPORTER_LOGE("Failed to import material: %s", mtl.name);
	}

	return err;
}

Error GltfImporter::writeMaterialInternal(const cgltf_material& mtl, Bool writeRayTracing) const
{
	if(!writeRayTracing)
	{
		ANKI_IMPORTER_LOGW("Skipping ray tracing is ignored");
	}
	ImporterString fname;
	fname.sprintf("%s%s", m_outDir.cstr(), computeMaterialResourceFilename(mtl).cstr());
	ANKI_IMPORTER_LOGV("Importing material %s", fname.cstr());

	if(!mtl.has_pbr_metallic_roughness)
	{
		ANKI_IMPORTER_LOGE("Expecting PBR metallic roughness");
		return Error::kUserData;
	}

	ImporterHashMap<CString, ImporterString> extras;
	ANKI_CHECK(appendExtras(mtl.extras, extras));

	ImporterString xml;
	xml += XmlDocument<MemoryPoolPtrWrapper<BaseMemoryPool>>::kXmlHeader;
	xml += "\n";
	xml += kMaterialTemplate;

	// Diffuse
	if(mtl.pbr_metallic_roughness.base_color_texture.texture)
	{
		const ImporterString fname = getTextureUri(mtl.pbr_metallic_roughness.base_color_texture);

		ImporterString uri;
		uri.sprintf("%s%s", m_texrpath.cstr(), fname.cstr());

		const F32* diffCol = &mtl.pbr_metallic_roughness.base_color_factor[0];

		xml.replaceAll("%diff%", ImporterString().sprintf("<input name=\"m_diffuseTex\" value=\"%s\"/>\n"
														  "\t\t<input name=\"m_diffuseScale\" value=\"%f %f %f %f\"/>",
														  uri.cstr(), diffCol[0], diffCol[1], diffCol[2], diffCol[3]));
		xml.replaceAll("%diffTexMutator%", "1");

		Vec4 constantColor;
		ANKI_CHECK(findConstantColorsInImage(fname, constantColor));

		const Bool constantAlpha = constantColor.w() >= 0.0f;
		xml.replaceAll("%alphaTestMutator%", (constantAlpha) ? "0" : "1");

		if(m_importTextures)
		{
			ImporterString out = m_outDir;
			out += fname;
			fixImageUri(out);
			ANKI_CHECK(importImage(fname, out, !constantAlpha));
		}
	}
	else
	{
		const F32* diffCol = &mtl.pbr_metallic_roughness.base_color_factor[0];

		xml.replaceAll("%diff%", ImporterString().sprintf("<input name=\"m_diffuseScale\" value=\"%f %f %f %f\"/>", diffCol[0], diffCol[1],
														  diffCol[2], diffCol[3]));

		xml.replaceAll("%diffTexMutator%", "0");
		xml.replaceAll("%alphaTestMutator%", "0");
	}

	// Specular color (freshnel)
	{
		Vec3 specular;
		auto it = extras.find("specular");
		if(it != extras.getEnd())
		{
			ImporterStringList tokens;
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
					   ImporterString().sprintf("<input name=\"m_specularScale\" value=\"%f %f %f\"/>", specular.x(), specular.y(), specular.z()));

		xml.replaceAll("%specTexMutator%", "0");
	}

	// Identify metallic/roughness texture
	F32 constantMetaliness = -1.0f, constantRoughness = -1.0f;
	if(mtl.pbr_metallic_roughness.metallic_roughness_texture.texture)
	{
		const ImporterString fname = getTextureUri(mtl.pbr_metallic_roughness.metallic_roughness_texture);

		Vec4 constantColor;
		ANKI_CHECK(findConstantColorsInImage(fname, constantColor));
		constantRoughness = constantColor.y();
		constantMetaliness = constantColor.z();
	}

	// Roughness/metallic
	if(mtl.pbr_metallic_roughness.metallic_roughness_texture.texture && (constantRoughness < 0.0f || constantMetaliness < 0.0f))
	{
		ImporterString uri;
		uri.sprintf("%s%s", m_texrpath.cstr(), getTextureUri(mtl.pbr_metallic_roughness.metallic_roughness_texture).cstr());

		xml.replaceAll("%roughnessMetalness%",
					   ImporterString().sprintf("<input name=\"m_roughnessMetalnessTex\" value=\"%s\"/>\n"
												"\t\t<input name=\"m_roughnessScale\" value=\"%f\"/>\n"
												"\t\t<input name=\"m_metalnessScale\" value=\"%f\"/>",
												uri.cstr(), mtl.pbr_metallic_roughness.roughness_factor, mtl.pbr_metallic_roughness.metallic_factor));

		xml.replaceAll("%roughnessMetalnessTexMutator%", "1");

		if(m_importTextures)
		{
			const ImporterString in = getTextureUri(mtl.pbr_metallic_roughness.metallic_roughness_texture);
			ImporterString out = m_outDir;
			out += in;
			fixImageUri(out);
			ANKI_CHECK(importImage(in, out, false));
		}
	}
	else
	{
		// No texture or both roughness and metalness are constant

		const F32 roughness = (constantRoughness >= 0.0f) ? constantRoughness * mtl.pbr_metallic_roughness.roughness_factor
														  : mtl.pbr_metallic_roughness.roughness_factor;
		const F32 metalness = (constantMetaliness >= 0.0f) ? constantMetaliness * mtl.pbr_metallic_roughness.metallic_factor
														   : mtl.pbr_metallic_roughness.metallic_factor;

		xml.replaceAll("%roughnessMetalness%", ImporterString().sprintf("<input name=\"m_roughnessScale\" value=\"%f\"/>\n"
																		"\t\t<input name=\"m_metalnessScale\" value=\"%f\"/>",
																		roughness, metalness));

		xml.replaceAll("%roughnessMetalnessTexMutator%", "0");
	}

	// Normal texture
	if(mtl.normal_texture.texture)
	{
		Vec4 constantColor;
		ANKI_CHECK(findConstantColorsInImage(getTextureUri(mtl.normal_texture).cstr(), constantColor));
		if(constantColor.xyz() == -1.0f)
		{
			ImporterString uri;
			uri.sprintf("%s%s", m_texrpath.cstr(), getTextureUri(mtl.normal_texture).cstr());

			xml.replaceAll("%normal%", ImporterString().sprintf("<input name=\"m_normalTex\" value=\"%s\"/>", uri.cstr()));

			xml.replaceAll("%normalTexMutator%", "1");

			if(m_importTextures)
			{
				const ImporterString in = getTextureUri(mtl.normal_texture);
				ImporterString out = m_outDir;
				out += in;
				fixImageUri(out);
				ANKI_CHECK(importImage(in, out, false));
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
		ImporterString uri;
		uri.sprintf("%s%s", m_texrpath.cstr(), getTextureUri(mtl.emissive_texture).cstr());

		xml.replaceAll("%emission%", ImporterString().sprintf("<input name=\"m_emissiveTex\" value=\"%s\"/>\n"
															  "\t\t<input name=\"m_emissionScale\" value=\"%f %f %f\"/>",
															  uri.cstr(), mtl.emissive_factor[0], mtl.emissive_factor[1], mtl.emissive_factor[2]));

		xml.replaceAll("%emissiveTexMutator%", "1");

		if(m_importTextures)
		{
			const ImporterString in = getTextureUri(mtl.emissive_texture);
			ImporterString out = m_outDir;
			out += in;
			fixImageUri(out);
			ANKI_CHECK(importImage(in, out, false));
		}
	}
	else
	{
		xml.replaceAll("%emission%", ImporterString().sprintf("<input name=\"m_emissionScale\" value=\"%f %f %f\"/>", mtl.emissive_factor[0],
															  mtl.emissive_factor[1], mtl.emissive_factor[2]));

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

		xml.replaceAll("%subsurface%", ImporterString().sprintf("<input name=\"m_subsurface\" value=\"%f\"/>", subsurface));
	}

	// Height texture
	auto it = extras.find("height_map");
	if(it != extras.getEnd())
	{
		ImporterString uri;
		uri.sprintf("%s%s", m_texrpath.cstr(), it->cstr());

		xml.replaceAll("%height%", ImporterString().sprintf("<input name=\"m_heightTex\" value=\"%s\" \"/>\n"
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
