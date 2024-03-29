#pragma once

#include <string>

struct File {
	uint32_t offset = 0;
	uint32_t seq = 0;
	uint32_t filesize = 0;
	std::wstring path;
};
