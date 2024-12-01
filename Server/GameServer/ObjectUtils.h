#pragma once

class ObjectUtils
{
public:
	static uint64 CreateObjectId();
	
private:
	static atomic<uint64> s_idGenerator;
};

