// Copyright (C) 2009-2020, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <anki/resource/Common.h>
#include <anki/gr/ShaderProgram.h>
#include <anki/util/HashMap.h>
#include <anki/shader_compiler/ShaderProgramBinary.h>

namespace anki
{

/// @addtogroup resource
/// @{

/// This is a ray tracing library. Essentially a shader program with some functionality on how to get group indices.
class ShaderProgramRaytracingLibrary
{
	friend class ShaderProgramResourceSystem;

public:
	CString getLibraryName() const
	{
		return m_libraryName;
	}

	U32 getRayTypeCount() const
	{
		ANKI_ASSERT(m_rayTypeCount < MAX_U32);
		return m_rayTypeCount;
	}

	const ShaderProgramPtr& getShaderProgram() const
	{
		return m_program;
	}

	/// Given the filename of a program (that contains hit shaders) and a specific mutation get the group handle.
	void getHitShaderGroupHandle(CString resourceFilename, ConstWeakArray<MutatorValue> mutation,
								 WeakArray<U8>& handle) const
	{
		const U32 hitGroupIndex = getHitGroupIndex(generateHitGroupHash(resourceFilename, mutation));
		getShaderGroupHandle(hitGroupIndex, handle);
	}

	void getMissShaderGroupHandle(U32 rayType, WeakArray<U8>& handle) const
	{
		ANKI_ASSERT(rayType < getRayTypeCount());
		getShaderGroupHandle(rayType + 1, handle);
	}

private:
	String m_libraryName;
	U32 m_rayTypeCount = MAX_U32;
	ShaderProgramPtr m_program;
	HashMap<U64, U32> m_groupHashToGroupIndex;

	/// Given the filename of a program (that contains hit shaders) and a specific mutation get a hash back.
	static U64 generateHitGroupHash(CString resourceFilename, ConstWeakArray<MutatorValue> mutation)
	{
		ANKI_ASSERT(resourceFilename.getLength() > 0);
		U64 hash = computeHash(resourceFilename.cstr(), resourceFilename.getLength());
		hash = appendHash(mutation.getBegin(), mutation.getSizeInBytes(), hash);
		return hash;
	}

	/// The hash generated by generateHitGroupHash() can be used to retrieve the hit group position in the m_program.
	U32 getHitGroupIndex(U64 groupHash) const
	{
		auto it = m_groupHashToGroupIndex.find(groupHash);
		ANKI_ASSERT(it != m_groupHashToGroupIndex.getEnd());
		return *it;
	}

	void getShaderGroupHandle(U32 groupIndex, WeakArray<U8>& handle) const;
};

/// A system that does some work on shader programs before resources start loading.
class ShaderProgramResourceSystem
{
public:
	ShaderProgramResourceSystem(CString cacheDir, GrManager* gr, ResourceFilesystem* fs,
								const GenericMemoryPoolAllocator<U8>& alloc)
		: m_alloc(alloc)
		, m_gr(gr)
		, m_fs(fs)
	{
		m_cacheDir.create(alloc, cacheDir);
	}

	~ShaderProgramResourceSystem();

	ANKI_USE_RESULT Error init();

	ConstWeakArray<ShaderProgramRaytracingLibrary> getRayTracingLibraries() const
	{
		return m_rtLibraries;
	}

private:
	GenericMemoryPoolAllocator<U8> m_alloc;
	String m_cacheDir;
	GrManager* m_gr;
	ResourceFilesystem* m_fs;
	DynamicArray<ShaderProgramRaytracingLibrary> m_rtLibraries;

	/// Iterate all programs in the filesystem and compile them to AnKi's binary format.
	static Error compileAllShaders(CString cacheDir, GrManager& gr, ResourceFilesystem& fs,
								   GenericMemoryPoolAllocator<U8>& alloc);

	static Error createRayTracingPrograms(CString cacheDir, GrManager& gr, ResourceFilesystem& fs,
										  GenericMemoryPoolAllocator<U8>& alloc,
										  DynamicArray<ShaderProgramRaytracingLibrary>& libs);
};
/// @}

} // end namespace anki