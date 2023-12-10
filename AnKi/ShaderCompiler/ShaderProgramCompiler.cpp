// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/ShaderCompiler/ShaderProgramCompiler.h>
#include <AnKi/ShaderCompiler/ShaderProgramParser.h>
#include <AnKi/ShaderCompiler/Dxc.h>
#include <AnKi/Util/Serializer.h>
#include <AnKi/Util/HashMap.h>

namespace anki {

void freeShaderProgramBinary(ShaderProgramBinary*& binary)
{
	if(binary == nullptr)
	{
		return;
	}

	BaseMemoryPool& mempool = ShaderCompilerMemoryPool::getSingleton();

	for(ShaderProgramBinaryCodeBlock& code : binary->m_codeBlocks)
	{
		mempool.free(code.m_binary.getBegin());
	}
	mempool.free(binary->m_codeBlocks.getBegin());

	for(ShaderProgramBinaryMutator& mutator : binary->m_mutators)
	{
		mempool.free(mutator.m_values.getBegin());
	}
	mempool.free(binary->m_mutators.getBegin());

	for(ShaderProgramBinaryMutation& m : binary->m_mutations)
	{
		mempool.free(m.m_values.getBegin());
	}
	mempool.free(binary->m_mutations.getBegin());

	for(ShaderProgramBinaryVariant& variant : binary->m_variants)
	{
		mempool.free(variant.m_techniqueCodeBlocks.getBegin());
	}
	mempool.free(binary->m_variants.getBegin());

	mempool.free(binary->m_techniques.getBegin());

	for(ShaderProgramBinaryStruct& s : binary->m_structs)
	{
		mempool.free(s.m_members.getBegin());
	}
	mempool.free(binary->m_structs.getBegin());

	mempool.free(binary);

	binary = nullptr;
}

/// Spin the dials. Used to compute all mutator combinations.
static Bool spinDials(ShaderCompilerDynamicArray<U32>& dials, ConstWeakArray<ShaderProgramParserMutator> mutators)
{
	ANKI_ASSERT(dials.getSize() == mutators.getSize() && dials.getSize() > 0);
	Bool done = true;

	U32 crntDial = dials.getSize() - 1;
	while(true)
	{
		// Turn dial
		++dials[crntDial];

		if(dials[crntDial] >= mutators[crntDial].m_values.getSize())
		{
			if(crntDial == 0)
			{
				// Reached the 1st dial, stop spinning
				done = true;
				break;
			}
			else
			{
				dials[crntDial] = 0;
				--crntDial;
			}
		}
		else
		{
			done = false;
			break;
		}
	}

	return done;
}

static void compileVariantAsync(const ShaderProgramParser& parser, ShaderProgramBinaryMutation& mutation,
								ShaderCompilerDynamicArray<ShaderProgramBinaryVariant>& variants,
								ShaderCompilerDynamicArray<ShaderProgramBinaryCodeBlock>& codeBlocks,
								ShaderCompilerDynamicArray<U64>& sourceCodeHashes, ShaderProgramAsyncTaskInterface& taskManager, Mutex& mtx,
								Atomic<I32>& error)
{
	class Ctx
	{
	public:
		const ShaderProgramParser* m_parser;
		ShaderProgramBinaryMutation* m_mutation;
		ShaderCompilerDynamicArray<ShaderProgramBinaryVariant>* m_variants;
		ShaderCompilerDynamicArray<ShaderProgramBinaryCodeBlock>* m_codeBlocks;
		ShaderCompilerDynamicArray<U64>* m_sourceCodeHashes;
		Mutex* m_mtx;
		Atomic<I32>* m_err;
	};

	Ctx* ctx = newInstance<Ctx>(ShaderCompilerMemoryPool::getSingleton());
	ctx->m_parser = &parser;
	ctx->m_mutation = &mutation;
	ctx->m_variants = &variants;
	ctx->m_codeBlocks = &codeBlocks;
	ctx->m_sourceCodeHashes = &sourceCodeHashes;
	ctx->m_mtx = &mtx;
	ctx->m_err = &error;

	auto callback = [](void* userData) {
		Ctx& ctx = *static_cast<Ctx*>(userData);
		class Cleanup
		{
		public:
			Ctx* m_ctx;

			~Cleanup()
			{
				deleteInstance(ShaderCompilerMemoryPool::getSingleton(), m_ctx);
			}
		} cleanup{&ctx};

		if(ctx.m_err->load() != 0)
		{
			// Don't bother
			return;
		}

		const U32 techniqueCount = ctx.m_parser->getTechniques().getSize();

		// Compile the sources
		ShaderCompilerDynamicArray<ShaderProgramBinaryTechniqueCodeBlocks> codeBlockIndices;
		codeBlockIndices.resize(techniqueCount);
		for(auto& it : codeBlockIndices)
		{
			it.m_codeBlockIndices.fill(kMaxU32);
		}

		ShaderCompilerString compilerErrorLog;
		Error err = Error::kNone;
		U newCodeBlockCount = 0;
		for(U32 t = 0; t < techniqueCount && !err; ++t)
		{
			const ShaderProgramParserTechnique& technique = ctx.m_parser->getTechniques()[t];
			for(ShaderType shaderType : EnumBitsIterable<ShaderType, ShaderTypeBit>(technique.m_shaderTypes))
			{
				ShaderCompilerString source;
				ctx.m_parser->generateVariant(ctx.m_mutation->m_values, technique, shaderType, source);

				// Check if the source code was found before
				const U64 sourceCodeHash = source.computeHash();

				if(technique.m_activeMutators[shaderType] != kMaxU64)
				{
					LockGuard lock(*ctx.m_mtx);

					ANKI_ASSERT(ctx.m_sourceCodeHashes->getSize() == ctx.m_codeBlocks->getSize());

					Bool found = false;
					for(U32 i = 0; i < ctx.m_sourceCodeHashes->getSize(); ++i)
					{
						if((*ctx.m_sourceCodeHashes)[i] == sourceCodeHash)
						{
							codeBlockIndices[t].m_codeBlockIndices[shaderType] = i;
							found = true;
							break;
						}
					}

					if(found)
					{
						continue;
					}
				}

				ShaderCompilerDynamicArray<U8> spirv;
				err = compileHlslToSpirv(source, shaderType, ctx.m_parser->compileWith16bitTypes(), spirv, compilerErrorLog);
				if(err)
				{
					break;
				}

				const U64 newHash = computeHash(spirv.getBegin(), spirv.getSizeInBytes());

				// Add the binary if not already there
				{
					LockGuard lock(*ctx.m_mtx);

					Bool found = false;
					for(U32 j = 0; j < ctx.m_codeBlocks->getSize(); ++j)
					{
						if((*ctx.m_codeBlocks)[j].m_hash == newHash)
						{
							codeBlockIndices[t].m_codeBlockIndices[shaderType] = j;
							found = true;
							break;
						}
					}

					if(!found)
					{
						codeBlockIndices[t].m_codeBlockIndices[shaderType] = ctx.m_codeBlocks->getSize();

						auto& codeBlock = *ctx.m_codeBlocks->emplaceBack();
						spirv.moveAndReset(codeBlock.m_binary);
						codeBlock.m_hash = newHash;

						ctx.m_sourceCodeHashes->emplaceBack(sourceCodeHash);
						ANKI_ASSERT(ctx.m_sourceCodeHashes->getSize() == ctx.m_codeBlocks->getSize());

						++newCodeBlockCount;
					}
				}
			}
		}

		if(err)
		{
			I32 expectedErr = 0;
			const Bool isFirstError = ctx.m_err->compareExchange(expectedErr, err._getCode());
			if(isFirstError)
			{
				ANKI_SHADER_COMPILER_LOGE("Shader compilation failed:\n%s", compilerErrorLog.cstr());
				return;
			}
			return;
		}

		// Do variant stuff
		{
			LockGuard lock(*ctx.m_mtx);

			Bool createVariant = true;
			if(newCodeBlockCount == 0)
			{
				// No new code blocks generated, search all variants to see if there is a duplicate

				for(U32 i = 0; i < ctx.m_variants->getSize(); ++i)
				{
					Bool same = true;
					for(U32 t = 0; t < techniqueCount; ++t)
					{
						const ShaderProgramBinaryTechniqueCodeBlocks& a = (*ctx.m_variants)[i].m_techniqueCodeBlocks[t];
						const ShaderProgramBinaryTechniqueCodeBlocks& b = codeBlockIndices[t];

						if(memcmp(&a, &b, sizeof(a)) != 0)
						{
							// Not the same
							same = false;
							break;
						}
					}

					if(same)
					{
						createVariant = false;
						ctx.m_mutation->m_variantIndex = i;
						break;
					}
				}
			}

			// Create a new variant
			if(createVariant)
			{
				ctx.m_mutation->m_variantIndex = ctx.m_variants->getSize();

				ShaderProgramBinaryVariant* variant = ctx.m_variants->emplaceBack();

				codeBlockIndices.moveAndReset(variant->m_techniqueCodeBlocks);
			}
		}
	};

	taskManager.enqueueTask(callback, ctx);
}

Error compileShaderProgramInternal(CString fname, ShaderProgramFilesystemInterface& fsystem, ShaderProgramPostParseInterface* postParseCallback,
								   ShaderProgramAsyncTaskInterface* taskManager_, const ShaderCompilerOptions& compilerOptions,
								   ShaderProgramBinary*& binary)
{
	ShaderCompilerMemoryPool& memPool = ShaderCompilerMemoryPool::getSingleton();

	// Initialize the binary
	binary = newInstance<ShaderProgramBinary>(memPool);
	memcpy(&binary->m_magic[0], kShaderBinaryMagic, 8);

	// Parse source
	ShaderProgramParser parser(fname, &fsystem, compilerOptions);
	ANKI_CHECK(parser.parse());

	if(postParseCallback && postParseCallback->skipCompilation(parser.getHash()))
	{
		return Error::kNone;
	}

	// Get mutators
	U32 mutationCount = 0;
	if(parser.getMutators().getSize() > 0)
	{
		newArray(memPool, parser.getMutators().getSize(), binary->m_mutators);

		for(U32 i = 0; i < binary->m_mutators.getSize(); ++i)
		{
			ShaderProgramBinaryMutator& out = binary->m_mutators[i];
			const ShaderProgramParserMutator& in = parser.getMutators()[i];

			zeroMemory(out);

			newArray(memPool, in.m_values.getSize(), out.m_values);
			memcpy(out.m_values.getBegin(), in.m_values.getBegin(), in.m_values.getSizeInBytes());

			memcpy(out.m_name.getBegin(), in.m_name.cstr(), in.m_name.getLength() + 1);

			// Update the count
			mutationCount = (i == 0) ? out.m_values.getSize() : mutationCount * out.m_values.getSize();
		}
	}
	else
	{
		ANKI_ASSERT(binary->m_mutators.getSize() == 0);
	}

	// Create all variants
	Mutex mtx;
	Atomic<I32> errorAtomic(0);
	class SyncronousShaderProgramAsyncTaskInterface : public ShaderProgramAsyncTaskInterface
	{
	public:
		void enqueueTask(void (*callback)(void* userData), void* userData) final
		{
			callback(userData);
		}

		Error joinTasks() final
		{
			// Nothing
			return Error::kNone;
		}
	} syncTaskManager;
	ShaderProgramAsyncTaskInterface& taskManager = (taskManager_) ? *taskManager_ : syncTaskManager;

	if(parser.getMutators().getSize() > 0)
	{
		// Initialize
		ShaderCompilerDynamicArray<MutatorValue> mutationValues;
		mutationValues.resize(parser.getMutators().getSize());
		ShaderCompilerDynamicArray<U32> dials;
		dials.resize(parser.getMutators().getSize(), 0);
		ShaderCompilerDynamicArray<ShaderProgramBinaryVariant> variants;
		ShaderCompilerDynamicArray<ShaderProgramBinaryCodeBlock> codeBlocks;
		ShaderCompilerDynamicArray<U64> sourceCodeHashes;
		ShaderCompilerDynamicArray<ShaderProgramBinaryMutation> mutations;
		mutations.resize(mutationCount);
		ShaderCompilerHashMap<U64, U32> mutationHashToIdx;

		// Grow the storage of the variants array. Can't have it resize, threads will work on stale data
		variants.resizeStorage(mutationCount);

		mutationCount = 0;

		// Spin for all possible combinations of mutators and
		// - Create the spirv
		// - Populate the binary variant
		do
		{
			// Create the mutation
			for(U32 i = 0; i < parser.getMutators().getSize(); ++i)
			{
				mutationValues[i] = parser.getMutators()[i].m_values[dials[i]];
			}

			ShaderProgramBinaryMutation& mutation = mutations[mutationCount++];
			newArray(memPool, mutationValues.getSize(), mutation.m_values);
			memcpy(mutation.m_values.getBegin(), mutationValues.getBegin(), mutationValues.getSizeInBytes());

			mutation.m_hash = computeHash(mutationValues.getBegin(), mutationValues.getSizeInBytes());
			ANKI_ASSERT(mutation.m_hash > 0);

			if(parser.skipMutation(mutationValues))
			{
				mutation.m_variantIndex = kMaxU32;
			}
			else
			{
				// New and unique mutation and thus variant, add it

				compileVariantAsync(parser, mutation, variants, codeBlocks, sourceCodeHashes, taskManager, mtx, errorAtomic);

				ANKI_ASSERT(mutationHashToIdx.find(mutation.m_hash) == mutationHashToIdx.getEnd());
				mutationHashToIdx.emplace(mutation.m_hash, mutationCount - 1);
			}
		} while(!spinDials(dials, parser.getMutators()));

		ANKI_ASSERT(mutationCount == mutations.getSize());

		// Done, wait the threads
		ANKI_CHECK(taskManager.joinTasks());

		// Now error out
		ANKI_CHECK(Error(errorAtomic.getNonAtomically()));

		// Store temp containers to binary
		codeBlocks.moveAndReset(binary->m_codeBlocks);
		mutations.moveAndReset(binary->m_mutations);
		variants.moveAndReset(binary->m_variants);
	}
	else
	{
		newArray(memPool, 1, binary->m_mutations);
		ShaderCompilerDynamicArray<ShaderProgramBinaryVariant> variants;
		ShaderCompilerDynamicArray<ShaderProgramBinaryCodeBlock> codeBlocks;
		ShaderCompilerDynamicArray<U64> sourceCodeHashes;

		compileVariantAsync(parser, binary->m_mutations[0], variants, codeBlocks, sourceCodeHashes, taskManager, mtx, errorAtomic);

		ANKI_CHECK(taskManager.joinTasks());
		ANKI_CHECK(Error(errorAtomic.getNonAtomically()));

		ANKI_ASSERT(codeBlocks.getSize() >= parser.getTechniques().getSize());
		ANKI_ASSERT(binary->m_mutations[0].m_variantIndex == 0);
		ANKI_ASSERT(variants.getSize() == 1);
		binary->m_mutations[0].m_hash = 1;

		codeBlocks.moveAndReset(binary->m_codeBlocks);
		variants.moveAndReset(binary->m_variants);
	}

	// Sort the mutations
	std::sort(binary->m_mutations.getBegin(), binary->m_mutations.getEnd(),
			  [](const ShaderProgramBinaryMutation& a, const ShaderProgramBinaryMutation& b) {
				  return a.m_hash < b.m_hash;
			  });

	// Techniques
	newArray(memPool, parser.getTechniques().getSize(), binary->m_techniques);
	for(U32 i = 0; i < parser.getTechniques().getSize(); ++i)
	{
		zeroMemory(binary->m_techniques[i].m_name);
		memcpy(binary->m_techniques[i].m_name.getBegin(), parser.getTechniques()[i].m_name.cstr(), parser.getTechniques()[i].m_name.getLength() + 1);

		binary->m_techniques[i].m_shaderTypes = parser.getTechniques()[i].m_shaderTypes;

		binary->m_shaderTypes |= parser.getTechniques()[i].m_shaderTypes;
	}

	// Structs
	if(parser.getGhostStructs().getSize())
	{
		newArray(memPool, parser.getGhostStructs().getSize(), binary->m_structs);
	}

	for(U32 i = 0; i < parser.getGhostStructs().getSize(); ++i)
	{
		const ShaderProgramParserGhostStruct& in = parser.getGhostStructs()[i];
		ShaderProgramBinaryStruct& out = binary->m_structs[i];

		zeroMemory(out);
		memcpy(out.m_name.getBegin(), in.m_name.cstr(), in.m_name.getLength() + 1);

		ANKI_ASSERT(in.m_members.getSize());
		newArray(memPool, in.m_members.getSize(), out.m_members);

		for(U32 j = 0; j < in.m_members.getSize(); ++j)
		{
			const ShaderProgramParserMember& inm = in.m_members[j];
			ShaderProgramBinaryStructMember& outm = out.m_members[j];

			zeroMemory(outm.m_name);
			memcpy(outm.m_name.getBegin(), inm.m_name.cstr(), inm.m_name.getLength() + 1);
			outm.m_offset = inm.m_offset;
			outm.m_type = inm.m_type;
		}

		out.m_size = in.m_members.getBack().m_offset + getShaderVariableDataTypeInfo(in.m_members.getBack().m_type).m_size;
	}

	return Error::kNone;
}

Error compileShaderProgram(CString fname, ShaderProgramFilesystemInterface& fsystem, ShaderProgramPostParseInterface* postParseCallback,
						   ShaderProgramAsyncTaskInterface* taskManager, const ShaderCompilerOptions& compilerOptions, ShaderProgramBinary*& binary)
{
	const Error err = compileShaderProgramInternal(fname, fsystem, postParseCallback, taskManager, compilerOptions, binary);
	if(err)
	{
		ANKI_SHADER_COMPILER_LOGE("Failed to compile: %s", fname.cstr());
		freeShaderProgramBinary(binary);
	}

	return err;
}

} // end namespace anki
