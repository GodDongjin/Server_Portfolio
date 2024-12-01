#include "pch.h"
#include "ObjectUtils.h"

atomic<uint64> ObjectUtils::s_idGenerator = 1;

uint64 ObjectUtils:: CreateObjectId()
{
	// ID »ý¼º±â
	const uint64 newId = s_idGenerator.fetch_add(1);
	return newId;
}