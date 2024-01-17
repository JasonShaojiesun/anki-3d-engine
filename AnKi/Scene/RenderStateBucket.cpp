// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Scene/RenderStateBucket.h>

namespace anki {

RenderStateBucketContainer::~RenderStateBucketContainer()
{
	for(RenderingTechnique t : EnumIterable<RenderingTechnique>())
	{
		for([[maybe_unused]] ExtendedBucket& b : m_buckets[t])
		{
			ANKI_ASSERT(!b.m_program.isCreated() && b.m_userCount == 0 && b.m_meshletGroupCount == 0);
		}

		ANKI_ASSERT(m_bucketUserCount[t] == 0);
		ANKI_ASSERT(m_activeBucketCount[t] == 0);
		ANKI_ASSERT(m_meshletGroupCount[t] == 0);
	}
}

RenderStateBucketIndex RenderStateBucketContainer::addUser(const RenderStateInfo& state, RenderingTechnique technique, U32 lod0MeshletCount)
{
	// Compute state gash
	Array<U64, 3> toHash;
	toHash[0] = state.m_program->getUuid();
	toHash[1] = U64(state.m_primitiveTopology);
	toHash[2] = state.m_indexedDrawcall;
	const U64 hash = computeHash(toHash.getBegin(), toHash.getSizeInBytes());

	const U32 meshletGroupCount = lod0MeshletCount + (kMeshletGroupSize - 1) / kMeshletGroupSize;

	SceneDynamicArray<ExtendedBucket>& buckets = m_buckets[technique];

	RenderStateBucketIndex out;
	out.m_technique = technique;

	LockGuard lock(m_mtx);

	++m_bucketUserCount[technique];
	m_meshletGroupCount[technique] += meshletGroupCount;

	// Search bucket
	for(U32 i = 0; i < buckets.getSize(); ++i)
	{
		if(buckets[i].m_hash == hash)
		{
			++buckets[i].m_userCount;
			buckets[i].m_meshletGroupCount += meshletGroupCount;
			buckets[i].m_lod0MeshletCount += lod0MeshletCount;

			if(buckets[i].m_userCount == 1)
			{
				ANKI_ASSERT(!buckets[i].m_program.isCreated());
				ANKI_ASSERT(buckets[i].m_meshletGroupCount == meshletGroupCount && buckets[i].m_meshletGroupCount == lod0MeshletCount);
				buckets[i].m_program = state.m_program;
				++m_activeBucketCount[technique];
			}
			else
			{
				ANKI_ASSERT(buckets[i].m_program.isCreated());
			}

			out.m_index = i;
			out.m_lod0MeshletCount = lod0MeshletCount;
			return out;
		}
	}

	// Bucket not found, create one
	ExtendedBucket& newBucket = *buckets.emplaceBack();
	newBucket.m_hash = hash;
	newBucket.m_indexedDrawcall = state.m_indexedDrawcall;
	newBucket.m_primitiveTopology = state.m_primitiveTopology;
	newBucket.m_program = state.m_program;
	newBucket.m_userCount = 1;
	newBucket.m_meshletGroupCount = meshletGroupCount;
	newBucket.m_lod0MeshletCount = lod0MeshletCount;

	++m_activeBucketCount[technique];

	out.m_index = buckets.getSize() - 1;
	out.m_lod0MeshletCount = lod0MeshletCount;
	return out;
}

void RenderStateBucketContainer::removeUser(RenderStateBucketIndex& bucketIndex)
{
	if(!bucketIndex.isValid())
	{
		return;
	}

	const RenderingTechnique technique = bucketIndex.m_technique;
	const U32 idx = bucketIndex.m_index;
	const U32 meshletGroupCount = bucketIndex.m_lod0MeshletCount + (kMeshletGroupSize - 1) / kMeshletGroupSize;
	const U32 meshletCount = bucketIndex.m_lod0MeshletCount;
	bucketIndex.invalidate();

	LockGuard lock(m_mtx);

	ANKI_ASSERT(idx < m_buckets[technique].getSize());

	ANKI_ASSERT(m_bucketUserCount[technique] > 0);
	--m_bucketUserCount[technique];

	ANKI_ASSERT(m_meshletGroupCount[technique] >= meshletGroupCount);
	m_meshletGroupCount[technique] -= meshletGroupCount;

	ExtendedBucket& bucket = m_buckets[technique][idx];
	ANKI_ASSERT(bucket.m_userCount > 0 && bucket.m_program.isCreated() && bucket.m_meshletGroupCount >= meshletGroupCount
				&& bucket.m_lod0MeshletCount >= meshletCount);

	--bucket.m_userCount;
	bucket.m_meshletGroupCount -= meshletGroupCount;
	bucket.m_lod0MeshletCount -= meshletCount;

	if(bucket.m_userCount == 0)
	{
		// No more users, make sure you release any references
		bucket.m_program.reset(nullptr);

		ANKI_ASSERT(m_activeBucketCount[technique] > 0);
		--m_activeBucketCount[technique];
	}
}

} // end namespace anki
