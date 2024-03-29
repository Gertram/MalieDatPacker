#pragma once

#include <string>

const auto index_name_length = 0x16;

struct Index{
	uint16_t flag = 0;
	uint32_t seq = 0;
	uint32_t count = 0;
	uint32_t indexSeq = 0;
	std::wstring str;
};
