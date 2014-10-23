// Copyright (C) 2014, Panagiotis Christopoulos Charitos.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include "anki/util/StringList.h"
#include <cstring>

namespace anki {

//==============================================================================
template<typename TAlloc>
Error StringListBase<TAlloc>::join(
	Allocator alloc,
	const CString& separator,
	String& out) const
{
	if(Base::isEmpty())
	{
		return ErrorCode::NONE;
	}

	// Count the characters
	const I sepLen = separator.getLength();
	I charCount = 0;
	for(const String& str : *this)
	{
		charCount += str.getLength() + sepLen;
	}

	charCount -= sepLen; // Remove last separator
	ANKI_ASSERT(charCount > 0);

	// Allocate
	Error err = out.create(alloc, '?', charCount + 1);
	if(err)
	{
		return err;
	}

	// Append to output
	Char* to = &out[0];
	typename Base::ConstIterator it = Base::getBegin();
	for(; it != Base::getEnd(); it++)
	{
		const String& from = *it;
		std::memcpy(out, &from[0], from.getLength() * sizeof(Char));
		to += from.getLength();

		if(it != Base::end() - 1)
		{
			std::memcpy(to, &separator[0], sepLen * sizeof(Char));
			to += sepLen;
		}
	}

	return err;
}

//==============================================================================
template<typename TAlloc>
I StringListBase<TAlloc>::getIndexOf(const CString& value) const
{
	U pos = 0;

	for(auto it = Base::getBegin(); it != Base::getEnd(); ++it)
	{
		if(*it == value)
		{
			break;
		}
		++ pos;
	}

	return (pos == Base::size()) ? -1 : pos;
}

//==============================================================================
template<typename TAlloc>
Error StringListBase<TAlloc>::splitString(
	Allocator alloc,
	const CString& s, 
	const Char separator,
	Self& out)
{
	ANKI_ASSERT(out.isEmpty());

	Error err = ErrorCode::NONE;
	const Char* begin = &s[0];
	const Char* end = begin;

	while(!err)
	{
		if(*end == '\0')
		{
			if(begin < end)
			{
				err = out.emplaceBack(alloc);

				String str;
				if(!err)
				{
					err = str.create(alloc, begin, end);
				}

				if(!err)
				{
					out.getBack() = std::move(str);
				}
			}

			break;
		}
		else if(*end == separator)
		{
			if(begin < end)
			{
				err = out.emplaceBack(alloc);

				String str;
				if(!err)
				{
					err = str.create(alloc, begin, end);
				}

				if(!err)
				{
					out.getBack() = std::move(str);
					begin = end + 1;
				}
			}
			else
			{
				++begin;
			}
		}

		++end;
	}

	return err;
}

//==============================================================================
template<typename TAlloc>
void StringListBase<TAlloc>::sortAll(const Sort method)
{
	if(method == Sort::ASCENDING)
	{
		Base::sort([](const String& a, const String& b)
		{
			return a < b;
		});
	}
	else
	{
		ANKI_ASSERT(method == Sort::DESCENDING);
		Base::sort([](const String& a, const String& b)
		{
			return a < b;
		});
	}
}

} // end namespace anki
